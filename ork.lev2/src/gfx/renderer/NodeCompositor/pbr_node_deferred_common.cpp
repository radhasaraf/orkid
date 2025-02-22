////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <algorithm>
#include <ork/pch.h>
#include <ork/kernel/opq.h>
#include <ork/kernel/mutex.h>
#include <ork/reflect/properties/register.h>
#include <ork/application/application.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/lev2/gfx/material_pbr.inl>

#include <ork/lev2/gfx/renderer/NodeCompositor/pbr_node_deferred.h>
#include <ork/profiling.inl>

namespace ork::lev2::pbr::deferrednode {
///////////////////////////////////////////////////////////////////////////////

DeferredContext::DeferredContext(RenderCompositingNode* node, std::string shadername, int numlights)
    : _node(node) {
  ///////////
  _shadername = shadername;
  _layername  = "All";

  for (int i = 0; i < numlights; i++) {

    auto p = new PointLight;
    p->next();
    p->_color.x = float(rand() & 0xff) / 256.0;
    p->_color.y = float(rand() & 0xff) / 256.0;
    p->_color.z = float(rand() & 0xff) / 256.0;
    p->_radius  = 16 + float(rand() & 0xff) / 2.0;
    _pointlights.push_back(p);
  }
  _lightAccumFormat = EBufferFormat::RGBA16F;
}

///////////////////////////////////////////////////////////////////////////////

DeferredContext::~DeferredContext() {
}

///////////////////////////////////////////////////////////////////////////////

lev2::texture_ptr_t DeferredContext::brdfIntegrationTexture() const {
  return _brdfIntegrationMap;
}

///////////////////////////////////////////////////////////////////////////////

auxparambinding_ptr_t DeferredContext::createAuxParamBinding(std::string paramname){
  auto it = _auxbindings.find(paramname);
  OrkAssert(it==_auxbindings.end());
  auto mapping = std::make_shared<AuxParamBinding>();
  _auxbindings[paramname] = mapping;
  return mapping;
}

///////////////////////////////////////////////////////////////////////////////
// deferred layout
// rt0/GL_RGBA8    (32,32)  - albedo,ao (primary color)
// rt1/GL_RGB10_A2 (32,64)  - normal,model
// rt2/GL_RGBA8    (32,96)  - mtl,ruf,aux1,aux2
// rt3/GL_R32F     (32,128) - depth
///////////////////////////////////////

void DeferredContext::gpuInit(Context* target) {
  //target->debugPushGroup("Deferred::rendeinitr");
  auto FXI = target->FXI();
  if (nullptr == _rtgs_gbuffer) {
    _brdfIntegrationMap = PBRMaterial::brdfIntegrationMap(target);
    //////////////////////////////////////////////////////////////
    printf( "LOADING DeferredContext SHADER<%s>\n", _shadername.c_str() );
    _lightingmtl = std::make_shared<FreestyleMaterial>();
    _lightingmtl->gpuInit(target, _shadername);
    _tekBaseLighting       = _lightingmtl->technique("baselight");
    _tekBaseLightingStereo = _lightingmtl->technique("baselight_stereo");
    //
    _tekPointLightingUntextured       = _lightingmtl->technique("pointlight_untextured");
    _tekPointLightingTextured         = _lightingmtl->technique("pointlight_textured");
    _tekPointLightingUntexturedStereo = _lightingmtl->technique("pointlight_untextured_stereo");
    _tekPointLightingTexturedStereo   = _lightingmtl->technique("pointlight_textured_stereo");
    //
    _tekSpotLightingUntextured             = _lightingmtl->technique("spotlight_untextured");
    _tekSpotLightingTextured               = _lightingmtl->technique("spotlight_textured");
    _tekSpotLightingUntexturedStereo       = _lightingmtl->technique("spotlight_untextured_stereo");
    _tekSpotLightingTexturedStereo         = _lightingmtl->technique("spotlight_textured_stereo");
    _tekSpotLightingTexturedShadowed       = _lightingmtl->technique("spotlight_textured_shadowed");
    _tekSpotLightingTexturedShadowedStereo = _lightingmtl->technique("spotlight_textured_shadowed_stereo");
    //
    _tekSpotDecalingTexturedStereo = _lightingmtl->technique("spotdecal_textured_stereo");
    _tekSpotDecalingTextured       = _lightingmtl->technique("spotdecal_textured");
    //
    _tekDownsampleDepthCluster = _lightingmtl->technique("downsampledepthcluster");

    _tekEnvironmentLightingSDF       = _lightingmtl->technique("environmentlightingSDF");
    _tekEnvironmentLightingSDFStereo = _lightingmtl->technique("environmentlightingSDF_stereo");

    _tekEnvironmentLighting       = _lightingmtl->technique("environmentlighting");
    _tekEnvironmentLightingStereo = _lightingmtl->technique("environmentlighting_stereo");

    //////////////////////////////////////////////////////////////
    // init lightblock
    //////////////////////////////////////////////////////////////
    _lightblock = _lightingmtl->paramBlock("ub_light");
    //////////////////////////////////////////////////////////////
    _parMatIVPArray = _lightingmtl->param("IVPArray");
    _parMatVArray   = _lightingmtl->param("VArray");
    _parMatPArray   = _lightingmtl->param("PArray");
    _parMapGBuf     = _lightingmtl->param("MapGBuffer");
    _parMapDepth       = _lightingmtl->param("MapDepth");
    _parMapShadowDepth = _lightingmtl->param("MapShadowDepth");
    _parMapDepthCluster     = _lightingmtl->param("MapDepthCluster");
    _parLightCookieTexture  = _lightingmtl->param("MapLightingCookie");
    _parMapSpecularEnv      = _lightingmtl->param("MapSpecularEnv");
    _parMapDiffuseEnv       = _lightingmtl->param("MapDiffuseEnv");
    _parMapBrdfIntegration  = _lightingmtl->param("MapBrdfIntegration");
    _parMapVolTexA          = _lightingmtl->param("MapVolTexA");
    _parInvViewSize         = _lightingmtl->param("InvViewportSize");
    _parTime                = _lightingmtl->param("Time");
    _parNumLights           = _lightingmtl->param("NumLights");
    _parTileDim             = _lightingmtl->param("TileDim");
    _parNearFar             = _lightingmtl->param("NearFar");
    _parZndc2eye            = _lightingmtl->param("Zndc2eye");
    _parEnvironmentMipBias  = _lightingmtl->param("EnvironmentMipBias");
    _parEnvironmentMipScale = _lightingmtl->param("EnvironmentMipScale");
    _parSpecularMipBias     = _lightingmtl->param("SpecularMipBias");
    _parSpecularLevel       = _lightingmtl->param("SpecularLevel");
    _parDiffuseLevel        = _lightingmtl->param("DiffuseLevel");
    _parAmbientLevel        = _lightingmtl->param("AmbientLevel");
    _parSkyboxLevel         = _lightingmtl->param("SkyboxLevel");
    _parDepthFogDistance    = _lightingmtl->param("DepthFogDistance");
    _parDepthFogPower       = _lightingmtl->param("DepthFogPower");
    _parShadowParams        = _lightingmtl->param("ShadowParams");

    OrkAssert(_parDepthFogDistance);
    //////////////////////////////////////////////////////////////
    _rtgs_gbuffer = std::make_shared<RtgSet>(target,MsaaSamples::MSAA_1X, true);
    _rtgs_gbuffer->addBuffer("DeferredGbuffer", EBufferFormat::RGBA32UI);
    _rtgs_gbuffer->_autoclear = false;
    //////////////////////////////////////////////////////////////
    _rtgs_laccum = std::make_shared<RtgSet>(target,MsaaSamples::MSAA_1X, true);
    _rtgs_laccum->addBuffer("DeferredLightAccum", _lightAccumFormat);
    if(_auxBufferFormat!=EBufferFormat::NONE)
      _rtgs_laccum->addBuffer("Auxiliary", _auxBufferFormat);
    _rtgs_laccum->_autoclear = false;
    //////////////////////////////////////////////////////////////
    auto mtl_load_req1 = std::make_shared<asset::LoadRequest>("src://effect_textures/white");
    //TODOVULKAN auto mtl_load_req2 = std::make_shared<asset::LoadRequest>("src://effect_textures/voltex_pn2");
    _whiteTexture = asset::AssetManager<TextureAsset>::load(mtl_load_req1);
    //TODOVULKAN _voltexA      = asset::AssetManager<TextureAsset>::load(mtl_load_req2);
    //////////////////////////////////////////////////////////////
    // new pipeline stuff.
    //////////////////////////////////////////////////////////////
    auto fxcache = _lightingmtl->pipelineCache();
    FxPipelinePermutation permu;
    permu._forced_technique = false //_enableSDF
                            ? _tekEnvironmentLightingSDF
                            : _tekEnvironmentLighting;

    _pipeline_envlighting_model0_mono = fxcache->findPipeline(permu);

    printf( "SHADER<%s> Load Complete\n", _shadername.c_str() );

  }
  //target->debugPopGroup();
  target->debugPopGroup();
  auto ev = std::make_shared<GpuEvent>();
  ev->_eventID = "ork::lev2::pbr::deferrednode::DeferredContext::gpuInit";
  target->enqueueGpuEvent(ev);
  if(_onGpuInitialized){
    _onGpuInitialized();
  }
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::renderGbuffer(RenderCompositingNode* node, CompositorDrawData& drawdata, const ViewData& VD) {
  return;//
  EASY_BLOCK("renderGbuffer");
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  //auto RSI                     = targ->RSI();
  auto& ddprops                = drawdata._properties;
  auto irenderer               = ddprops["irenderer"_crcu].get<lev2::IRenderer*>();

  ViewportRect tgt_rect(0, 0, _rtgGbuffer->width(), _rtgGbuffer->height());
  ViewportRect mrt_rect(0, 0, _rtgGbuffer->width(), _rtgGbuffer->height());
  ///////////////////////////////////////////////////////////////////////////
  FBI->PushRtGroup(_rtgGbuffer.get());
  //FBI->SetAutoClear(false); // explicit clear
  //targ->beginFrame(false);
  ///////////////////////////////////////////////////////////////////////////
  const auto TOPCPD  = CIMPL->topCPD();
  auto CPD           = TOPCPD;
  CPD.assignLayers(_layername);
  CPD._irendertarget = _rtgGbuffer->_rendertarget.get();
  CPD.SetDstRect(tgt_rect);
  CPD.SetMrtRect(mrt_rect);
  CPD._passID = "defgbuffer1"_crcu;
  ///////////////////////////////////////////////////////////////////////////
  auto DB = RCFD.GetDB();
  if (DB) {
    ///////////////////////////////////////////////////////////////////////////
    // DrawableBuffer -> RenderQueue enqueue
    ///////////////////////////////////////////////////////////////////////////
    for (const auto& layer_name : CPD.getLayerNames()) {
      // printf("Deferred::renderEnqueuedScene::layer<%s>", layer_name.c_str());
      targ->debugMarker(FormatString("Deferred::renderEnqueuedScene::layer<%s>", layer_name.c_str()));
      DB->enqueueLayerToRenderQueue(layer_name, irenderer);
    }
    /////////////////////////////////////////////////
    auto MTXI = targ->MTXI();
    CIMPL->pushCPD(CPD); // drawenq
    targ->debugPushGroup("toolvp::DrawEnqRenderables");
    _rtgGbuffer->_clearColor = fvec4(0, 0, 0, 0);
    FBI->rtGroupClear(_rtgGbuffer.get());
    //auto newmask = RGBAMask{true,true,true,false};
    //auto oldmask = RSI->SetRGBAWriteMask(newmask);
    irenderer->drawEnqueuedRenderables();
    framerenderer.renderMisc();
    //RSI->SetRGBAWriteMask(oldmask);
    targ->debugPopGroup(); // drawenq
    CIMPL->popCPD();
    irenderer->resetQueue();
  }
  /////////////////////////////////////////////////////////////////////////////////////////
  //targ->endFrame();
  FBI->PopRtGroup();
}

///////////////////////////////////////////////////////////////////////////////

const uint32_t* DeferredContext::captureDepthClusters(const CompositorDrawData& drawdata, const ViewData& VD) {
  /*
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  auto this_buf                = FBI->GetThisBuffer();
  auto vprect                  = ViewportRect(0, 0, _clusterW, _clusterH);
  auto quadrect                = SRect(0, 0, _clusterW, _clusterH);
  auto tgt_rect                = targ->mainSurfaceRectAtOrigin();

  const auto TOPCPD = CIMPL->topCPD();
  auto CPD          = TOPCPD;
  CPD.assignLayers(_layername);
  CPD._clearColor   = _clearColor;
  CPD.SetDstRect(tgt_rect);
  CPD._passID = "defcluster"_crcu;
  CPD.SetDstRect(vprect);
  CPD._irendertarget        = _clusterRT;
  CPD._cameraMatrices       = nullptr;
  CPD._stereoCameraMatrices = nullptr;
  CPD._stereo1pass          = false;
  CIMPL->pushCPD(CPD); // findclusters
  targ->debugPushGroup("Deferred::findclusters");
  {
    FBI->SetAutoClear(true);
    FBI->PushRtGroup(_rtgDepthCluster.get());
    targ->beginFrame();
    _lightingmtl->begin(_tekDownsampleDepthCluster, RCFD);
    _lightingmtl->bindParamInt(_parTileDim, KTILEDIMXY);
    _lightingmtl->bindParamCTex(_parMapDepth, _rtgGbuffer->_depthBuffer->_texture.get());
    _lightingmtl->bindParamVec2(_parNearFar, fvec2(VD._near, VD._far));
    _lightingmtl->bindParamVec2(_parZndc2eye, VD._zndc2eye);
    _lightingmtl->bindParamVec2(_parInvViewSize, fvec2(1.0 / float(_width), 1.0f / float(_height)));
    _lightingmtl->_rasterstate->SetBlending(Blending::OFF);
    _lightingmtl->_rasterstate->SetDepthTest(EDepthTest::OFF);
    _lightingmtl->_rasterstate->SetCullTest(ECullTest::OFF);
    _lightingmtl->commit();
    this_buf->Render2dQuadEML(fvec4(-1, -1, 2, 2), fvec4(0, 0, 1, 1), fvec4(0, 0, 0, 0));
    _lightingmtl->end(RCFD);
    targ->endFrame();
    FBI->PopRtGroup();
  }
  targ->debugPopGroup(); // findclusters
  CIMPL->popCPD();       // findclusters
  auto buf0      = _rtgDepthCluster->GetMrt(0);
  bool captureok = FBI->capture(buf0.get(), &_clustercapture);
  assert(captureok);
  return (const uint32_t*)_clustercapture._data;*/
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::renderUpdate(RenderCompositingNode* node, CompositorDrawData& drawdata) {

  _rtgGbuffer = _rtgs_gbuffer->fetch(node->_bufferKey);
  _rtgLbuffer = _rtgs_laccum->fetch(node->_bufferKey);

  auto& ddprops = drawdata._properties;
  //////////////////////////////////////////////////////
  // Resize RenderTargets
  //////////////////////////////////////////////////////
  int newwidth  = ddprops["OutputWidth"_crcu].get<int>();
  int newheight = ddprops["OutputHeight"_crcu].get<int>();
  if (_rtgGbuffer->width() != newwidth or _rtgGbuffer->height() != newheight) {
    printf( "RESIZEDEFCTX\n");
    _width    = newwidth;
    _height   = newheight;
    _clusterW = (newwidth + KTILEDIMXY - 1) / KTILEDIMXY;
    _clusterH = (newheight + KTILEDIMXY - 1) / KTILEDIMXY;
    _rtgGbuffer->Resize(newwidth, newheight);
    _rtgLbuffer->Resize(newwidth, newheight);
    //_rtgDecal->Resize(newwidth, newheight);
    //_rtgDepthCluster->Resize(_clusterW, _clusterH);
  }
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::updateDebugLights(const ViewData& VD) {
  const int KTILEMAXX = _clusterW - 1;
  const int KTILEMAXY = _clusterH - 1;
  //////////////////////////////////////////////////////////////////
  // update pointlights
  //////////////////////////////////////////////////////////////////
  for (auto pl : _pointlights) {
    if (pl->_counter < 1) {
      pl->next();
    } else {
      fvec3 delta = pl->_dst - pl->_pos;
      pl->_pos += delta.normalized() * 2.0f;
      pl->_counter--;
    }
    Sphere sph(pl->_pos, pl->_radius);
    pl->_aabox         = sph.projectedBounds(VD.VPL);
    const auto& boxmin = pl->_aabox.Min();
    const auto& boxmax = pl->_aabox.Max();
    pl->_aamin         = ((boxmin + fvec3(1, 1, 1)) * 0.5);
    pl->_aamax         = ((boxmax + fvec3(1, 1, 1)) * 0.5);
    pl->_minX          = int(floor(pl->_aamin.x * KTILEMAXX));
    pl->_maxX          = int(ceil(pl->_aamax.x * KTILEMAXX));
    pl->_minY          = int(floor(pl->_aamin.y * KTILEMAXY));
    pl->_maxY          = int(ceil(pl->_aamax.y * KTILEMAXY));
    pl->dist2cam       = (pl->_pos - VD._camposmono).magnitude();
    pl->_minZ          = pl->dist2cam - pl->_radius; // Zndc2eye.x / (pl->_aabox.Min().z - Zndc2eye.y);
    pl->_maxZ          = pl->dist2cam + pl->_radius; // Zndc2eye.x / (pl->_aabox.Max().z - Zndc2eye.y);
  }
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::bindViewParams(const ViewData& VD) {
  _lightingmtl->bindParamMatrixArray(_parMatIVPArray, VD._ivp, 2);
  _lightingmtl->bindParamMatrixArray(_parMatVArray, VD._v, 2);
  _lightingmtl->bindParamMatrixArray(_parMatPArray, VD._p, 2);
  _lightingmtl->bindParamVec2(_parZndc2eye, VD._zndc2eye);
  _lightingmtl->bindParamVec2(_parNearFar, fvec2(VD._near, VD._far));
  _lightingmtl->bindParamVec2(_parInvViewSize, fvec2(1.0 / float(_width), 1.0f / float(_height)));
  _lightingmtl->bindParamFloat(_parTime, VD._time);
}

void DeferredContext::bindRasterState(Context* ctx, ECullTest culltest, EDepthTest depthtest, BlendingMacro blending) {
  _lightingmtl->_rasterstate->setBlendingMacro(blending);
  _lightingmtl->_rasterstate->_depthtest = depthtest;
  _lightingmtl->_rasterstate->_culltest = culltest;
  //ctx->RSI()->BindRasterState(_lightingmtl->_rasterstate);
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::renderBaseLighting(RenderCompositingNode* node, CompositorDrawData& drawdata, const ViewData& VD) {
  printf("WTF2\n");

  /////////////////////////////////////////////////////////////////
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto CIMPL                   = drawdata._cimpl;
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  auto this_buf                = FBI->GetThisBuffer();
  //auto RSI                     = targ->RSI();
  auto DWI                     = targ->DWI();
  const auto TOPCPD            = CIMPL->topCPD();
  _accumCPD                    = TOPCPD;
  _decalCPD                    = TOPCPD;
  /////////////////////////////////////////////////////////////////
  auto vprect   = ViewportRect(0, 0, _width, _height);
  auto quadrect = SRect(0, 0, _width, _height);
  _accumCPD.SetDstRect(vprect);
  _accumCPD._irendertarget        = _rtgLbuffer->_rendertarget.get();
  _accumCPD._cameraMatrices       = nullptr;
  _accumCPD._stereoCameraMatrices = nullptr;
  _accumCPD._stereo1pass          = false;
  _decalCPD.SetDstRect(vprect);
  _decalCPD._irendertarget        = nullptr;
  _decalCPD._cameraMatrices       = nullptr;
  _decalCPD._stereoCameraMatrices = nullptr;
  _decalCPD._stereo1pass          = false;
  CIMPL->pushCPD(_accumCPD); // base lighting
  FBI->PushRtGroup(_rtgLbuffer.get());
  //FBI->rtGroupClear(rtg_laccum.get());
  //////////////////////////////////////////////////////////////////
  // base lighting
  //////////////////////////////////////////////////////////////////
  targ->debugPushGroup("Deferred::BaseLighting");
  _lightingmtl->begin(
      VD._isStereo //
          ? _tekBaseLightingStereo
          : _tekBaseLighting,
      RCFD);
  //////////////////////////////////////////////////////
  bindViewParams(VD);
  bindRasterState(targ, ECullTest::OFF, EDepthTest::OFF, BlendingMacro::OFF);
  //////////////////////////////////////////////////////
  _lightingmtl->bindParamCTex(_parMapGBuf, _rtgGbuffer->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapDepth, _rtgGbuffer->_depthBuffer->_texture.get());
  _lightingmtl->commit();
  DWI->quad2DEMLTiled(fvec4(-1, -1, 2, 2), fvec4(0, 0, 1, 1), fvec4(0, 0, 0, 0), 2);
  _lightingmtl->end(RCFD);
  CIMPL->popCPD();       // base lighting
  targ->debugPopGroup(); // BaseLighting
  FBI->PopRtGroup();     // deferredRtg
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::beginPointLighting(RenderCompositingNode* node, CompositorDrawData& drawdata, const ViewData& VD, lev2::Texture* cookietexture) {

  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  auto FXI                     = targ->FXI();
  //auto RSI                     = targ->RSI();
  targ->debugPushGroup("Deferred::PointLighting");
  CIMPL->pushCPD(_accumCPD);
  FBI->PushRtGroup(_rtgLbuffer.get());
  const FxShaderTechnique* tek = nullptr;
  if (VD._isStereo) {
    tek = cookietexture ? _tekPointLightingTexturedStereo : _tekPointLightingUntexturedStereo;
  } else {
    tek = cookietexture ? _tekPointLightingTextured : _tekPointLightingUntextured;
  }
  _lightingmtl->begin(tek, RCFD);
  //////////////////////////////////////////////////////
  bindViewParams(VD);
  bindRasterState(targ, ECullTest::OFF, EDepthTest::OFF, BlendingMacro::ADDITIVE);
  //////////////////////////////////////////////////////
  _lightingmtl->bindParamCTex(_parMapGBuf, _rtgGbuffer->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapDepth, _rtgGbuffer->_depthBuffer->_texture.get());
  //_lightingmtl->bindParamCTex(_parMapDepthCluster, _rtgDepthCluster->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapBrdfIntegration, _brdfIntegrationMap.get());
  ///////////////////////////
  if (cookietexture)
    _lightingmtl->bindParamCTex(_parLightCookieTexture, cookietexture);
  ///////////////////////////
  _lightingmtl->bindParamFloat(_parSpecularLevel, _specularLevel);
  _lightingmtl->bindParamFloat(_parDiffuseLevel, _diffuseLevel);
  //////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::endPointLighting(CompositorDrawData& drawdata, const ViewData& VD) {
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  _lightingmtl->end(RCFD);
  CIMPL->popCPD();       // _accumCPD
  targ->debugPopGroup(); // Deferred::PointLighting
  FBI->PopRtGroup();     // _rtgLaccum
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::beginSpotLighting(RenderCompositingNode* node, CompositorDrawData& drawdata, const ViewData& VD, lev2::Texture* cookietexture) {

  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  auto FXI                     = targ->FXI();
  //auto RSI                     = targ->RSI();
  targ->debugPushGroup("Deferred::PointLighting");
  CIMPL->pushCPD(_accumCPD);
  FBI->PushRtGroup(_rtgLbuffer.get());
  const FxShaderTechnique* tek = nullptr;
  if (VD._isStereo) {
    tek = cookietexture ? _tekSpotLightingTexturedStereo : _tekSpotLightingUntexturedStereo;
  } else {
    tek = cookietexture ? _tekSpotLightingTextured : _tekSpotLightingUntextured;
  }
  _lightingmtl->begin(tek, RCFD);
  //////////////////////////////////////////////////////
  bindViewParams(VD);
  bindRasterState(targ, ECullTest::OFF, EDepthTest::OFF, BlendingMacro::ADDITIVE);
  //////////////////////////////////////////////////////
  _lightingmtl->bindParamCTex(_parMapGBuf, _rtgGbuffer->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapDepth, _rtgGbuffer->_depthBuffer->_texture.get());
  //_lightingmtl->bindParamCTex(_parMapDepthCluster, _rtgDepthCluster->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapBrdfIntegration, _brdfIntegrationMap.get());
  ///////////////////////////
  if (cookietexture)
    _lightingmtl->bindParamCTex(_parLightCookieTexture, cookietexture);
  ///////////////////////////
  _lightingmtl->bindParamFloat(_parSpecularLevel, _specularLevel);
  _lightingmtl->bindParamFloat(_parDiffuseLevel, _diffuseLevel);
  //////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::endSpotLighting(CompositorDrawData& drawdata, const ViewData& VD) {
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  _lightingmtl->end(RCFD);
  CIMPL->popCPD();       // _accumCPD
  targ->debugPopGroup(); // Deferred::PointLighting
  FBI->PopRtGroup();     // _rtgLaccum
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::beginShadowedSpotLighting(RenderCompositingNode* node, CompositorDrawData& drawdata, const ViewData& VD, lev2::Texture* cookietexture) {


  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  auto FXI                     = targ->FXI();
  //auto RSI                     = targ->RSI();
  targ->debugPushGroup("Deferred::PointLighting");
  CIMPL->pushCPD(_accumCPD);
  FBI->PushRtGroup(_rtgLbuffer.get());
  const FxShaderTechnique* tek = nullptr;
  OrkAssert(cookietexture != nullptr);
  tek = VD._isStereo ? _tekSpotLightingTexturedShadowedStereo : _tekSpotLightingTexturedShadowed;
  _lightingmtl->begin(tek, RCFD);
  //////////////////////////////////////////////////////
  bindViewParams(VD);
  bindRasterState(targ, ECullTest::OFF, EDepthTest::OFF, BlendingMacro::ADDITIVE);
  //////////////////////////////////////////////////////
  _lightingmtl->bindParamCTex(_parMapGBuf, _rtgGbuffer->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapDepth, _rtgGbuffer->_depthBuffer->_texture.get());
  //_lightingmtl->bindParamCTex(_parMapDepthCluster, _rtgDepthCluster->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapBrdfIntegration, _brdfIntegrationMap.get());
  ///////////////////////////
  if (cookietexture)
    _lightingmtl->bindParamCTex(_parLightCookieTexture, cookietexture);
  ///////////////////////////
  _lightingmtl->bindParamFloat(_parSpecularLevel, _specularLevel);
  _lightingmtl->bindParamFloat(_parDiffuseLevel, _diffuseLevel);
  //////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::endShadowedSpotLighting(CompositorDrawData& drawdata, const ViewData& VD) {
  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  _lightingmtl->end(RCFD);
  CIMPL->popCPD();       // _accumCPD
  targ->debugPopGroup(); // Deferred::PointLighting
  FBI->PopRtGroup();     // _rtgLaccum
}
///////////////////////////////////////////////////////////////////////////////

void DeferredContext::beginSpotDecaling(RenderCompositingNode* node, CompositorDrawData& drawdata, const ViewData& VD, lev2::Texture* cookietexture) {

  auto CIMPL                   = drawdata._cimpl;
  FrameRenderer& framerenderer = drawdata.mFrameRenderer;
  RenderContextFrameData& RCFD = framerenderer.framedata();
  auto targ                    = drawdata.context();
  auto FBI                     = targ->FBI();
  auto FXI                     = targ->FXI();
  //auto RSI                     = targ->RSI();
  targ->debugPushGroup("Deferred::SpotDecaling");
  CIMPL->pushCPD(_decalCPD);
  FBI->PushRtGroup(_rtgDecal.get());
  const FxShaderTechnique* tek = nullptr;
  if (VD._isStereo) {
    tek = cookietexture ? _tekSpotDecalingTexturedStereo : _tekSpotDecalingTexturedStereo;
  } else {
    tek = cookietexture ? _tekSpotDecalingTextured : _tekSpotDecalingTextured;
  }
  _lightingmtl->begin(tek, RCFD);
  //////////////////////////////////////////////////////
  bindViewParams(VD);
  bindRasterState(targ, ECullTest::OFF, EDepthTest::OFF, BlendingMacro::OFF);
  ///////////////////////////
  _lightingmtl->bindParamCTex(_parMapGBuf, _rtgGbuffer->GetMrt(0)->texture());
  _lightingmtl->bindParamCTex(_parMapDepth, _rtgGbuffer->_depthBuffer->_texture.get());
  if (cookietexture)
    _lightingmtl->bindParamCTex(_parLightCookieTexture, cookietexture);
  //////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void DeferredContext::endSpotDecaling(CompositorDrawData& drawdata, const ViewData& VD) {
  auto CIMPL = drawdata._cimpl;
  auto& RCFD = drawdata.RCFD();
  auto targ  = drawdata.context();
  auto FBI   = targ->FBI();

  _lightingmtl->end(RCFD);
  CIMPL->popCPD();       // _decalCPD
  targ->debugPopGroup(); // Deferred::SpotDecaling
  FBI->PopRtGroup();     // _rtgDecal
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::deferrednode
