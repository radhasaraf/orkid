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

namespace ork::lev2 {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CallbackDrawable::CallbackDrawable(DrawableOwner* pent)
    : Drawable()
    , mSortKey(4)
    , mRenderCallback(0)
    , _enqueueOnLayerCallback(0)
    , mDataDestroyer(0) {
}
///////////////////////////////////////////////////////////////////////////////
CallbackDrawable::~CallbackDrawable() {
  if (mDataDestroyer) {
    mDataDestroyer->Destroy();
  }
  mDataDestroyer          = 0;
  _enqueueOnLayerCallback = 0;
  mRenderCallback         = 0;
}
///////////////////////////////////////////////////////////////////////////////
// Multithreaded Renderer DB
///////////////////////////////////////////////////////////////////////////////
void CallbackDrawable::enqueueOnLayer(const DrawQueueXfData& xfdata, DrawableBufLayer& buffer) const {
  // ork::opq::assertOnQueue2(opq::updateSerialQueue());

  DrawableBufItem& cdb = buffer.Queue(xfdata, this);
  cdb.mUserData0       = GetUserDataA();
  if (_enqueueOnLayerCallback) {
    _enqueueOnLayerCallback(cdb);
  }
  if (_enqueueOnLayerLambda) {
    _enqueueOnLayerLambda(cdb);
  }
}
///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
void CallbackDrawable::enqueueToRenderQueue(const DrawableBufItem& item, lev2::IRenderer* renderer) const {
  ork::opq::assertOnQueue2(opq::mainSerialQueue());

  lev2::CallbackRenderable& renderable = renderer->enqueueCallback();
  renderable.SetMatrix(item.mXfData.mWorldMatrix);
  renderable.SetObject(GetOwner());
  renderable.SetRenderCallback(mRenderCallback);
  renderable.SetSortKey(mSortKey);
  renderable.SetDrawableDataA(GetUserDataA());
  renderable.SetDrawableDataB(GetUserDataB());
  renderable.SetUserData0(item.mUserData0);
  renderable.SetUserData1(item.mUserData1);
  renderable.SetModColor(renderer->GetTarget()->RefModColor());
}

///////////////////////////////////////////////////////////////////////////////

CallbackRenderable::CallbackRenderable(IRenderer* renderer)
    : IRenderable()
    , mSortKey(0)
    , mMaterialIndex(0)
    , mMaterialPassIndex(0)
    , mUserData0()
    , mUserData1()
    , mRenderCallback(0) {
}

void CallbackRenderable::Render(const IRenderer* renderer) const {
  renderer->RenderCallback(*this);
}

void CallbackRenderable::SetSortKey(uint32_t skey) {
  mSortKey = skey;
}

void CallbackRenderable::SetUserData0(IRenderable::var_t pdata) {
  mUserData0 = pdata;
}
const IRenderable::var_t& CallbackRenderable::GetUserData0() const {
  return mUserData0;
}
void CallbackRenderable::SetUserData1(IRenderable::var_t pdata) {
  mUserData1 = pdata;
}
const IRenderable::var_t& CallbackRenderable::GetUserData1() const {
  return mUserData1;
}

void CallbackRenderable::SetRenderCallback(cbtype_t cb) {
  mRenderCallback = cb;
}
CallbackRenderable::cbtype_t CallbackRenderable::GetRenderCallback() const {
  return mRenderCallback;
}
uint32_t CallbackRenderable::ComposeSortKey(const IRenderer* renderer) const {
  return mSortKey;
}

} // namespace ork::lev2
