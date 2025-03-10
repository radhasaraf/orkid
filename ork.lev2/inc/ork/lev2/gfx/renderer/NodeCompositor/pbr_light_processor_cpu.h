////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include "NodeCompositor.h"
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/lev2/gfx/rtgroup.h>
#include <ork/lev2/gfx/renderer/compositor.h>
#include <ork/lev2/gfx/renderer/irendertarget.h>
#include <ork/lev2/gfx/lighting/gfx_lighting.h>

namespace ork::lev2::pbr::deferrednode {

///////////////////////////////////////////////////////////////////////////////
// CpuLightProcessor :
//  render depthcluster map on GPU
//  transfer depthcluster map to CPU
//  cull and chunkify lights on CPU
//  submit light primitives on CPU
///////////////////////////////////////////////////////////////////////////////

struct CpuLightProcessor {

  static constexpr size_t KMAXLIGHTS         = 512;
  static constexpr int KMAXNUMTILESX         = 512;
  static constexpr int KMAXNUMTILESY         = 256;
  static constexpr int KMAXTILECOUNT         = KMAXNUMTILESX * KMAXNUMTILESY;
  static constexpr size_t KMAXLIGHTSPERCHUNK = 32768 / sizeof(fvec4);

  /////////////////////////////////////////////////////

  CpuLightProcessor(DeferredContext& defctx,DeferredCompositingNodePbr* compnode);

  /////////////////////////////////////////////////////

  void render(CompositorDrawData& drawdata, const ViewData& VD, enumeratedlights_constptr_t enumlights);

  /////////////////////////////////////////////////////
  void _gpuInit(lev2::Context* target);
  void _clearFrameLighting();
  void _renderUnshadowedUnTexturedPointLights(CompositorDrawData& drawdata, const ViewData& VD, enumeratedlights_constptr_t enumlights);

  typedef std::vector<PointLight> pllist_t;
  typedef ork::LockedResource<pllist_t> locked_pllist_t;

  ork::fixedvector<locked_pllist_t, KMAXTILECOUNT> _lighttiles;
  int _pendingtiles[KMAXTILECOUNT];
  ork::fixedvector<int, KMAXTILECOUNT> _chunktiles;
  ork::fixedvector<fvec4, KMAXTILECOUNT> _chunktiles_pos;
  ork::fixedvector<fvec4, KMAXTILECOUNT> _chunktiles_uva;
  ork::fixedvector<fvec4, KMAXTILECOUNT> _chunktiles_uvb;
  std::atomic<int> _lightjobcount;
  std::atomic<int> _pendingtilecounter;
  fxuniformbuffer_ptr_t _lightbuffer = nullptr;
  DeferredContext& _deferredContext;
  const uint32_t* _depthClusterBase = nullptr;
  DeferredCompositingNodePbr* _defcompnode;
  std::vector<PointLight> _pointlights;

};

} // namespace ork::lev2::deferrednode {
