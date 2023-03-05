////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/lev2/aud/singularity/krztypes.h>
#include "reflection.h"

namespace ork::audio::singularity {

///////////////////////////////////////////////////////////////////////////////

struct BlockModulationData final : public ork::Object {

  DeclareConcreteX(BlockModulationData, ork::Object);

  BlockModulationData();
  BlockModulationData(const BlockModulationData&) = delete;

  controllerdata_ptr_t _src1;
  controllerdata_ptr_t _src2;
  controllerdata_ptr_t _src2DepthCtrl;

  float _src1Depth    = 0.0f;
  float _src2MinDepth = 0.0f;
  float _src2MaxDepth = 0.0f;
  evalit_t _evaluator;
};

///////////////////////////////////////////////////////////////////////////////

struct DspParamData final : public ork::Object {

  DeclareConcreteX(DspParamData, ork::Object);
  bool postDeserialize(reflect::serdes::IDeserializer&) override;

  DspParamData(std::string name = "");

  void useDefaultEvaluator();
  void usePitchEvaluator();
  void useFrequencyEvaluator();
  void useAmplitudeEvaluator();
  void useKrzPosEvaluator();
  void useKrzEvnOddEvaluator();

  std::string _name;
  std::string _units;
  std::string _evaluatorid;

  int _edit_coarse_numsteps = 1;
  float _edit_coarse_shape  = 1.0f;
  float _edit_coarse_min    = 0.0f;
  float _edit_coarse_max    = 1.0f;

  int _edit_fine_numsteps = 1;
  float _edit_fine_shape  = 1.0f;
  float _edit_fine_min    = 0.0f;
  float _edit_fine_max    = 1.0f;

  int _edit_keytrack_numsteps = 1;
  float _edit_keytrack_shape  = 1.0f;
  float _edit_keytrack_min    = 0.0f;
  float _edit_keytrack_max    = 1.0f;

  float _coarse         = 0.0f;
  float _fine           = 0.0f;
  float _fineHZ         = 0.0f;
  float _keyTrack       = 0.0f;
  float _velTrack       = 0.0f;
  int _keystartNote     = 60;
  bool _keystartBipolar = true; // false==unipolar
  // evalit_t _evaluator;
  dspparammod_ptr_t _mods;
};

///////////////////////////////////////////////////////////////////////////////

struct DspParam : public ork::Object {

  DspParam();
  void reset();
  void keyOn(int ikey, int ivel);
  float eval(bool dump = false);

  dspparam_constptr_t _data;

  controller_t _C1;
  controller_t _C2;
  int _keyRaw = 0;
  float _keyOff  = 0.0f;
  float _unitVel = 0.0f;

  float _s1val = 0.0f;
  float _s2val = 0.0f;
  float _kval  = 0.0f;
  float _vval  = 0.0f;

  evalit_t _evaluator;
  int _update_count = 0;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::audio::singularity
