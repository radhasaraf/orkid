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
#include <ork/lev2/gfx/fxstate_instance.h>

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////
InstancedDrawable::InstancedDrawable() 
  : Drawable() {
  _instancedata = std::make_shared<InstancedDrawableInstanceData>();
  _drawcount = 0;
}
///////////////////////////////////////////////////////////////////////////////
void InstancedDrawable::resize(size_t count) {
  OrkAssert(count <= k_max_instances);
  _instancedata->resize(count);
  _count = count;
}
///////////////////////////////////////////////////////////////////////////////
InstancedModelDrawable::InstancedModelDrawable()
    : InstancedDrawable() {
}
/////////////////////////////////////////////////////////////////////
InstancedModelDrawable::~InstancedModelDrawable() {
}
///////////////////////////////////////////////////////////////////////////////
struct IMDIMPL_SUBMESH {
  const XgmSubMesh* _xgmsubmesh = nullptr;
  fxinstancelut_ptr_t _fxlut;
};
struct IMDIMPL_MODEL {
  std::vector<IMDIMPL_SUBMESH> _submeshes;
};
using imdimpl_model_ptr_t = std::shared_ptr<IMDIMPL_MODEL>;

///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::bindModelAsset(AssetPath assetpath) {
  _asset = asset::AssetManager<XgmModelAsset>::load(assetpath);
  bindModel(_asset->_model.atomicCopy());
}
///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::bindModel(model_ptr_t model) {
  _model = model;
  // generate material instance data
  auto impl = std::make_shared<IMDIMPL_MODEL>();
  _impl.set<imdimpl_model_ptr_t>(impl);
  int inummeshes = _model->numMeshes();
  for (int imesh = 0; imesh < inummeshes; imesh++) {
    auto mesh       = _model->mesh(imesh);
    int inumclusset = mesh->numSubMeshes();
    for (int ics = 0; ics < inumclusset; ics++) {
      auto xgmsub = mesh->subMesh(ics);
      IMDIMPL_SUBMESH submesh_impl;
      submesh_impl._fxlut = xgmsub->_material->createFxStateInstanceLut();
      submesh_impl._xgmsubmesh = xgmsub;
      impl->_submeshes.push_back(submesh_impl);
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::gpuInit(Context* ctx) const {
  _instanceMatrixTex = Texture::createBlank(1024, 256, EBufferFormat::RGBA32F);
  _instanceColorTex  = Texture::createBlank(1024, 256, EBufferFormat::RGBA32F);
  _instanceIdTex     = Texture::createBlank(1024, 128, EBufferFormat::RGBA16UI);

  _instanceMatrixTex->_debugName = "_instanceMatrixTex";
  _instanceColorTex->_debugName  = "_instanceColorTex";
  _instanceIdTex->_debugName     = "_instanceIdTex";
}
///////////////////////////////////////////////////////////////////////////////
void InstancedModelDrawable::enqueueToRenderQueue(
    drawablebufitem_constptr_t item, //
    lev2::IRenderer* renderer) const {
  ork::opq::assertOnQueue2(opq::mainSerialQueue());
  ////////////////////////////////////////////////////////////////////
  if (not _model)
    return;
  if (not _impl.isA<imdimpl_model_ptr_t>())
    return;
  ////////////////////////////////////////////////////////////////////
  auto context                         = renderer->GetTarget();
  auto RCFD                            = context->topRenderContextFrameData();
  const auto& topCPD                   = RCFD->topCPD();
  const auto& monofrustum              = topCPD.monoCamFrustum();
  lev2::CallbackRenderable& renderable = renderer->enqueueCallback();
  ////////////////////////////////////////////////////////////////////
  bool isPick    = context->FBI()->isPickState();
  bool isSkinned = _model->isSkinned();
  OrkAssert(false == isSkinned); // not yet..
  if (not _instanceMatrixTex) {
    gpuInit(context); // todo figure out better do-only-once method...
  }
  ////////////////////////////////////////////////////////////////////
  renderable.SetObject(GetOwner());
  renderable.SetSortKey(0x00000001);
  renderable.SetDrawableDataA(GetUserDataA());
  renderable.SetDrawableDataB(GetUserDataB());
  //renderable.SetUserData0(item->_userdata[0]);
  //renderable.SetUserData1(item->_userdata[1]);
  renderable._instanced = true;
  ////////////////////////////////////////////////////////////////////
  renderable.SetRenderCallback([this](lev2::RenderContextInstData& RCID) { //
    auto context     = RCID.context();
    auto GBI         = context->GBI();
    auto TXI         = context->TXI();
    auto FXI         = context->FXI();
    auto FBI         = context->FBI();
    auto impl        = _impl.getShared<IMDIMPL_MODEL>();
    bool isPick      = FBI->isPickState();
    bool isStereo    = RCID._RCFD->isStereo();
    int fxinst_index = isStereo ? 1 : (isPick ? 2 : 0);
    ////////////////////////////////////////////////////////
    bool updatetex = true; //( (_drawcount++) < 5000);
    ////////////////////////////////////////////////////////
    // upload instance matrices to GPU
    ////////////////////////////////////////////////////////
    TextureInitData texdata;
    texdata._w           = k_texture_dimension_x; // 64 bytes per instance
    texdata._h           = k_texture_dimension_y;
    texdata._format      = EBufferFormat::RGBA32F;
    texdata._autogenmips = false;
    texdata._data        = (const void*)_instancedata->_worldmatrices.data();
    OrkAssert(_count <= k_max_instances);
    if(updatetex)
      TXI->initTextureFromData(_instanceMatrixTex.get(), texdata);
    ////////////////////////////////////////////////////////
    texdata._w    = k_texture_dimension_x; // 16 bytes per instance
    texdata._h    = k_texture_dimension_y / 4;
    texdata._data = (const void*)_instancedata->_modcolors.data();
    if(updatetex)
      TXI->initTextureFromData(_instanceColorTex.get(), texdata);
    ////////////////////////////////////////////////////////
    texdata._w           = k_texture_dimension_x; // 8 bytes per instance
    texdata._h           = k_texture_dimension_y / 8;
    texdata._format      = EBufferFormat::RGBA16UI;
    texdata._autogenmips = false;
    texdata._data        = (const void*)_instancedata->_pickids.data();

    if(updatetex)
      TXI->initTextureFromData(_instanceIdTex.get(), texdata);

    _instanceIdTex->TexSamplingMode().PresetPointAndClamp();
    TXI->ApplySamplingMode(_instanceIdTex.get());
    ////////////////////////////////////////////////////////
    // instanced render
    ////////////////////////////////////////////////////////
    RCID._isInstanced = true;
    for (auto& sub : impl->_submeshes) {
      auto xgmsub = sub._xgmsubmesh;
      auto fxlut = sub._fxlut;
      OrkAssert(fxlut);
      auto fxinst = fxlut->findfxinst(RCID);
      OrkAssert(fxinst);
      fxinst->wrappedDrawCall(RCID, [&]() {
        auto idata = _instancedata;
        ////////////////////////////////////
        // bind instancetex to sampler
        ////////////////////////////////////
        FXI->BindParamCTex(fxinst->_parInstanceMatrixMap, _instanceMatrixTex.get());
        FXI->BindParamCTex(fxinst->_parInstanceIdMap, _instanceIdTex.get());
        FXI->BindParamCTex(fxinst->_parInstanceColorMap, _instanceColorTex.get());
        ////////////////////////////////////
        int inumclus = xgmsub->_clusters.size();
        for (int ic = 0; ic < inumclus; ic++) {
          auto cluster    = xgmsub->cluster(ic);
          auto vtxbuf     = cluster->_vertexBuffer;
          size_t numprims = cluster->numPrimGroups();
          for (size_t ipg = 0; ipg < numprims; ipg++) {
            auto primgroup = cluster->primgroup(ipg);
            auto idxbuf    = primgroup->mpIndices;
            auto primtype  = primgroup->mePrimType;
            int numindices = primgroup->miNumIndices;
            GBI->DrawInstancedIndexedPrimitiveEML(*vtxbuf, *idxbuf, primtype, _count);
          }
        }
      }); // mtlinst->wrappedDrawCall(RCID, [&]() {
    }     // for (auto& sub : impl._submeshes) {
    RCID._isInstanced = false;
  });     // renderable.SetRenderCallback
  ////////////////////////////////////////////////////////////////////
} // InstancedModelDrawable::enqueueToRenderQueue(
/////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
