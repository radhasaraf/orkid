////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/renderable.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/util/logger.h>

namespace ork::lev2 {
static logchannel_ptr_t logchan_model = logger()->createChannel("model",fvec3(0.9,0.2,0.9));
///////////////////////////////////////////////////////////////////////////////
ModelDrawable::ModelDrawable(DrawableOwner* pent) {
  for (int i = 0; i < kMaxEngineParamFloats; i++)
    mEngineParamFloats[i] = 0.0f;
}
/////////////////////////////////////////////////////////////////////
ModelDrawable::~ModelDrawable() {
}
/////////////////////////////////////////////////////////////////////
void ModelDrawable::setEngineParamFloat(int idx, float fv) {
  OrkAssert(idx >= 0 && idx < kMaxEngineParamFloats);
  mEngineParamFloats[idx] = fv;
}
/////////////////////////////////////////////////////////////////////
float ModelDrawable::getEngineParamFloat(int idx) const {
  OrkAssert(idx >= 0 && idx < kMaxEngineParamFloats);
  return mEngineParamFloats[idx];
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::bindModelInst(xgmmodelinst_ptr_t minst) {
  logchan_model->log("drw<%s> bindModelInst(%p)", _name.c_str(), minst.get() );
  _modelinst                  = minst;
  const lev2::XgmModel* Model = _modelinst->xgmModel();
  bool isSkinned              = Model->isSkinned();
  if (isSkinned) {
    _worldpose = std::make_shared<XgmWorldPose>(Model->skeleton());
  }
  Drawable::var_t ap;
  ap.set(_worldpose);
  SetUserDataA(ap);
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::bindModelAsset(AssetPath assetpath) {

  logchan_model->log("drw<%s> bindModelAsset(%s)", _name.c_str(), assetpath.c_str() );

  ork::opq::assertOnQueue(opq::mainSerialQueue());

  asset::vars_ptr_t asset_vars;

  if (_data) {

    asset_vars = std::make_shared<asset::vars_t>();

    for (auto item : _data->_assetvars) {

      const std::string& k = item.first;
      const rendervar_t& v = item.second;

      std::string v_str;
      if (auto as_str = v.tryAs<std::string>()) {
        asset_vars->makeValueForKey<std::string>(k, as_str.value());
        v_str = as_str.value();
      } else if (auto as_bool = v.tryAs<bool>()) {
        asset_vars->makeValueForKey<bool>(k, as_bool.value());
        v_str = as_bool.value() ? "true" : "false";
      } else if (auto as_dbl = v.tryAs<double>()) {
        asset_vars->makeValueForKey<double>(k, as_dbl.value());
        v_str = FormatString("%f", as_dbl.value());
      } else {
        OrkAssert(false);
      }

      logchan_model->log("modelassetvar k<%s> v<%s>", k.c_str(), v_str.c_str());
    }
  }

  _asset = asset::AssetManager<XgmModelAsset>::load(assetpath, asset_vars);
  bindModel(_asset->_model.atomicCopy());
}
void ModelDrawable::bindModelAsset(xgmmodelassetptr_t asset) {
  _asset = asset;
  bindModel(_asset->_model.atomicCopy());
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::bindModel(model_ptr_t model) {
  logchan_model->log("drw<%s> bindModel(%p)", _name.c_str(), (void*) model.get() );
  _model         = model;
  auto modelinst = std::make_shared<XgmModelInst>(_model.get());
  bindModelInst(modelinst);
}
///////////////////////////////////////////////////////////////////////////////
void ModelDrawable::enqueueToRenderQueue(drawablebufitem_constptr_t item, lev2::IRenderer* renderer) const {
  ork::opq::assertOnQueue2(opq::mainSerialQueue());
  auto RCFD                   = renderer->GetTarget()->topRenderContextFrameData();
  const auto& topCPD          = RCFD->topCPD();
  const lev2::XgmModel* Model = _modelinst->xgmModel();
  const auto& monofrustum     = topCPD.monoCamFrustum();

  // TODO - resolve frustum in case of stereo camera

  const ork::fmtx4 matw         = item->mXfData._worldTransform->composed();
  bool isPickState              = renderer->GetTarget()->FBI()->isPickState();
  bool isSkinned                = Model->isSkinned();
  ork::fvec3 center_plus_offset = _offset + Model->boundingCenter();
  ork::fvec3 ctr                = ork::fvec4(center_plus_offset * _scale).transform(matw);
  ork::fvec3 vwhd               = Model->boundingAA_WHD();
  float frad                    = vwhd.x;
  if (vwhd.y > frad)
    frad = vwhd.y;
  if (vwhd.z > frad)
    frad = vwhd.z;
  frad *= 0.6f;

  bool bCenterInFrustum = monofrustum.contains(ctr);

  //////////////////////////////////////////////////////////////////////

  ork::fvec3 matw_trans;
  ork::fquat matw_rot;
  float matw_scale;

  matw.decompose(matw_trans, matw_rot, matw_scale);

  int inumacc = 0;
  int inumrej = 0;

  //////////////////////////////////////////////////////////////////////

  auto do_submesh = [&](xgmsubmeshinst_ptr_t submeshinst) {
    auto submesh = submeshinst->_submesh;

    auto material = submesh->_material;

    int inumclus = submesh->_clusters.size();

    for (int ic = 0; ic < inumclus; ic++) {
      bool btest = true;

      auto cluster = submesh->cluster(ic);

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
                //&&  (fdn<100.0f); // 50m actors
                && (fdf > kdist);
        if (false == btest) {
        }
        btest = true; // todo fix culler
      } else {        // Rigid
        const Sphere& bsph = cluster->mBoundingSphere;

        float clussphrad = bsph.mRadius * matw_scale * _scale;
        fvec3 clussphctr = ((bsph.mCenter + _offset) * _scale).transform(matw);
        Sphere sph2(clussphctr, clussphrad);

        btest = true; // CollisionTester::FrustumSphereTest( frus, sph2 );
      }

      if (btest) {
        lev2::ModelRenderable& renderable = renderer->enqueueModel();

        // if(mEngineParamFloats[0] < 1.0f && mEngineParamFloats[0] > 0.0f)
        //  orkprintf("mEngineParamFloats[0] = %g\n", mEngineParamFloats[0]);

        for (int i = 0; i < kMaxEngineParamFloats; i++)
          renderable.SetEngineParamFloat(i, mEngineParamFloats[i]);

        renderable._modelinst = std::const_pointer_cast<const XgmModelInst>(_modelinst);
        renderable.SetObject(GetOwner());
        renderable._submeshinst = submeshinst;
        renderable._cluster = cluster;
        renderable.SetModColor(_modcolor);
        renderable.SetMatrix(matw);
        // renderable.SetLightMask(lmask);
        renderable._scale = _scale;
        renderable._orientation = _orientation;
        renderable._offset = _offset;

        size_t umat = size_t(material.get());
        u32 imtla   = (umat & 0xff);
        u32 imtlb   = ((umat >> 8) & 0xff);
        u32 imtlc   = ((umat >> 16) & 0xff);
        u32 imtld   = ((umat >> 24) & 0xff);
        u32 imtl    = (imtla + imtlb + imtlc + imtld) & 0xff;

        const auto& rqsortdata = material->GetRenderQueueSortingData();

        int isortpass = (rqsortdata.miSortingPass + 16) & 0xff;
        int isortoffs = rqsortdata.miSortingOffset;

        int isortkey = (isortpass << 24) | (isortoffs << 16) | imtl;

        renderable._sortkey = isortkey;
        // orkprintf( " ModelDrawable::enqueueToRenderQueue() rable<%p> \n", & renderable );

        if (item->_onrenderable) {
          item->_onrenderable(&renderable);
        }

        inumacc++;
      } else {
        inumrej++;
      }
    }
  };
  //////////////////////////////////////////////////////////////////////

  for( auto submeshinst : _modelinst->_submeshinsts ){
    auto submesh = submeshinst->_submesh;
    if(submeshinst->_enabled)
      do_submesh(submeshinst);
  }

}
///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
ModelRenderable::ModelRenderable(IRenderer* renderer) {
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
  //renderer->RenderModel(*this);
  auto context = renderer->GetTarget();
  auto minst   = this->_modelinst;
  auto model   = minst->xgmModel();
  auto submesh = _submeshinst->_submesh;
  auto mesh = submesh->_parentmesh;

  context->debugPushGroup(FormatString("DefaultRenderer::RenderModel model<%p> minst<%p>", model, minst.get()));
  /////////////////////////////////////////////////////////////
  float fscale        = this->_scale;
  const fvec3& offset = this->_offset;
  fmtx4 smat, tmat, rmat;
  smat.setScale(fscale);
  tmat.setTranslation(offset);
  //rmat.setRotateY(rotate.y + rotate.z);
  rmat.fromQuaternion(_orientation);
  fmtx4 wmat = this->GetMatrix();
  /////////////////////////////////////////////////////////////
  // compute world matrix
  /////////////////////////////////////////////////////////////
  fmtx4 nmat = fmtx4::multiply_ltor(tmat,rmat,smat,wmat);
  if (minst->isBlenderZup()) { // zup to yup conversion matrix
    fmtx4 rmatx, rmaty;
    rmatx.rotateOnX(3.14159f * -0.5f);
    rmaty.rotateOnX(3.14159f);
    nmat = fmtx4::multiply_ltor(rmatx,rmaty,nmat);
  }
  /////////////////////////////////////////////////////////////
  RenderContextInstData RCID;
  RenderContextInstModelData RCID_MD;
  //RCID.SetMaterialInst(&minst->RefMaterialInst());
  auto RCFD = context->topRenderContextFrameData();
  RCID._RCFD = RCFD;
  RCID_MD.mMesh    = mesh;
  RCID_MD.mSubMesh = submesh;
  RCID_MD._cluster = this->_cluster;
  //RCID.SetMaterialIndex(0);
  RCID.SetRenderer(renderer);
  RCID._dagrenderable = this;
  RCID._fx_instance_lut = _submeshinst->_fxinstancelut;
  // context->debugMarker(FormatString("toolrenderer::RenderModel isskinned<%d> owner_as_ent<%p>", int(model->isSkinned()),
  // as_ent));
  ///////////////////////////////////////
  // printf( "Renderer::RenderModel() rable<%p>\n", & ModelRen );
  //logchan_model->log("renderable<%p> fxlut(%p)", (void*) this, (void*) RCID._fx_instance_lut.get() );
  bool model_is_skinned = model->isSkinned();
  RCID._isSkinned       = model_is_skinned;
  RCID_MD.SetSkinned(model_is_skinned);
  RCID_MD.SetModelInst(minst);
  auto ObjColor = this->_modColor;
  if (model_is_skinned) {
    model->RenderSkinned(minst.get(), ObjColor, nmat, context, RCID, RCID_MD);
  } else {
    model->RenderRigid(ObjColor, nmat, context, RCID, RCID_MD);
  }
  context->debugPopGroup();
}
/////////////////////////////////////////////////////////////////////
uint32_t ModelRenderable::ComposeSortKey(const IRenderer* renderer) const {
  return _sortkey;
}
/////////////////////////////////////////////////////////////////////
} // namespace ork::lev2