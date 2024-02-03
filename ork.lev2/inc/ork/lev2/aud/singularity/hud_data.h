////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <mutex>
#include "envelope.h"
#include "konoff.h"

namespace ork::audio::singularity {
static const int koscopelength     = 8192;
static const int koscopelengthmask = koscopelength - 1;

////////////////////////////////////////

struct lfoframe {
  int _index           = 0;
  float _value         = 0.0f;
  float _currate       = 1.0f;
  const LfoData* _data = nullptr;
};
struct funframe {
  int _index           = 0;
  float _value         = 0.0f;
  float _a             = 0.0f;
  float _b             = 0.0f;
  const FunData* _data = nullptr;
};
////////////////////////////////////////
struct HudFrameControl {
  lyrdata_constptr_t _layerdata;
  alg_ptr_t _alg;
  kmregion_constptr_t _kmregion = nullptr;
  int _note                 = 0;
  int _vel                  = 0;
  int _layerIndex           = -1;
  bool _useFm4              = false;

  std::string _miscText;
};
} // namespace ork::audio::singularity
