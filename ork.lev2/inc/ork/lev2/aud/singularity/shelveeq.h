////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

namespace ork::audio::singularity {

struct ShelveEq {
  void init();
  float _SPN;
  float _x1l, _x2l;
  float _x1r, _x2r;
  float _y1l, _y2l;
  float _yl, _yr, _y1r, _y2r;
  float _a0, _a1, _a2, _b1, _b2;
};
struct LoShelveEq : public ShelveEq {
  void set(float fc, float peakg);
  float compute(float inp);
};
struct HiShelveEq : public ShelveEq {
  void set(float cf, float peakg);
  float compute(float inp);
};

} // namespace ork::audio::singularity
