#include <ork/pch.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/ui/viewport.h>
#include <ork/lev2/ui/event.h>
#include <ork/lev2/gfx/gfxmaterial_ui.h>
#include <ork/util/hotkey.h>
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/gfx/gfxprimitives.h>

namespace ork::ui {
///////////////////////////////////////////////////////////////////////////////
Rect::Rect(int x, int y, int w, int h)
    : _x(x)
    , _y(y)
    , _w(w)
    , _h(h) {
}
///////////////////////////////////////////////////////////////////////////////
Rect::Rect() {
  _x = 0;
  _y = 0;
  _w = 0;
  _h = 0;
}
///////////////////////////////////////////////////////////////////////////////
SRect Rect::asSRect() const {
  SRect rval(_x, _y, _w, _h);
  return rval;
}
///////////////////////////////////////////////////////////////////////////////
int Rect::x2() const {
  return _x + (_w - 1);
}
///////////////////////////////////////////////////////////////////////////////
int Rect::y2() const {
  return _y + (_h - 1);
}
///////////////////////////////////////////////////////////////////////////////
int Rect::center_x() const {
  return _x + (_w >> 1);
}
///////////////////////////////////////////////////////////////////////////////
int Rect::center_y() const {
  return _y + (_h >> 1);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::moveTop(int y) {
  _y = y;
  printf("rect<%p> moveTop<%d>\n", this, y);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::moveLeft(int x) {
  int dx = x - _x;
  _x += dx;
  printf("rect<%p> moveLeft<%d>\n", this, x);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::moveBottom(int y) {
  int dy = y - y2();
  _y += dy;
  printf("rect<%p> moveBottom<%d>\n", this, y);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::moveRight(int x) {
  int dx = x - x2();
  _x += dx;
  printf("rect<%p> moveRight<%d>\n", this, x);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::moveCenter(int x, int y) {
  int dx = x - center_x();
  int dy = y - center_y();
  _x += dx;
  _y += dy;
}
///////////////////////////////////////////////////////////////////////////////
void Rect::setTop(int y) {
  int dy = y - _y;
  _y += dy;
  _h -= dy;
  printf("rect<%p> setTop<%d>\n", this, y);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::setLeft(int x) {
  int dx = x - _x;
  _x += dx;
  _w -= dx;
  printf("rect<%p> setLeft<%d>\n", this, x);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::setBottom(int y) {
  int dy = y - y2();
  _h += dy;
  printf("rect<%p> setBottom<%d>\n", this, y);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::setRight(int x) {
  int dx = x - x2();
  _w += dx;
  printf("rect<%p> setRight<%d>\n", this, x);
}
///////////////////////////////////////////////////////////////////////////////
void Rect::reset() {
  _x = 0;
  _y = 0;
  _w = 0;
  _h = 0;
}
///////////////////////////////////////////////////////////////////////////////
bool Rect::isPointInside(int x, int y) const {
  return (x >= _x) and (x <= x2()) and (y >= _y) and (y <= y2());
}
/////////////////////////////////////////////////////////////////////////
} // namespace ork::ui