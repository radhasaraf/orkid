//#include <audiofile.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <ork/lev2/aud/singularity/synthdata.h>
#include <ork/lev2/aud/singularity/dspblocks.h>
#include <ork/lev2/aud/singularity/synth.h>

namespace ork::audio::singularity {

///////////////////////////////////////////////////////////////////////////////

DspParam::DspParam() {
  _data = std::make_shared<DspParamData>();
  reset();
}

void DspParam::reset() {
  _evaluator = [](DspParam& cec) { return 0.0f; };
  _C1        = []() { return 0.0f; };
  _C2        = []() { return 0.0f; };
}

void DspParam::keyOn(int ikey, int ivel) {
  _keyOff  = float(ikey - _data->_keystartNote);
  _unitVel = float(ivel) / 127.0f;

  if (false == _data->_keystartBipolar) {
    if (_keyOff < 0)
      _keyOff = 0;

    if (_data->_keystartNote == 0)
      _keyOff = 0;

    // printf( "ikey<%d> ksn<%d> ko<%d>\n", ikey, _keystartNote, int(_keyOff) );
  }

  // printf( "_keystartNote<%d>\n", _keystartNote );
  // printf( "_keyOff<%f>\n", _keyOff );
  // printf( "_unitVel<%f>\n", _unitVel );
}

///////////////////////////////////////////////////////////////////////////////

float DspParam::eval(bool dump) {
  float tot = _evaluator(*this);
  if (dump)
    printf("coarse<%g> c1<%g> c2<%g> tot<%g>\n", _data->_coarse, _C1(), _C2(), tot);

  return tot;
}

///////////////////////////////////////////////////////////////////////////////

void DspParamData::useDefaultEvaluator() {
  _mods->_evaluator = [this](DspParam& cec) -> float {
    float kt = _keyTrack * cec._keyOff;
    float vt = -_velTrack * cec._unitVel;
    float rv = _coarse     //
               + cec._C1() //
               + cec._C2() //
               + kt + vt;
    // printf("kt<%f> vt<%f> rv<%f>\n", kt, vt, rv);
    return rv;
  };
}

///////////////////////////////////////////////////////////////////////////////

void DspParamData::useAmplitudeEvaluator() {
  _mods->_evaluator = [this](DspParam& cec) -> float {
    cec._kval  = _keyTrack * cec._keyOff;
    cec._vval  = lerp(-_velTrack, 0.0f, cec._unitVel);
    cec._s1val = cec._C1();
    cec._s2val = cec._C2();
    float x    = (_coarse) //
              + cec._s1val //
              + cec._s2val //
              + cec._kval  //
              + cec._vval;
    // printf("vt<%f> kt<%f> x<%f>\n", _velTrack, _keyTrack, x);
    return x;
  };
}

///////////////////////////////////////////////////////////////////////////////

void DspParamData::usePitchEvaluator() {

  _edit_coarse_min        = 0.0f;
  _edit_coarse_max        = 84.0f;
  _edit_coarse_numsteps   = 84;
  _edit_fine_min          = -50.0f;
  _edit_fine_max          = 50.0f;
  _edit_fine_numsteps     = 102;
  _edit_keytrack_numsteps = 402;
  _edit_keytrack_min      = -200.0f;
  _edit_keytrack_max      = 200.0f;
  _edit_keytrack_numsteps = 400;

  _mods->_evaluator = [this](DspParam& cec) -> float {
    float kt       = _keyTrack * cec._keyOff;
    float vt       = _velTrack * cec._unitVel;
    float totcents = (_coarse * 100.0f) //
                     + _fine            //
                     + cec._C1()        //
                     + cec._C2()        //
                     + kt               //
                     + vt;
    // float ratio = cents_to_linear_freq_ratio(totcents);
    // printf( "rat<%f>\n", ratio);
    /*
    printf( "cec._coarse<%f>\n", cec._coarse);
    printf( "cec._fine<%f>\n", cec._fine);
    printf( "c1<%f>\n", cec._C1());
    printf( "c2<%f>\n", cec._C2());
    printf( "vt<%f>\n", vt);
    printf( "totcents<%f>\n", totcents);
    */
    return totcents;
  };
}

///////////////////////////////////////////////////////////////////////////////

void DspParamData::useFrequencyEvaluator() {
  _mods->_evaluator = [this](DspParam& cec) -> float {
    float ktcents  = _keyTrack * cec._keyOff;
    cec._vval      = _velTrack * cec._unitVel;
    float vtcents  = cec._vval;
    float totcents = cec._C1() + cec._C2() + ktcents + vtcents;
    float ratio    = cents_to_linear_freq_ratio(totcents);
    // printf( "vtcents<%f> ratio<%f>\n", vtcents, ratio );
    // printf( "ratio<%f>\n", ratio);
    return _coarse * ratio;
  };
}

///////////////////////////////////////////////////////////////////////////////

void DspParamData::useKrzPosEvaluator() {
  _mods->_evaluator = [this](DspParam& cec) -> float {
    cec._kval  = _keyTrack * cec._keyOff;
    cec._vval  = _velTrack * cec._unitVel;
    cec._s1val = cec._C1();
    cec._s2val = cec._C2();
    float x    = (_coarse) //
              + cec._s1val //
              + cec._s2val //
              + cec._kval  //
              + cec._vval;
    return clip_float(x, -100, 100);
  };
}

///////////////////////////////////////////////////////////////////////////////

void DspParamData::useKrzEvnOddEvaluator() {
  _mods->_evaluator = [this](DspParam& cec) -> float {
    float kt = _keyTrack * cec._keyOff;
    float vt = lerp(-_velTrack, 0.0f, cec._unitVel);
    float x  = (_coarse)  //
              + cec._C1() //
              + cec._C2() //
              + kt        //
              + vt;
    // printf( "vt<%f> kt<%f> x<%f>\n", vt, kt, x );
    return clip_float(x, -10, 10);
  };
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::audio::singularity
