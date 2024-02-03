////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/lev2/aud/singularity/synth.h>
#include <assert.h>
#include <ork/lev2/aud/singularity/filters.h>
#include <ork/lev2/aud/singularity/alg_nonlin.h>

ImplementReflectionX(ork::audio::singularity::SHAPER_DATA, "DspNonlinShaper");
ImplementReflectionX(ork::audio::singularity::SHAPE2_DATA, "DspNonlinShaper2");
ImplementReflectionX(ork::audio::singularity::TWOPARAM_SHAPER_DATA, "DspNonlinShaper2Param");
ImplementReflectionX(ork::audio::singularity::WrapData, "DspNonlinWrap");
ImplementReflectionX(ork::audio::singularity::DistortionData, "DspNonlinDistortion");

namespace ork::audio::singularity {

float shaper(float inp, float adj);
float wrap(float inp, float adj);

///////////////////////////////////////////////////////////////////////////////

void SHAPER_DATA::describeX(class_t* clazz){}

SHAPER_DATA::SHAPER_DATA(std::string name)
    : DspBlockData(name) {
  _blocktype = "SHAPER";
  addParam("amount","x")->useDefaultEvaluator(); 
}
dspblk_ptr_t SHAPER_DATA::createInstance() const {
  return std::make_shared<SHAPER>(this);
}

SHAPER::SHAPER(const DspBlockData* dbd)
    : DspBlock(dbd) {
}

void SHAPER::compute(DspBuffer& dspbuf) // final
{
  float pad      = _dbd->_inputPad;
  int inumframes = _layer->_dspwritecount;

  float amt = _param[0].eval(); //,0.01f,100.0f);
  _fval[0]  = amt;

  // float la = decibel_to_linear_amp_ratio(amt);
  if (not _dbd->_bypass) {
    auto inputchan  = getInpBuf(dspbuf, 0) + _layer->_dspwritebase;
    auto outputchan = getOutBuf(dspbuf, 0) + _layer->_dspwritebase;
    for (int i = 0; i < inumframes; i++) {
      float s1      = shaper(inputchan[i] * pad, amt);
      outputchan[i] = s1;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void SHAPE2_DATA::describeX(class_t* clazz){}

SHAPE2_DATA::SHAPE2_DATA(std::string name)
    : DspBlockData(name) {
  _blocktype = "SHAPE2";
  addParam("amount","x")->useDefaultEvaluator(); 
}
dspblk_ptr_t SHAPE2_DATA::createInstance() const {
  return std::make_shared<SHAPE2>(this);
}

SHAPE2::SHAPE2(const DspBlockData* dbd)
    : DspBlock(dbd) {
}

void SHAPE2::compute(DspBuffer& dspbuf) // final
{
  float pad      = _dbd->_inputPad;
  int inumframes = _layer->_dspwritecount;
  float amt      = _param[0].eval();
  _fval[0]       = amt;
  if (1) {
    auto inputchan  = getInpBuf(dspbuf, 0) + _layer->_dspwritebase;
    auto outputchan = getOutBuf(dspbuf, 0) + _layer->_dspwritebase;
    for (int i = 0; i < inumframes; i++) {
      float s1      = shaper(inputchan[i] * pad, amt);
      float s2      = shaper(s1, amt * 0.75);
      outputchan[i] = s2;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void TWOPARAM_SHAPER_DATA::describeX(class_t* clazz){}

TWOPARAM_SHAPER_DATA::TWOPARAM_SHAPER_DATA(std::string name)
    : DspBlockData(name) {
  _blocktype = "TWOPARAM_SHAPER";
  addParam("even")->useDefaultEvaluator(); 
  addParam("odd")->useDefaultEvaluator(); 
}
dspblk_ptr_t TWOPARAM_SHAPER_DATA::createInstance() const {
  return std::make_shared<TWOPARAM_SHAPER>(this);
}

TWOPARAM_SHAPER::TWOPARAM_SHAPER(const DspBlockData* dbd)
    : DspBlock(dbd) {
}

/*float shaper(float inp, float adj)
{
    float index = pi*4.0f*inp*adj;
    return sinf(index); ///adj;
}*/

void TWOPARAM_SHAPER::doKeyOn(const KeyOnInfo& koi) {
  ph1 = 0.0f;
  ph2 = 0.0f;
}

void TWOPARAM_SHAPER::compute(DspBuffer& dspbuf) // final
{
  float pad      = _dbd->_inputPad;
  int inumframes = _layer->_dspwritecount;
  float evn      = _param[0].eval();
  float odd      = _param[1].eval();

  // evn = -22;//(0.5f+sinf(ph1+pi)*0.5f)*-60.0f;
  // odd = (sinf(ph1)*30.f)-30.0f;
  ph1 += 0.0003f;

  _fval[0] = evn;
  _fval[1] = odd;
  // printf( "_dbd->_inputPad<%f>\n", _dbd->_inputPad );
  if (1) {
    auto inputchan  = getInpBuf(dspbuf, 0) + _layer->_dspwritebase;
    auto outputchan = getOutBuf(dspbuf, 0) + _layer->_dspwritebase;
    for (int i = 0; i < inumframes; i++) {
      float u   = inputchan[i] * pad;
      float usq = u * u;
      float le  = usq * decibel_to_linear_amp_ratio(evn);
      float lo  = u * decibel_to_linear_amp_ratio(odd);

      // float e = (2.0f*powf(le,2.0f))-1.0f;
      // float e = ((2.0f*powf(le,2.0f))-1.0f)*1000.0f;//decibel_to_linear_amp_ratio(-evn);
      float index = clip_float(le * 6, -12, 12); // clip_float(powf(le,4),-12.0f,12.0f);
      // index *= index;
      float e = sinf(index * pi2); /// adj;

      index   = clip_float(lo * 6, -12.0f, 12.0f);
      float o = sinf(index * pi2); /// adj;

      float r = (e + o) * 0.5f;
      // r = wrap(r,-30);
      outputchan[i] = r;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void WrapData::describeX(class_t* clazz){}

WrapData::WrapData(std::string name)
    : DspBlockData(name) {
  _blocktype = "WRAP";
  addParam("adjust", "dB")->useAmplitudeEvaluator();
}
dspblk_ptr_t WrapData::createInstance() const {
  return std::make_shared<Wrap>(this);
}
Wrap::Wrap(const DspBlockData* dbd)
    : DspBlock(dbd) {
}

void Wrap::compute(DspBuffer& dspbuf) // final
{
  int inumframes = _layer->_dspwritecount;
  float adjust   = _param[0].eval(false);
  _fval[0]       = adjust;
 // printf( "adjust<%g>\n", adjust);
  if (not _dbd->_bypass) {
    auto inpbuf = getInpBuf(dspbuf, 0) + _layer->_dspwritebase;
    auto outbuf = getOutBuf(dspbuf, 0) + _layer->_dspwritebase;
    float gain = decibel_to_linear_amp_ratio(adjust+30.0f);
    for (int i = 0; i < inumframes; i++) {
      outbuf[i] = fmod(inpbuf[i] * gain, 1.0f);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void DistortionData::describeX(class_t* clazz){}

DistortionData::DistortionData(std::string name)
    : DspBlockData(name) {
  _blocktype = "DIST";
  addParam("drive","dB")->useDefaultEvaluator();
}
dspblk_ptr_t DistortionData::createInstance() const {
  return std::make_shared<Distortion>(this);
}
Distortion::Distortion(const DspBlockData* dbd)
    : DspBlock(dbd) {
}

void Distortion::compute(DspBuffer& dspbuf) // final
{
  float pad      = _dbd->_inputPad;
  int inumframes = _layer->_dspwritecount;
  float adj      = _param[0].eval();
  _fval[0]       = adj;
  float ratio    = decibel_to_linear_amp_ratio(adj + 30.0) * pad;
  float invratio = 1.0f / ratio;
  if (1) {
    auto inputchan  = getInpBuf(dspbuf, 0) + _layer->_dspwritebase;
    auto outputchan = getOutBuf(dspbuf, 0) + _layer->_dspwritebase;
    for (int i = 0; i < inumframes; i++) {
      float inp     = inputchan[i];
      float out     = inp * ratio;
      out           = softsat(out, 1);
      outputchan[i] = out * invratio;
      // printf("inp<%g> out<%g>\n", inp, out);
    }
  }
}

} // namespace ork::audio::singularity
