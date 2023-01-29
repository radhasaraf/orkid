////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include "NodeCompositor.h"
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/kernel/varmap.inl>
#include "pbr_common.h"

namespace ork::lev2::pbr::deferrednode {

class DeferredCompositingNode;

///////////////////////////////////////////////////////////////////////////////

struct DeferredContext {

#if defined(ENABLE_COMPUTE_SHADERS)
  static constexpr int KTILEDIMXY = 64;
#else
  static constexpr int KTILEDIMXY = 64;
#endif
  //static constexpr float KNEAR = 0.1f;
  //static constexpr float KFAR  = 100000.0f;
  ////////////////////////////////////////////////////////////////////
  DeferredContext(RenderCompositingNode* node, std::string shadername, int numlights);
  ~DeferredContext();
  ////////////////////////////////////////////////////////////////////
  void updateDebugLights(const ViewData& VD);
  ////////////////////////////////////////////////////////////////////
  void gpuInit(Context* target);
  const uint32_t* captureDepthClusters(const CompositorDrawData& drawdata, const ViewData& VD);
  void renderUpdate(CompositorDrawData& drawdata);
  void renderGbuffer(CompositorDrawData& drawdata, const ViewData& VD);
  void renderBaseLighting(CompositorDrawData& drawdata, const ViewData& VD);
  ////////////////////////////////////////////////////////////////////
  void beginPointLighting(CompositorDrawData& drawdata, const ViewData& VD, Texture* cookietexture);
  void endPointLighting(CompositorDrawData& drawdata, const ViewData& VD);
  ////////////////////////////////////////////////////////////////////
  void beginSpotLighting(CompositorDrawData& drawdata, const ViewData& VD, Texture* cookietexture);
  void endSpotLighting(CompositorDrawData& drawdata, const ViewData& VD);
  ////////////////////////////////////////////////////////////////////
  void beginShadowedSpotLighting(CompositorDrawData& drawdata, const ViewData& VD, Texture* cookietexture);
  void endShadowedSpotLighting(CompositorDrawData& drawdata, const ViewData& VD);
  ////////////////////////////////////////////////////////////////////
  void beginSpotDecaling(CompositorDrawData& drawdata, const ViewData& VD, Texture* cookietexture);
  void endSpotDecaling(CompositorDrawData& drawdata, const ViewData& VD);
  ////////////////////////////////////////////////////////////////////
  void bindViewParams(const ViewData& VD);
  void bindRasterState(Context* ctx, ECullTest culltest, EDepthTest depthtest, Blending blending);
  ////////////////////////////////////////////////////////////////////
  RenderCompositingNode* _node;
  FreestyleMaterial _lightingmtl;
  CompositingPassData _accumCPD;
  CompositingPassData _decalCPD;
  fvec4 _clearColor;
  std::string _shadername;
  lev2::texture_ptr_t brdfIntegrationTexture() const;
  ////////////////////////////////////////////////////////////////////
  int _width    = 0;
  int _height   = 0;
  int _clusterW = 0;
  int _clusterH = 0;
  textureassetptr_t _whiteTexture;
  textureassetptr_t _voltexA;
  ////////////////////////////////////////////////////////////////////
  std::vector<PointLight*> _pointlights;

  ////////////////////////////////////////////////////////////////////

  const FxShaderTechnique* _tekBaseLighting              = nullptr;

  const FxShaderTechnique* _tekEnvironmentLighting       = nullptr;
  const FxShaderTechnique* _tekEnvironmentLightingStereo = nullptr;

  const FxShaderTechnique* _tekEnvironmentLightingSDF       = nullptr;
  const FxShaderTechnique* _tekEnvironmentLightingSDFStereo = nullptr;

  const FxShaderTechnique* _tekBaseLightingStereo        = nullptr;
  const FxShaderTechnique* _tekDownsampleDepthCluster    = nullptr;
  //
  const FxShaderTechnique* _tekPointLightingUntextured       = nullptr;
  const FxShaderTechnique* _tekPointLightingTextured         = nullptr;
  const FxShaderTechnique* _tekPointLightingUntexturedStereo = nullptr;
  const FxShaderTechnique* _tekPointLightingTexturedStereo   = nullptr;
  //
  const FxShaderTechnique* _tekSpotLightingUntextured             = nullptr;
  const FxShaderTechnique* _tekSpotLightingTextured               = nullptr;
  const FxShaderTechnique* _tekSpotLightingUntexturedStereo       = nullptr;
  const FxShaderTechnique* _tekSpotLightingTexturedStereo         = nullptr;
  const FxShaderTechnique* _tekSpotLightingTexturedShadowed       = nullptr;
  const FxShaderTechnique* _tekSpotLightingTexturedShadowedStereo = nullptr;
  //
  const FxShaderTechnique* _tekSpotDecalingTextured       = nullptr;
  const FxShaderTechnique* _tekSpotDecalingTexturedStereo = nullptr;
  //

#if defined(ENABLE_COMPUTE_SHADERS)
  FxComputeShader* _lightcollectcomputeshader = nullptr;
#endif

