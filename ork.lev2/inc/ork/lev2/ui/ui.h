////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/lev2/ui/enum.h>

///////////////////////////////////////////////////////////////////////////////

namespace ork {
class HotKey;
namespace lev2 {
class RtGroup;
class Context;
class Window;
class FrameTechniqueBase;
class FrameRenderer;
class PickBuffer;
class RenderContextFrameData;
} // namespace lev2

///////////////////////////////////////////////////////////////////////////////

namespace ui {

struct Widget;
struct Group;
struct Surface;
struct Panel;
struct SplitPanel;
struct Viewport;
struct Coordinate;
struct Event;
struct DrawEvent;
struct MultiTouchPoint;

using widget_ptr_t     = std::shared_ptr<Widget>;
using group_ptr_t      = std::shared_ptr<Group>;
using surface_ptr_t    = std::shared_ptr<Surface>;
using splitpanel_ptr_t = std::shared_ptr<SplitPanel>;
using panel_ptr_t      = std::shared_ptr<Panel>;
using viewport_ptr_t   = std::shared_ptr<Viewport>;

///////////////////////////////////////////////////////////////////////////////

} // namespace ui
} // namespace ork

///////////////////////////////////////////////////////////////////////////////
