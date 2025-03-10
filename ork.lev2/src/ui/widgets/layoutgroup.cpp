////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/ui/event.h>
#include <ork/lev2/ui/layoutgroup.inl>

/////////////////////////////////////////////////////////////////////////
namespace ork::ui {
/////////////////////////////////////////////////////////////////////////
LayoutGroup::LayoutGroup(const std::string& name, int x, int y, int w, int h)
    : Group(name, x, y, w, h) {
  _layout = std::make_shared<anchor::Layout>(this);
  _evrouter = [this](ui::event_constptr_t ev) -> ui::Widget* { //
    return doRouteUiEvent(ev);
  };
  _evhandler = [this](ui::event_constptr_t ev) -> ui::HandlerResult { //
    ui::HandlerResult result;
    bool was_handled = false;
    switch (ev->_eventcode) {
      case ui::EventCode::PUSH: 
      case ui::EventCode::RELEASE: 
      case ui::EventCode::BEGIN_DRAG: 
      case ui::EventCode::END_DRAG: 
      case ui::EventCode::DRAG: {
        result = LayoutGroup::OnUiEvent(ev);
        was_handled = (result.mHandler!=nullptr);
        break;
      }
    }
    if(was_handled)
      result.setHandled(this);
    return result;
  };
}
/////////////////////////////////////////////////////////////////////////
LayoutGroup::~LayoutGroup() {
}
/////////////////////////////////////////////////////////////////////////
void LayoutGroup::_doOnResized() {
  _clear = true;
}
/////////////////////////////////////////////////////////////////////////
void LayoutGroup::DoLayout() {
  // in this case, the layout is responsible
  // for laying out all children, recursively..
  // note that the layout will use the geometry of this group
  //  to compute the layout of all children
  // So it is expected that you set the size of this group
  //  either manually or driven indirectly through the resize
  //  of a parent..
  const auto& g = _geometry;
  if (0)
    printf(
        "LayoutGroup<%s>::DoLayout l<%p> x<%d> y<%d> w<%d> h<%d>\n", //
        _name.c_str(),
        (void*)_layout.get(),
        g._x,
        g._y,
        g._w,
        g._h);
  if (_layout)
    _layout->updateAll();
  //
}
/////////////////////////////////////////////////////////////////////////
void LayoutGroup::DoDraw(drawevent_constptr_t drwev) {
  if (_clear) {
    auto context = drwev->GetTarget();
    auto FBI     = context->FBI();
    lev2::ViewportRect vrect;
    vrect._x = x();
    vrect._y = y();
    vrect._w = width();
    vrect._h = height();
    FBI->pushScissor(vrect);
    FBI->pushViewport(vrect);
    //FBI->Clear(_clearColor, 1);
    FBI->popViewport();
    FBI->popScissor();
    //_clear = false;
  }
  drawChildren(drwev);
}
//////////////////////////////////////
anchor::layout_ptr_t LayoutGroup::layoutAndAddChild(widget_ptr_t w) {
  auto layout = _layout->childLayout(w.get());
  addChild(w);
  return layout;
}
//////////////////////////////////////
void LayoutGroup::removeChild(anchor::layout_ptr_t ch) {
  _layout->removeChild(ch);
  Group::removeChild(ch->_widget);
}
//////////////////////////////////////
void LayoutGroup::replaceChild(anchor::layout_ptr_t ch, layoutitem_ptr_t rep) {
  _layout->removeChild(rep->_layout);
  Group::removeChild(ch->_widget);
  Group::addChild(rep->_widget);
  ch->_widget  = rep->_widget.get();
  rep->_layout = ch;
}
//////////////////////////////////////
void LayoutGroup::setClearColor(fvec4 clr) {
  _clearColor = clr;
}
//////////////////////////////////////
fvec4 LayoutGroup::clearColor() const {
  return _clearColor;
}
//////////////////////////////////////
const std::set<uiguide_ptr_t>& LayoutGroup::horizontalGuides() const {
  return _hguides;
}
//////////////////////////////////////
const std::set<uiguide_ptr_t>& LayoutGroup::verticalGuides() const {
  return _vguides;
}
namespace anchor{
  std::pair<guide_ptr_t, guide_ptr_t> findGuidePairUnderMouse(const Layout* rootLayout, const fvec2& mousePos);
  void dragGuidePairH(const std::pair<guide_ptr_t, guide_ptr_t>& pair, float deltaY);
  void dragGuidePairV(const std::pair<guide_ptr_t, guide_ptr_t>& pair, float deltaX);
};
///////////////////////////////////////////////////////////
HandlerResult LayoutGroup::OnUiEvent(event_constptr_t ev) {
  // ev->mFilteredEvent.Reset();
  //printf("LayoutGroup<%p>::OnUiEvent _evhandlerset<%d>\n", this, int(_evhandler != nullptr));
  ui::HandlerResult result;
  bool was_handled = false;
  static std::pair<anchor::guide_ptr_t, anchor::guide_ptr_t> GUIDES_UNDER_MOUSE;
  static int lastx = ev->miX;
  static int lasty = ev->miY;
  switch (ev->_eventcode) {
    case ui::EventCode::PUSH: {
      was_handled = true;
      break;
    }
    case ui::EventCode::RELEASE: {
      was_handled = true;
      break;
    }
    case ui::EventCode::BEGIN_DRAG: {
      was_handled       = true;
      result.mHoldFocus = true;
      GUIDES_UNDER_MOUSE = anchor::findGuidePairUnderMouse(_layout.get(), fvec2(ev->miX, ev->miY));
      auto g1 = GUIDES_UNDER_MOUSE.first;
      auto g2 = GUIDES_UNDER_MOUSE.second;
      printf( "g1<%p> g2<%p>\n", g1.get(), g2.get() );
      if(g1 and g2){
        auto w1 = g1->_layout->_widget->_name;
        auto w2 = g1->_layout->_widget->_name;
        printf( "w1<%s> w2<%s>\n", w1.c_str(), w2.c_str() );
      }
      lastx = ev->miX;
      lasty = ev->miY;
      break;
    }
    case ui::EventCode::END_DRAG: {
      was_handled       = true;
      result.mHoldFocus = false;
      GUIDES_UNDER_MOUSE = std::pair<anchor::guide_ptr_t, anchor::guide_ptr_t>(nullptr,nullptr);
      break;
    }
    case ui::EventCode::DRAG: {
      result.mHoldFocus = true;
      was_handled       = true;
      auto g1 = GUIDES_UNDER_MOUSE.first;
      auto g2 = GUIDES_UNDER_MOUSE.second;
      int dx           = ev->miX - lastx;
      int dy           = ev->miY - lasty;
      if(g1 and g2 and (not g1->_locked) and (not g2->_locked)){
        if(g1->isVertical() && g2->isVertical()){
          dragGuidePairV(GUIDES_UNDER_MOUSE, dx);
          _layout->updateAll();
        }
        else if(g1->isHorizontal() && g2->isHorizontal()){
          dragGuidePairH(GUIDES_UNDER_MOUSE, dy);
          _layout->updateAll();
        }
        else{
          printf("BAD GUIDE PAIR\n");
        }
      }
      lastx = ev->miX;
      lasty = ev->miY;
      break;
    }
  }
  if (was_handled)
    result.setHandled(this);
  return result;
}
/////////////////////////////////////////////////////////////////////////
} // namespace ork::ui
