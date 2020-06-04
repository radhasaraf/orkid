#include <ork/lev2/gfx/scenegraph/scenegraph.h>
#include <ork/application/application.h>
#include <ork/kernel/opq.h>
#include <ork/lev2/gfx/renderer/NodeCompositor/OutputNodeRtGroup.h>
#include <ork/lev2/gfx/renderer/NodeCompositor/NodeCompositorPicking.h>
using namespace std::string_literals;
using namespace ork;

const int PICKBUFDIM = 17;

namespace ork::lev2::scenegraph {

PickBuffer::PickBuffer(ork::lev2::Context* ctx, Scene& scene)
    : _context(ctx)
    , _scene(scene) {
  _pick_mvp_matrix           = std::make_shared<fmtx4>();
  _pixelfetchctx._gfxContext = ctx;
  _pixelfetchctx.miMrtMask   = 3;
  _pixelfetchctx.mUsage[0]   = lev2::PixelFetchContext::EPU_PTR64;
  _pixelfetchctx.mUsage[1]   = lev2::PixelFetchContext::EPU_FLOAT;
}
///////////////////////////////////////////////////////////////////////////
uint64_t PickBuffer::pickWithScreenCoord(cameradata_ptr_t cam, fvec2 screencoord) {
  auto FBI = _context->FBI();
  int W    = _context->mainSurfaceWidth();
  int H    = _context->mainSurfaceHeight();
  float fx = float(screencoord.x) / W;
  float fy = float(screencoord.y) / H;
  fvec2 unitpos(fx, fy);
  auto mtcs = cam->computeMatrices(float(W) / float(H));
  auto ray  = std::make_shared<fray3>();
  mtcs.projectDepthRay(unitpos, *ray.get());
  auto o       = ray->mOrigin;
  auto d       = ray->mDirection;
  uint64_t val = pickWithRay(ray);
  printf("unitpos<%g %g>\n", fx, fy);
  printf("ori <%g %g %g>\n", o.x, o.y, o.z);
  printf("dir <%g %g %g>\n", d.x, d.y, d.z);
  printf("val <0x%zu>\n", val);

  return val;
}
///////////////////////////////////////////////////////////////////////////
uint64_t PickBuffer::pickWithRay(fray3_constptr_t ray) {
  _camdat.Persp(0.01, 1000, 0.1);

  auto perp = ray->mDirection.Cross(fvec3(0, 1, 0));
  perp      = ray->mDirection.Cross(perp);

  _camdat.Lookat(
      ray->mOrigin, //
      ray->mOrigin + ray->mDirection,
      perp);
  mydraw(ray);
  auto p = _pixelfetchctx.GetPointer(0);
  return uint64_t(p);
}
///////////////////////////////////////////////////////////////////////////
void PickBuffer::mydraw(fray3_constptr_t ray) {
  ork::opq::assertOnQueue2(opq::mainSerialQueue());
  _context->makeCurrentContext();
  auto FBI = _context->FBI();
  ///////////////////////////////////////////////////////////////////////////
  if (nullptr == _compdata) {
    _compdata = new CompositingData;
    _compdata->presetPicking();

    auto csi     = _compdata->findScene("scene1"_pool);
    auto itm     = csi->findItem("item1"_pool);
    auto tek     = dynamic_cast<NodeCompositingTechnique*>(itm->technique());
    auto rtgnode = dynamic_cast<RtGroupOutputCompositingNode*>(tek->_outputNode);
    auto piknode = dynamic_cast<PickingCompositingNode*>(tek->_renderNode);
    rtgnode->resize(PICKBUFDIM, PICKBUFDIM);
    piknode->gpuInit(_context, PICKBUFDIM, PICKBUFDIM);
    _pixelfetchctx.mRtGroup = piknode->GetOutputGroup();
    _compimpl               = _compdata->createImpl();
  }
  ///////////////////////////////////////////////////////////////////////////
  ork::lev2::RenderContextFrameData RCFD(_context); //
  RCFD._cimpl = _compimpl;
  _pixelfetchctx.mUserData.Set<ork::lev2::RenderContextFrameData*>(&RCFD);
  ///////////////////////////////////////////////////////////////////////////

  // mPickIds.clear();

  ork::recursive_mutex& glock = lev2::GfxEnv::GetRef().GetGlobalLock();
  glock.Lock(0x777);
  _context->pushRenderContextFrameData(&RCFD);
  ViewportRect tgt_rect(0, 0, PICKBUFDIM, PICKBUFDIM);
  ///////////////////////////////////////////////////////////////////////////
  // auto irenderer = _scenevp->GetRenderer();
  // irenderer->setContext(_context);
  RCFD.SetLightManager(nullptr);
  ///////////////////////////////////////////////////////////////////////////
  auto DB = DrawableBuffer::acquireForRead(0x1234);
  if (DB) {

    /////////////////////////////////////////////////////////////
    // since we have a pick ray
    //  just point the camera along the ray
    //  and the pixel of interest will be at the center
    //  of the rendered buffer
    /////////////////////////////////////////////////////////////

    auto mtcs                 = _camdat.computeMatrices(1.0);
    fmtx4 P                   = mtcs.GetPMatrix();
    fmtx4 V                   = mtcs.GetVMatrix();
    (*_pick_mvp_matrix.get()) = V * P;
    auto screen_coordinate    = fvec4(0.5, 0.5, 0, 0);
    /////////////////////////////////////////////////////////////

    lev2::UiViewportRenderTarget rt(nullptr);
    RCFD.setUserProperty("DB"_crc, lev2::rendervar_t(DB));
    RCFD.setUserProperty("pickbufferMvpMatrix"_crc, _pick_mvp_matrix);
    lev2::CompositingPassData CPD;
    CPD.AddLayer("All");
    CPD.SetDstRect(tgt_rect);
    CPD._ispicking     = true;
    CPD._irendertarget = &rt;
    ///////////////////////////////////////////////////////////////////////////
    lev2::FrameRenderer framerenderer(RCFD, [&]() {});
    lev2::CompositorDrawData drawdata(framerenderer);
    drawdata._cimpl = _compimpl;
    drawdata._properties["StereoEnable"_crcu].Set<bool>(false);
    drawdata._properties["primarycamindex"_crcu].Set<int>(0);
    drawdata._properties["cullcamindex"_crcu].Set<int>(0);
    drawdata._properties["irenderer"_crcu].Set<lev2::IRenderer*>(&_scene._renderer);
    drawdata._properties["simrunning"_crcu].Set<bool>(true);
    drawdata._properties["DB"_crcu].Set<const DrawableBuffer*>(DB);
    ///////////////////////////////////////////////////////////////////////////
    // draw the pickbuffer
    ///////////////////////////////////////////////////////////////////////////
    _compimpl->pushCPD(CPD);
    FBI->EnterPickState(nullptr);
    _compimpl->assemble(drawdata);
    DrawableBuffer::releaseFromRead(DB);
    FBI->LeavePickState();
    _compimpl->popCPD();
    ///////////////////////////////////////////??
    // fetch the pixel, yo.
    ///////////////////////////////////////////??
    FBI->GetPixel(screen_coordinate, _pixelfetchctx);
    ///////////////////////////////////////////??

  } // if(DB)
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  _context->popRenderContextFrameData();
  lev2::GfxEnv::GetRef().GetGlobalLock().UnLock();
  ///////////////////////////////////////////////////////////////////////////}
}
} // namespace ork::lev2::scenegraph
