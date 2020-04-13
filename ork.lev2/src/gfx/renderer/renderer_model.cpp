////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/renderable.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/gfxmodel.h>

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////
ModelDrawable::ModelDrawable(DrawableOwner* pent)
    : Drawable()
    , mfScale(1.0f)
    , mRotate(0.0f, 0.0f, 0.0f)
    , mOffset(0.0f, 0.0f, 0.0f)
    , mbShowBoundingSphere(false) {
  for (int i = 0; i < kMaxEngineParamFloats; i++)
    mEngineParamFloats[i] = 0.0f;
}
/////////////////////////////////////////////////////////////////////
ModelDrawable::~ModelDrawable() {
}
/////////////////////////////////////////////////////////////////////
void ModelDrawable::SetEngineParamFloat(int idx, float fv) {
  OrkAssert(idx >= 0 && idx < kMaxEngineParamFloats);
  mEngineParamFloats[idx] = fv;
}
/////////////////////////////////////////////////////////////////////
float ModelDrawable::GetEngineParamFloat(int idx) const {
  OrkAssert(idx >= 0 && idx < kMaxEngineParamFloats);
  return mEngineParamFloats[idx];
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::SetModelInst(xgmmodelinst_ptr_t pModelInst) {
  _modelinst                  = pModelInst;
  const lev2::XgmModel* Model = _modelinst->xgmModel();
  bool isSkinned              = Model->isSkinned();
  if (isSkinned) {
    _worldpose = std::make_shared<XgmWorldPose>(Model->skeleton());
  }
  Drawable::var_t ap;
  ap.Set(_worldpose);
  SetUserDataA(ap);
}
///////////////////////////////////////////////////////////////////////////////
xgmmodelinst_ptr_t ModelDrawable::GetModelInst() const {
  return _modelinst;
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::SetScale(float fscale) {
  mfScale = fscale;
}
///////////////////////////////////////////////////////////////////////////////
float ModelDrawable::GetScale() const {
  return mfScale;
}
///////////////////////////////////////////////////////////////////////////////
const fvec3& ModelDrawable::GetRotate() const {
  return mRotate;
}
///////////////////////////////////////////////////////////////////////////////
const fvec3& ModelDrawable::GetOffset() const {
  return mOffset;
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::SetRotate(const fvec3& v) {
  mRotate = v;
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::SetOffset(const fvec3& v) {
  mOffset = v;
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::ShowBoundingSphere(bool bflg) {
  mbShowBoundingSphere = bflg;
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::enqueueToRenderQueue(const DrawableBufItem& item, lev2::IRenderer* renderer) const {
  ork::opq::assertOnQueue2(opq::mainSerialQueue());
  auto RCFD                   = renderer->GetTarget()->topRenderContextFrameData();
  const auto& topCPD          = RCFD->topCPD();
  const lev2::XgmModel* Model = _modelinst->xgmModel();
  const auto& monofrustum     = topCPD.monoCamFrustum();

  // TODO - resolve frustum in case of stereo camera

  const ork::fmtx4& matw        = item.mXfData._worldMatrix;
  bool isPickState              = renderer->GetTarget()->FBI()->isPickState();
  bool isSkinned                = Model->isSkinned();
  ork::fvec3 center_plus_offset = mOffset + Model->boundingCenter();
  ork::fvec3 ctr                = ork::fvec4(center_plus_offset * mfScale).Transform(matw);
  ork::fvec3 vwhd               = Model->boundingAA_WHD();
  float frad                    = vwhd.GetX();
  if (vwhd.GetY() > frad)
    frad = vwhd.GetY();
  if (vwhd.GetZ() > frad)
    frad = vwhd.GetZ();
  frad *= 0.6f;

  bool bCenterInFrustum = monofrustum.contains(ctr);

  //////////////////////////////////////////////////////////////////////

  auto worldpose = GetUserDataA().Get<xgmworldpose_ptr_t>();

  ork::fvec3 matw_trans;
  ork::fquat matw_rot;
  float matw_scale;

  matw.decompose(matw_trans, matw_rot, matw_scale);

  //////////////////////////////////////////////////////////////////////

  int inumacc = 0;
  int inumrej = 0;

  int inummeshes = Model->numMeshes();
  for (int imesh = 0; imesh < inummeshes; imesh++) {
    const lev2::XgmMesh& mesh = *Model->mesh(imesh);

    // if( 0 == strcmp(mesh.meshName().c_str(),"fg_2_1_3_ground_SG_ground_GeoDaeId") )
    //{
    //	orkprintf( "yo\n" );
    //}

    if (_modelinst->isMeshEnabled(imesh)) {
      int inumclusset = mesh.numSubMeshes();

      for (int ics = 0; ics < inumclusset; ics++) {
        const lev2::XgmSubMesh& submesh   = *mesh.subMesh(ics);
        const lev2::GfxMaterial* material = submesh.mpMaterial;

        int inumclus = submesh.miNumClusters;

        for (int ic = 0; ic < inumclus; ic++) {
          bool btest = true;

          const lev2::XgmCluster& cluster = submesh.cluster(ic);

          if (isSkinned) {

            float fdb = monofrustum._bottomPlane.pointDistance(ctr);
            float fdt = monofrustum._topPlane.pointDistance(ctr);
            float fdl = monofrustum._leftPlane.pointDistance(ctr);
            float fdr = monofrustum._rightPlane.pointDistance(ctr);
            float fdn = monofrustum._nearPlane.pointDistance(ctr);
            float fdf = monofrustum._farPlane.pointDistance(ctr);

            const float kdist = -5.0f;
            btest             = (fdb > kdist) && (fdt > kdist) && (fdl > kdist) && (fdr > kdist) &&
                    (fdn > kdist)
                    //&&	(fdn<100.0f); // 50m actors
                    && (fdf > kdist);
            if (false == btest) {
            }
            btest = true; // todo fix culler
          } else {        // Rigid
            const Sphere& bsph = cluster.mBoundingSphere;

            float clussphrad = bsph.mRadius * matw_scale * mfScale;
            fvec3 clussphctr = ((bsph.mCenter + mOffset) * mfScale).Transform(matw);
            Sphere sph2(clussphctr, clussphrad);

            btest = true; // CollisionTester::FrustumSphereTest( frus, sph2 );
          }

          if (btest) {
            lev2::ModelRenderable& renderable = renderer->enqueueModel();

            // if(mEngineParamFloats[0] < 1.0f && mEngineParamFloats[0] > 0.0f)
            //	orkprintf("mEngineParamFloats[0] = %g\n", mEngineParamFloats[0]);

            for (int i = 0; i < kMaxEngineParamFloats; i++)
              renderable.SetEngineParamFloat(i, mEngineParamFloats[i]);

            renderable.SetModelInst(std::const_pointer_cast<const XgmModelInst>(_modelinst));
            renderable.SetObject(GetOwner());
            renderable.SetMesh(&mesh);
            renderable.SetSubMesh(&submesh);
            renderable.SetCluster(&cluster);
            renderable.SetModColor(renderer->GetTarget()->RefModColor());
            renderable.SetMatrix(matw);
            // renderable.SetLightMask(lmask);
            renderable.SetScale(mfScale);
            renderable.SetRotate(mRotate);
            renderable.SetOffset(mOffset);

            size_t umat = size_t(material);
            u32 imtla   = (umat & 0xff);
            u32 imtlb   = ((umat >> 8) & 0xff);
            u32 imtlc   = ((umat >> 16) & 0xff);
            u32 imtld   = ((umat >> 24) & 0xff);
            u32 imtl    = (imtla + imtlb + imtlc + imtld) & 0xff;

            int isortpass = (material->GetRenderQueueSortingData().miSortingPass + 16) & 0xff;
            int isortoffs = material->GetRenderQueueSortingData().miSortingOffset;

            int isortkey = (isortpass << 24) | (isortoffs << 16) | imtl;

            renderable.SetSortKey(isortkey);
            // orkprintf( " ModelDrawable::enqueueToRenderQueue() rable<%p> \n", & renderable );

            inumacc++;
          } else {
            inumrej++;
          }
        }
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
ModelRenderable::ModelRenderable(IRenderer* renderer)
    : IRenderable()
    , mSortKey(0)
    , mMaterialIndex(0)
    , mMaterialPassIndex(0)
    , mScale(1.0f)
    , mEdgeColor(-1)
    , mMesh(0)
    , mSubMesh(0)
    , mCluster(0)
    , mRotate(0.0f, 0.0f, 0.0f)
    , mOffset(0.0f, 0.0f, 0.0f) {
  for (int i = 0; i < kMaxEngineParamFloats; i++)
    mEngineParamFloats[i] = 0.0f;
}
///////////////////////////////////////////////////////////////////////////////
void ModelRenderable::SetEngineParamFloat(int idx, float fv) {
  OrkAssert(idx >= 0 && idx < kMaxEngineParamFloats);
  mEngineParamFloats[idx] = fv;
}
///////////////////////////////////////////////////////////////////////////////
float ModelRenderable::GetEngineParamFloat(int idx) const {
  OrkAssert(idx >= 0 && idx < kMaxEngineParamFloats);
  return mEngineParamFloats[idx];
}
///////////////////////////////////////////////////////////////////////////////
void ModelRenderable::Render(const IRenderer* renderer) const {
  renderer->RenderModel(*this);
}
///////////////////////////////////////////////////////////////////////////////
bool ModelRenderable::CanGroup(const IRenderable* oth) const {
  auto pren = dynamic_cast<const ModelRenderable*>(oth);
  if (pren) {
    const lev2::XgmSubMesh* submesh = pren->subMesh();
    const GfxMaterial* mtl          = submesh->GetMaterial();
    const GfxMaterial* mtl2         = subMesh()->GetMaterial();
    return (mtl == mtl2);
  }
  return false;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetMaterialIndex(int idx) {
  mMaterialIndex = idx;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetMaterialPassIndex(int idx) {
  mMaterialPassIndex = idx;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetModelInst(xgmmodelinst_constptr_t modelInst) {
  _modelinst = modelInst;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetEdgeColor(int edge_color) {
  mEdgeColor = edge_color;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetScale(float scale) {
  mScale = scale;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetSubMesh(const lev2::XgmSubMesh* cs) {
  mSubMesh = cs;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetCluster(const lev2::XgmCluster* c) {
  mCluster = c;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetMesh(const lev2::XgmMesh* m) {
  mMesh = m;
}
/////////////////////////////////////////////////////////////////////
float ModelRenderable::GetScale() const {
  return mScale;
}
/////////////////////////////////////////////////////////////////////
xgmmodelinst_constptr_t ModelRenderable::GetModelInst() const {
  return _modelinst;
}
/////////////////////////////////////////////////////////////////////
int ModelRenderable::GetMaterialIndex(void) const {
  return mMaterialIndex;
}
/////////////////////////////////////////////////////////////////////
int ModelRenderable::GetMaterialPassIndex(void) const {
  return mMaterialPassIndex;
}
/////////////////////////////////////////////////////////////////////
int ModelRenderable::GetEdgeColor() const {
  return mEdgeColor;
}
/////////////////////////////////////////////////////////////////////
const lev2::XgmSubMesh* ModelRenderable::subMesh(void) const {
  return mSubMesh;
}
/////////////////////////////////////////////////////////////////////
const lev2::XgmCluster* ModelRenderable::GetCluster(void) const {
  return mCluster;
}
/////////////////////////////////////////////////////////////////////
const lev2::XgmMesh* ModelRenderable::mesh(void) const {
  return mMesh;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetSortKey(uint32_t skey) {
  mSortKey = skey;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetRotate(const fvec3& v) {
  mRotate = v;
}
/////////////////////////////////////////////////////////////////////
void ModelRenderable::SetOffset(const fvec3& v) {
  mOffset = v;
}
/////////////////////////////////////////////////////////////////////
const fvec3& ModelRenderable::GetRotate() const {
  return mRotate;
}
/////////////////////////////////////////////////////////////////////
const fvec3& ModelRenderable::GetOffset() const {
  return mOffset;
}
/////////////////////////////////////////////////////////////////////
uint32_t ModelRenderable::ComposeSortKey(const IRenderer* renderer) const {
  return mSortKey;
}
/////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