  const FxShaderParam* _parMatIVPArray = nullptr;
  const FxShaderParam* _parMatPArray   = nullptr;
  const FxShaderParam* _parMatVArray   = nullptr;
  const FxShaderParam* _parZndc2eye    = nullptr;
  const FxShaderParam* _parMapGBuf     = nullptr;
  // const FxShaderParam* _parMapGBufAlbAo        = nullptr;
  // const FxShaderParam* _parMapGBufNrmL         = nullptr;
  // const FxShaderParam* _parMapGBufRufMtlAlpha  = nullptr;
  const FxShaderParam* _parMapDepth            = nullptr;
  const FxShaderParam* _parMapDepthCluster     = nullptr;
  const FxShaderParam* _parMapShadowDepth      = nullptr;
  const FxShaderParam* _parMapSpecularEnv      = nullptr;
  const FxShaderParam* _parMapDiffuseEnv       = nullptr;
  const FxShaderParam* _parMapBrdfIntegration  = nullptr;
  const FxShaderParam* _parMapVolTexA  = nullptr;
  const FxShaderParam* _parTime                = nullptr;
  const FxShaderParam* _parNearFar             = nullptr;
  const FxShaderParam* _parInvViewSize         = nullptr;
  const FxShaderParam* _parInvVpDim            = nullptr;
  const FxShaderParam* _parNumLights           = nullptr;
  const FxShaderParam* _parTileDim             = nullptr;
  const FxShaderParam* _parSpecularLevel       = nullptr;
  const FxShaderParam* _parSpecularMipBias     = nullptr;
  const FxShaderParam* _parDiffuseLevel        = nullptr;
  const FxShaderParam* _parAmbientLevel        = nullptr;
  const FxShaderParam* _parSkyboxLevel         = nullptr;
  const FxShaderParam* _parEnvironmentMipBias  = nullptr;
  const FxShaderParam* _parEnvironmentMipScale = nullptr;
  const FxShaderParam* _parDepthFogDistance    = nullptr;
  const FxShaderParam* _parDepthFogPower       = nullptr;
  const FxShaderParamBlock* _lightblock        = nullptr;
  const FxShaderParam* _parLightCookieTexture  = nullptr;
  const FxShaderParam* _parShadowParams        = nullptr;

  ////////////////////////////////////////////////////////////////////

  RtGroupRenderTarget* _accumRT      = nullptr;
  RtGroupRenderTarget* _gbuffRT      = nullptr;
  RtGroupRenderTarget* _decalRT      = nullptr;
  RtGroupRenderTarget* _clusterRT    = nullptr;
  lev2::texture_ptr_t _brdfIntegrationMap = nullptr;

  CaptureBuffer _clustercapture;
  rtgroup_ptr_t _rtgGbuffer      = nullptr;
  rtgroup_ptr_t _rtgDecal        = nullptr;
  rtgroup_ptr_t _rtgDepthCluster = nullptr;
  rtgroup_ptr_t _rtgLaccum       = nullptr;

  rtbuffer_ptr_t _rtbGbuffer      = nullptr;
  rtbuffer_ptr_t _rtbDepthCluster = nullptr;
  rtbuffer_ptr_t _rtbLightAccum   = nullptr;

  std::string _layername;
  float _specularLevel    = 1.0f;
  float _diffuseLevel     = 1.0f;
  float _depthFogPower    = 1.0f;
  float _depthFogDistance = 1.0f;
};

///////////////////////////////////////////////////////////////////////////////

class DeferredCompositingNode : public RenderCompositingNode {
  DeclareConcreteX(DeferredCompositingNode, RenderCompositingNode);

public:
  DeferredCompositingNode();
  ~DeferredCompositingNode();
  fvec4 _clearColor;
  fvec4 _fogColor;

private:
  void doGpuInit(lev2::Context* pTARG, int w, int h) final;
  void DoRender(CompositorDrawData& drawdata) final;

  lev2::rtbuffer_ptr_t GetOutput() const final;
  svar256_t _impl;
};

///////////////////////////////////////////////////////////////////////////////

struct DeferredCompositingNodePbr : public RenderCompositingNode {
  DeclareConcreteX(DeferredCompositingNodePbr, RenderCompositingNode);

public:
  DeferredCompositingNodePbr();
  ~DeferredCompositingNodePbr();

  void doGpuInit(lev2::Context* pTARG, int w, int h) final;
  void DoRender(CompositorDrawData& drawdata) final;

  lev2::rtbuffer_ptr_t GetOutput() const final;
  lev2::rtgroup_ptr_t GetOutputGroup() const final;

  svar256_t _impl;

  pbr::commonstuff_ptr_t _pbrcommon;

};

///////////////////////////////////////////////////////////////////////////////

#if defined(ENABLE_NVMESH_SHADERS)

class DeferredCompositingNodeNvMs : public RenderCompositingNode {
  DeclareConcreteX(DeferredCompositingNodeNvMs, RenderCompositingNode);

public:
  DeferredCompositingNodeNvMs();
  ~DeferredCompositingNodeNvMs();
  fvec4 _clearColor;
  fvec4 _fogColor;

private:
  void doGpuInit(lev2::Context* pTARG, int w, int h) final;
  void DoRender(CompositorDrawData& drawdata) final;

  lev2::rtbuffer_ptr_t GetOutput() const final;
  svar256_t _impl;
};

#endif

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::deferrednode
