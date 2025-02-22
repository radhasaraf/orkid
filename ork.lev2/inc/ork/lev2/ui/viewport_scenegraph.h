////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/lev2/ui/ui.h>
#include <ork/lev2/ui/viewport.h>
#include <ork/lev2/gfx/scenegraph/scenegraph.h>

namespace ork { namespace ui {

struct SceneGraphViewport : public Viewport {
  RttiDeclareAbstract(SceneGraphViewport, Viewport);
public:
  SceneGraphViewport(const std::string& name, int x=0, int y=0, int w=0, int h=0);
  void DoRePaintSurface(ui::drawevent_constptr_t drwev) final;
  void _doGpuInit(lev2::Context* pTARG) final;
  void forkDB();

  lev2::scenegraph::scene_ptr_t _scenegraph;
  lev2::compositoroutnode_rtgroup_ptr_t _outputnode;
  std::string _cameraname = "spawncam";
  lev2::acqdrawbuffer_ptr_t _override_acqdbuf;

};

}} // namespace ork::ui
