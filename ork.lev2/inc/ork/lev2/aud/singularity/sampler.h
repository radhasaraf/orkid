////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once
#include <ork/lev2/aud/singularity/synthdata.h>
#include <ork/lev2/aud/singularity/dspblocks.h>
#include <ork/lev2/aud/singularity/envelope.h>

namespace ork::audio::singularity {

///////////////////////////////////////////////////////////////////////////////

enum struct eLoopMode {
  NOTSET = -1,
  NONE   = 0,
  FWD,
  BIDIR,
  FROMKM,
};

///////////////////////////////////////////////////////////////////////////////

struct natenvseg {
  float _slope;
  float _time;
};

///////////////////////////////////////////////////////////////////////////////

struct sample {
  sample();

  std::string _name;
  const s16* _sampleBlock;

  int _blk_start;
  int _blk_alt;

  int _blk_loopstart;
  int _blk_loopend;

  int _blk_end;

  int _loopPoint;
  int _subid;
  float _sampleRate;
  float _linGain;
  int _rootKey;
  int _highestPitch;

  eLoopMode _loopMode = eLoopMode::NONE;
  natenvwrapperdata_ptr_t _naturalEnvelope;
  int _pitchAdjust = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct multisample {
  std::string _name;
  int _objid;
  std::map<int, sample*> _samples;
};

///////////////////////////////////////////////////////////////////////////////

struct kmregion {
  int _lokey = 0, _hikey = 0;
  int _lovel = 0, _hivel = 127;
  int _tuning                 = 0;
  eLoopMode _loopModeOverride = eLoopMode::NOTSET;
  float _volAdj               = 0.0f;
  float _linGain              = 1.0f;
  int _multsampID = -1, _sampID = -1;
  std::string _sampleName;
  const multisample* _multiSample = nullptr;
  const sample* _sample           = nullptr;
};

///////////////////////////////////////////////////////////////////////////////

struct KeyMap {
  std::string _name;
  std::vector<kmregion*> _regions;
  int _kmID = -1;

  kmregion* getRegion(int note, int vel) const;
};

///////////////////////////////////////////////////////////////////////////////

struct KmpBlockData {
  keymap_constptr_t _keymap;
  int _transpose   = 0;
  float _keyTrack  = 100.0f;
  float _velTrack  = 0.0f;
  int _timbreShift = 0;
  std::string _pbMode;
};

struct RegionSearch{
  int _sampselnote = -1;
  int _sampleRoot = -1;
  int _keydiff = -1;
  float _baseCents = 0.0f;
  float _preDSPGAIN = 1.0f;
  const kmregion* _kmregion = nullptr;
  int _curpitchadjx = 0;
  int _curpitchadj = 0;
  int _kmcents = 0;
  int _pchcents = 0;
  const sample* _sample = nullptr;

};
///////////////////////////////////////////////////////////////////////////////

struct NatEnv {
  NatEnv();
  void keyOn(const KeyOnInfo& KOI, const sample* s);
  void keyOff();
  const natenvseg& getCurSeg() const;
  float compute();
  void initSeg(int iseg);

  std::vector<natenvseg> _natenvseg;
  layer_ptr_t _layer;
  int _curseg;
  int _prvseg;
  int _numseg;
  int _framesrem;
  float _segtime;
  float _curamp;
  float _slopePerSecond;
  float _slopePerSample;
  float _SR;
  bool _ignoreRelease;
  int _state;
  envadjust_method_t _envadjust;
};

struct SAMPLER_DATA : public DspBlockData {
  SAMPLER_DATA(std::string name);
  dspblk_ptr_t createInstance() const override;
  RegionSearch findRegion(lyrdata_constptr_t ld, const KeyOnInfo& koi) const;
};

///////////////////////////////////////////////////////////////////////////////

struct sampleOsc {
  sampleOsc(const SAMPLER_DATA* data);

  void keyOn(const KeyOnInfo& koi);
  void keyOff();

  void updateFreqRatio();
  void setSrRatio(float r);
  void compute(int inumfr);
  float playNoLoop();
  float playLoopFwd();
  float playLoopBid();
  // bool playbackDone() const;

  // typedef float(sampleOsc::*pbfunc_t)();
  // pbfunc_t _pbFunc = nullptr;

  float (sampleOsc::*_pbFunc)() = nullptr;

  const SAMPLER_DATA* _sampler_data;
  layer_ptr_t _lyr;
  bool _active;
  int64_t _pbindex;
  int64_t _pbindexNext;
  int64_t _pbincrem;
  float _curratio;
  eLoopMode _loopMode;
  int _loopCounter;
  //
  int64_t _blk_start;
  int64_t _blk_alt;
  int64_t _blk_loopstart;
  int64_t _blk_loopend;
  int64_t _blk_end;
  int _curcents = 0;
  NatEnvWrapperInst* _natenvwrapperinst = nullptr;


  float _playbackRate;


  float _keyoncents;

  float _dt;
  float _synsr;
  // bool _isLooped;
  bool _enableNatEnv;
  bool _forwarddir;
  float _curSampSRratio;
  float _NATENV[1024];
  float _OUTPUT[1024];
  int _samppbnote;

  RegionSearch _regionsearch;

  natenv_ptr_t _natAmpEnv;

  bool _released;
};

///////////////////////////////////////////////////////////////////////////////

struct SAMPLER final : public DspBlock {
  using dataclass_t = SAMPLER_DATA;
  SAMPLER(const DspBlockData* dbd);
  void compute(DspBuffer& dspbuf);

  void doKeyOn(const KeyOnInfo& koi);
  void doKeyOff();
  sampleOsc* _spOsc = nullptr;
  natenvwrapperdata_ptr_t _natenvwrapperdata;
  float _filtp;
};

} // namespace ork::audio::singularity
