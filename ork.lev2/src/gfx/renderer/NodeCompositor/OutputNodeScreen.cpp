////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/lev2/gfx/renderer/NodeCompositor/NodeCompositorScreen.h>

#include <ork/application/application.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/renderer/builtin_frameeffects.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/pch.h>
#include <ork/reflect/properties/registerX.inl>

ImplementReflectionX(ork::lev2::ScreenOutputCompositingNode, "ScreenOutputCompositingNode");

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////
void ScreenOutputCompositingNode::describeX(class_t* c) {
  c->directProperty("Layer", &ScreenOutputCompositingNode::_layername);
  c->directProperty("SuperSample", &ScreenOutputCompositingNode::_supersample)
      ->annotate<ConstString>("editor.range.min", "0")
      ->annotate<ConstString>("editor.range.max", "5");
}
///////////////////////////////////////////////////////////////////////////////
struct SCRIMPL {
  ///////////////////////////////////////
  SCRIMPL(ScreenOutputCompositingNode* node)
      : _node(node)
      , _camname(AddPooledString("Camera"))
      , _layers(AddPooledString("All")) {
      _CPD._debugName = "ScreenOutputCompositingNode";
  }
  ///////////////////////////////////////
  ~SCRIMPL() {
  }
  ///////////////////////////////////////
  void gpuInit(lev2::Context* ctx) {
    if (_needsinit) {
      _blit2screenmtl.gpuInit(ctx, "orkshader://solid");
      _blit2screenmtl._rasterstate->_culltest = ECullTest::OFF;
      _fxtechnique1x1 = _blit2screenmtl.technique("texcolor");
      _fxtechnique2x2 = _blit2screenmtl.technique("downsample_2x2");
      _fxtechnique3x3 = _blit2screenmtl.technique("downsample_3x3");
      _fxtechnique4x4 = _blit2screenmtl.technique("downsample_4x4");
      _fxtechnique5x5 = _blit2screenmtl.technique("downsample_5x5");
      _fxtechnique6x6 = _blit2screenmtl.technique("downsample_6x6");
      _fxpMVP         = _blit2screenmtl.param("MatMVP");
      _fxpColorMap    = _blit2screenmtl.param("ColorMap");
      _needsinit      = false;
      _msaadownsamplebuffer = std::make_shared<RtGroup>(ctx, 8, 8, MsaaSamples::MSAA_1X);
      auto dsbuf        = _msaadownsamplebuffer->createRenderTarget(_node->_format);
      dsbuf->_debugName = "MsaaDownsampleBuffer";

      _subpass_assemble = std::make_shared<RenderSubPass>();
      _subpass_assemble->_debugName = "OCN-SCR-ASSEMBLE";
      _subpass_assemble->_rtg_input = nullptr;
      _subpass_assemble->_rtg_output = nullptr;

      _subpass_composite = std::make_shared<RenderSubPass>();
      _subpass_composite->_debugName = "OCN-SCR-COMPOSITE";
      _subpass_composite->_rtg_input = nullptr;
      _subpass_composite->_rtg_output = nullptr;

      _screen_renderpass = std::make_shared<RenderPass>();
      _screen_renderpass->_immutable = true;
      _screen_renderpass->_debugName = "OCN-SCR-RENDERPASS";
      _screen_renderpass->_subpasses.push_back(_subpass_assemble);
      _screen_renderpass->_subpasses.push_back(_subpass_composite);
    }
  }
  ///////////////////////////////////////
  void beginAssemble(CompositorDrawData& drawdata) {
    auto& ddprops                = drawdata._properties;
    FrameRenderer& framerenderer = drawdata.mFrameRenderer;
    RenderContextFrameData& RCFD = framerenderer.framedata();
    auto CIMPL                   = drawdata._cimpl;
    const auto& CCTX = CIMPL->compositingContext();
    auto DB                      = RCFD.GetDB();
    Context* targ                = drawdata.context();
    int w                        = CCTX.miWidth;
    int h                        = CCTX.miHeight;
    if (targ->hiDPI()) {
      //w /= 2;
      //h /= 2;
    }
    _width  = w * (_node->supersample() + 1);
    _height = h * (_node->supersample() + 1);
    //////////////////////////////////////////////////////
    drawdata._properties["OutputWidth"_crcu].set<int>(_width);
    drawdata._properties["OutputHeight"_crcu].set<int>(_height);
    drawdata._properties["StereoEnable"_crcu].set<bool>(false);
    drawdata._properties["SubPass"_crcu].set<rendersubpass_ptr_t>(_subpass_assemble);
    _CPD.defaultSetup(drawdata);
    CIMPL->pushCPD(_CPD);
    //targ->beginRenderPass(_screen_renderpass);

  }
  void endAssemble(CompositorDrawData& drawdata) {
    auto CIMPL                   = drawdata._cimpl;
    Context* targ                = drawdata.context();
    //targ->endSubPass(_subpass_assemble);
    CIMPL->popCPD();
  }
  ///////////////////////////////////////
  ScreenOutputCompositingNode* _node = nullptr;
  PoolString _camname, _layers;
  CompositingPassData _CPD;
  FreestyleMaterial _blit2screenmtl;
  const FxShaderTechnique* _fxtechnique1x1;
  const FxShaderTechnique* _fxtechnique2x2;
  const FxShaderTechnique* _fxtechnique3x3;
  const FxShaderTechnique* _fxtechnique4x4;
  const FxShaderTechnique* _fxtechnique5x5;
  const FxShaderTechnique* _fxtechnique6x6;
  const FxShaderParam* _fxpMVP;
  const FxShaderParam* _fxpColorMap;
  bool _needsinit = true;
  int _width      = 0;
  int _height     = 0;
  rtgroup_ptr_t _msaadownsamplebuffer;
  renderpass_ptr_t _screen_renderpass;
  rendersubpass_ptr_t _subpass_assemble;
  rendersubpass_ptr_t _subpass_composite;
};
///////////////////////////////////////////////////////////////////////////////
ScreenOutputCompositingNode::ScreenOutputCompositingNode()
    : _supersample(0) {
  _format = EBufferFormat::RGBA8;
  _impl = std::make_shared<SCRIMPL>(this);
}
ScreenOutputCompositingNode::~ScreenOutputCompositingNode() {
}
void ScreenOutputCompositingNode::setSuperSample(int ss) {
  switch(ss){
    case 1:
      _supersample = 1;
      break;
    case 4:
      _supersample = 1;
      break;
    case 9:
      _supersample = 2;
      break;
    case 16:
      _supersample = 3;
      break;
    case 25:
      _supersample = 4;
      break;
    case 36:
      _supersample = 5;
      break;
    default:
      OrkAssert(false);
      break;
  }
}
void ScreenOutputCompositingNode::gpuInit(lev2::Context* pTARG, int iW, int iH) {
  _impl.get<std::shared_ptr<SCRIMPL>>()->gpuInit(pTARG);
}
void ScreenOutputCompositingNode::beginAssemble(CompositorDrawData& drawdata) {
  _impl.get<std::shared_ptr<SCRIMPL>>()->beginAssemble(drawdata);
}
void ScreenOutputCompositingNode::endAssemble(CompositorDrawData& drawdata) {
  _impl.get<std::shared_ptr<SCRIMPL>>()->endAssemble(drawdata);
}
void ScreenOutputCompositingNode::composite(CompositorDrawData& drawdata) {
  drawdata.context()->debugPushGroup("ScreenOutputCompositingNode::composite");
  auto impl = _impl.get<std::shared_ptr<SCRIMPL>>();
  /////////////////////////////////////////////////////////////////////////////
  // VR compositor
  /////////////////////////////////////////////////////////////////////////////
  FrameRenderer& framerenderer      = drawdata.mFrameRenderer;
  RenderContextFrameData& framedata = framerenderer.framedata();
  Context* context                  = framedata.GetTarget();
  auto fbi                          = context->FBI();
  if (auto try_final = drawdata._properties["final_out"_crcu].tryAs<RtBuffer*>()) {
    auto buffer = try_final.value();
    if (buffer) {
      assert(buffer != nullptr);
      auto tex = buffer->texture();
      if (tex) {
        /////////////////////////////////////////////////////////////////////////////
        // be nice and composite to main screen as well...
        /////////////////////////////////////////////////////////////////////////////

        int num_msaa_samples = msaaEnumToInt(tex->_msaa_samples);

        if(num_msaa_samples==1){
          context->debugPushGroup("ScreenCompositingNode::to_screen");
          auto this_buf = context->FBI()->GetThisBuffer();
          auto& mtl     = impl->_blit2screenmtl;
          switch (this->_supersample) {
            case 0:
              drawdata.context()->debugPushGroup("ScreenCompositingNode::to_screen<0>");
              mtl.begin(impl->_fxtechnique1x1, framedata);
              break;
            case 1:
              drawdata.context()->debugPushGroup("ScreenCompositingNode::to_screen<1>");
              mtl.begin(impl->_fxtechnique2x2, framedata);
              break;
            case 2:
              drawdata.context()->debugPushGroup("ScreenCompositingNode::to_screen<2>");
              mtl.begin(impl->_fxtechnique3x3, framedata);
              break;
            case 3:
              drawdata.context()->debugPushGroup("ScreenCompositingNode::to_screen<3>");
              mtl.begin(impl->_fxtechnique4x4, framedata);
              break;
            case 4:
              drawdata.context()->debugPushGroup("ScreenCompositingNode::to_screen<4>");
              mtl.begin(impl->_fxtechnique5x5, framedata);
              break;
            case 5:
              drawdata.context()->debugPushGroup("ScreenCompositingNode::to_screen<5>");
              mtl.begin(impl->_fxtechnique6x6, framedata);
              break;
            default:
              OrkAssert(false);
              break;
          }
          mtl._rasterstate->setBlendingMacro(BlendingMacro::OFF);
          mtl.bindParamCTex(impl->_fxpColorMap, tex);
          mtl.bindParamMatrix(impl->_fxpMVP, fmtx4::Identity());
          ViewportRect extents(0, 0, context->mainSurfaceWidth(), context->mainSurfaceHeight());
          fbi->pushViewport(extents);
          fbi->pushScissor(extents);
          //TODOVULKAN this_buf->Render2dQuadEML(fvec4(-1, -1, 2, 2), fvec4(0, 0, 1, 1), fvec4(0, 0, 1, 1));
          fbi->popViewport();
          fbi->popScissor();
          mtl.end(framedata);
          drawdata.context()->debugPopGroup();
          drawdata.context()->debugPopGroup();
        }
        else{
          auto inp_rtg = drawdata._properties["render_outgroup"_crcu].get<rtgroup_ptr_t>();
          context->FBI()->msaaBlit(inp_rtg,impl->_msaadownsamplebuffer);
          auto this_buf = context->FBI()->GetThisBuffer();
          auto& mtl     = impl->_blit2screenmtl;
          mtl.begin(impl->_fxtechnique1x1, framedata);
          mtl._rasterstate->setBlendingMacro(BlendingMacro::OFF);
          tex = impl->_msaadownsamplebuffer->GetMrt(0)->texture();
          mtl.bindParamCTex(impl->_fxpColorMap, tex);
          mtl.bindParamMatrix(impl->_fxpMVP, fmtx4::Identity());
          ViewportRect extents(0, 0, context->mainSurfaceWidth(), context->mainSurfaceHeight());
          fbi->pushViewport(extents);
          fbi->pushScissor(extents);
          this_buf->Render2dQuadEML(fvec4(-1, -1, 2, 2), fvec4(0, 0, 1, 1), fvec4(0, 0, 1, 1));
          fbi->popViewport();
          fbi->popScissor();
          mtl.end(framedata);
        }

      }
    }
  }
  context->debugPopGroup();
  //context->endRenderPass(_screen_renderpass);
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
