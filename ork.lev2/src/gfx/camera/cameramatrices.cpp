////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/lev2/gfx/camera/cameradata.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/pch.h>

namespace ork::lev2 {

void CameraMatrices::setCustomView(const ork::fmtx4& view) {
  _vmatrix = view;
  // view.dump("setview");
  _explicitViewMatrix = true;
}

void CameraMatrices::setCustomProjection(const ork::fmtx4& proj) {
  // proj.dump("setproj");
  _pmatrix                  = proj;
  _explicitProjectionMatrix = true;
  _aspectRatio              = 1.0f;
  // mNear                     = .1f;
  // mFar                      = 1000.f;
  // mAper                     = 60;
}

////////////////////////////////////////////////////////////////////////////////

void CameraMatrices::projectDepthRay(const fvec2& v2d, fvec3& vdir, fvec3& vori) const {
  fvec3 near_xt_lerp;
  near_xt_lerp.lerp(_frustum.mNearCorners[0], _frustum.mNearCorners[1], v2d.x);
  fvec3 near_xb_lerp;
  near_xb_lerp.lerp(_frustum.mNearCorners[3], _frustum.mNearCorners[2], v2d.x);
  fvec3 near_lerp;
  near_lerp.lerp(near_xt_lerp, near_xb_lerp, v2d.y);
  fvec3 far_xt_lerp;
  far_xt_lerp.lerp(_frustum.mFarCorners[0], _frustum.mFarCorners[1], v2d.x);
  fvec3 far_xb_lerp;
  far_xb_lerp.lerp(_frustum.mFarCorners[3], _frustum.mFarCorners[2], v2d.x);
  fvec3 far_lerp;
  far_lerp.lerp(far_xt_lerp, far_xb_lerp, v2d.y);
  vdir = (far_lerp - near_lerp).normalized();
  vori = near_lerp;
}

////////////////////////////////////////////////////////////////////////////////

void CameraMatrices::projectDepthRay(const fvec2& v2d, fray3& ray_out) const {
  fvec3 dir, ori;
  projectDepthRay(v2d, dir, ori);
  ray_out = fray3(ori, dir);
}

////////////////////////////////////////////////////////////////////////////////

void CameraMatrices::GetPixelLengthVectors(const fvec3& Pos, const fvec2& vp, fvec3& OutX, fvec3& OutY) const {
  /////////////////////////////////////////////////////////////////
  int ivpw = int(vp.x);
  int ivph = int(vp.y);
  /////////////////////////////////////////////////////////////////
  fvec4 va    = Pos;
  fvec4 va_xf = va.transform(_vpmatrix);
  va_xf.perspectiveDivideInPlace();
  va_xf = va_xf * fvec4(vp.x, vp.y, 0.0f);
  /////////////////////////////////////////////////////////////////
  fvec4 vdx    = Pos + _camdat.xNormal();
  fvec4 vdx_xf = vdx.transform(_vpmatrix);
  vdx_xf.perspectiveDivideInPlace();
  vdx_xf     = vdx_xf * fvec4(vp.x, vp.y, 0.0f);
  float MagX = (vdx_xf - va_xf).magnitude(); // magnitude in pixels of mBillboardRight
  /////////////////////////////////////////////////////////////////
  fvec4 vdy    = Pos + _camdat.yNormal();
  fvec4 vdy_xf = vdy.transform(_vpmatrix);
  vdy_xf.perspectiveDivideInPlace();
  vdy_xf     = vdy_xf * fvec4(vp.x, vp.y, 0.0f);
  float MagY = (vdy_xf - va_xf).magnitude(); // magnitude in pixels of mBillboardUp
  /////////////////////////////////////////////////////////////////
  OutX = _camdat.xNormal() * (2.0f / MagX);
  OutY = _camdat.yNormal() * (2.0f / MagY);
  /////////////////////////////////////////////////////////////////
}

} // namespace ork::lev2
