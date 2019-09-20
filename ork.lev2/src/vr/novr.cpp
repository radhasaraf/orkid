#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/glheaders.h> // todo abstract somehow ?
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/vr/vr.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#if !defined(ENABLE_OPENVR)
////////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::orkidvr {
////////////////////////////////////////////////////////////////////////////////

NoVrDevice::NoVrDevice() {
  auto handgroup = lev2::InputManager::inputGroup("hands");
  _qtmousesubsc  = msgrouter::channel("qtmousepos")->subscribe([this](msgrouter::content_t c) { _qtmousepos = c.Get<fvec2>(); });

  _qtkbdownsubs = msgrouter::channel("qtkeyboard.down")->subscribe([this, handgroup](msgrouter::content_t c) {
    int key = c.Get<int>();
    switch (key) {
      case 'w':
        handgroup->setChannel("left.trigger").as<bool>(true);
        break;
      case 'a':
        handgroup->setChannel("left.thumb").as<bool>(true);
        break;
      case 's':
        break;
      case 'd':
        handgroup->setChannel("right.thumb").as<bool>(true);
        break;
    }
  });
  _qtkbupsubs   = msgrouter::channel("qtkeyboard.up")->subscribe([this, handgroup](msgrouter::content_t c) {
    int key = c.Get<int>();
    switch (key) {
      case 'w':
        handgroup->setChannel("left.trigger").as<bool>(false);
        break;
      case 'a':
        handgroup->setChannel("left.thumb").as<bool>(false);
        break;
      case 's':
        break;
      case 'd':
        handgroup->setChannel("right.thumb").as<bool>(false);
        break;
    }
  });

  _posemap["projl"].Perspective(45, 16.0 / 9.0, .1, 100000);
  _posemap["projr"].Perspective(45, 16.0 / 9.0, .1, 100000);
  _posemap["eyel"] = fmtx4::Identity;
  _posemap["eyer"] = fmtx4::Identity;
}
NoVrDevice::~NoVrDevice() {}
////////////////////////////////////////////////////////////////////////////////
void NoVrDevice::_updatePoses(fmtx4 observermatrix) {
  auto mpos = _qtmousepos;
  float r   = mpos.Mag();
  float z   = 1.0f - r;
  auto v3   = fvec3(-mpos.x, -mpos.y, z).Normal();
  fmtx4 w;
  w.LookAt(fvec3(0, 0, 0), v3, fvec3(0, 1, 0));
  _posemap["hmd"] = w;
  // printf("v3<%g %g %g>\n", v3.x, v3.y, v3.z);
  _updatePosesCommon(observermatrix);
}
////////////////////////////////////////////////////////////////////////////////
void NoVrDevice::_processControllerEvents() {
  auto handgroup = lev2::InputManager::inputGroup("hands");
  bool curthumbL = handgroup->tryAs<bool>("left.thumb").value();
  bool curthumbR = handgroup->tryAs<bool>("right.thumb").value();
  ///////////////////////////////////////////////////////////
  // turn left,right ( we rotate in discrete steps here, because it causes eye strain otherwise)
  ///////////////////////////////////////////////////////////

  if (curthumbL and false == _prevthumbL) {

    fquat q;
    q.FromAxisAngle(fvec4(0, 1, 0, PI / 12.0));
    _headingmatrix = _headingmatrix * q.ToMatrix();
  } else if (curthumbR and false == _prevthumbR) {
    fquat q;
    q.FromAxisAngle(fvec4(0, 1, 0, -PI / 12.0));
    _headingmatrix = _headingmatrix * q.ToMatrix();
  }
  _prevthumbL = curthumbL;
  _prevthumbR = curthumbR;
}
////////////////////////////////////////////////////////////////////////////////
NoVrDevice& concrete_get() {
  static NoVrDevice _device;
  return _device;
}
Device& device() {
  return concrete_get();
}
////////////////////////////////////////////////////////////////////////////////
void gpuUpdate(fmtx4 observermatrix) {
  auto& mgr = concrete_get();
  mgr._processControllerEvents();
  mgr._updatePoses(observermatrix);
}

void composite(GfxTarget* targ, Texture* twoeyetex) {
  auto& mgr = concrete_get();
   auto fbi = targ->FBI();
   auto buf = fbi->GetThisBuffer();
   int w = targ->GetW();
   int h = targ->GetH();
   SRect vprect(0, 0, w - 1, h - 1);
   SRect quadrect(0, h - 1, w - 1, 0);
  if( false == mgr._private.IsA<CompositingMaterial>() ){
    auto& mtl = mgr._private.Make<CompositingMaterial>();
    mtl.Init(targ);
  }
  auto& mtl = mgr._private.Get<CompositingMaterial>();
   mtl.SetTextureA(twoeyetex);
   mtl.SetLevelA(fvec4::White());
   mtl.SetLevelB(fvec4::Black());
   mtl.SetLevelC(fvec4::Black());
   mtl.SetTechnique("Asolo");
   // TODO - apply post
   buf->RenderMatOrthoQuad(vprect, quadrect, &mtl, 0.0f, 0.0f, 1.0f, 1.0f, 0, fvec4::White());


}
////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::orkidvr
////////////////////////////////////////////////////////////////////////////////
#endif // #if !defined(ENABLE_VR)
