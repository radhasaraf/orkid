////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////


#pragma once

#include "reflection.h"
#include "dspblocks.h"
#include <ork/math/cmatrix4.h>
#include <ork/math/cmatrix3.h>
#include <ork/math/cvector4.h>
#include <ork/math/cvector2.h>
#include "shelveeq.h"
#include "delays.h"

namespace ork::audio::singularity {

///////////////////////////////////////////////////////////////////////////////
struct Sum2Data final : public DspBlockData {
  DeclareConcreteX(Sum2Data,DspBlockData);
  Sum2Data(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};

struct SUM2 : public DspBlock {
  using dataclass_t = Sum2Data;
  SUM2(const DspBlockData* dbd);
  void compute(DspBuffer& dspbuf) final;
};
///////////////////////////////////////////////////////////////////////////////
struct MonoInStereoOutData : public DspBlockData {
  DeclareConcreteX(MonoInStereoOutData,DspBlockData);
  MonoInStereoOutData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct MonoInStereoOut : public DspBlock {
  using dataclass_t = MonoInStereoOutData;
  MonoInStereoOut(const DspBlockData* dbd);
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
  float _filt;
  float _panbase;
};
///////////////////////////////////////////////////////////////////////////////
struct StereoEnhancerData : public DspBlockData {
  DeclareConcreteX(StereoEnhancerData,DspBlockData);
  StereoEnhancerData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct StereoEnhancer : public DspBlock {
  using dataclass_t = StereoEnhancerData;
  StereoEnhancer(const DspBlockData* dbd);
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
};
///////////////////////////////////////////////////////////////////////////////
struct PitchShifterData : public DspBlockData {
  DeclareConcreteX(PitchShifterData,DspBlockData);
  PitchShifterData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct PitchShifter : public DspBlock {
  using dataclass_t = PitchShifterData;
  PitchShifter(const PitchShifterData* dbd);
  ~PitchShifter();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

};
///////////////////////////////////////////////////////////////////////////////
struct RecursivePitchShifterData : public DspBlockData {
  DeclareConcreteX(RecursivePitchShifterData,DspBlockData);
  RecursivePitchShifterData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
  float _feedback;
};
struct RecursivePitchShifter : public DspBlock {
  using dataclass_t = RecursivePitchShifterData;
  RecursivePitchShifter(const dataclass_t* dbd);
  ~RecursivePitchShifter();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;

};
///////////////////////////////////////////////////////////////////////////////
struct TimeDomainConvolveData : public DspBlockData {
  DeclareConcreteX(TimeDomainConvolveData,DspBlockData);
  TimeDomainConvolveData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct TimeDomainConvolve : public DspBlock {
  using dataclass_t = TimeDomainConvolveData;
  TimeDomainConvolve(const dataclass_t* dbd);
  ~TimeDomainConvolve();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
  const dataclass_t* _mydata;
};
///////////////////////////////////////////////////////////////////////////////
struct StereoDynamicEchoData : public DspBlockData {
  DeclareConcreteX(StereoDynamicEchoData,DspBlockData);
  StereoDynamicEchoData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct StereoDynamicEcho : public DspBlock {
  using dataclass_t = StereoDynamicEchoData;
  StereoDynamicEcho(const StereoDynamicEchoData* dbd);
  ~StereoDynamicEcho();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
  delaycontext_ptr_t _delayL;
  delaycontext_ptr_t _delayR;
};
///////////////////////////////////////////////////////////////////////////////
// Feedback Delay Network Reverb (4 nodes)
///////////////////////////////////////////////////////////////////////////////
struct Fdn4ReverbData : public DspBlockData {
  DeclareConcreteX(Fdn4ReverbData,DspBlockData);
  Fdn4ReverbData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
  float _time_base  = 0.01 ; // sec
  float _time_scale = 0.031; // sec
  float _input_gain = 0.75;  // linear
  float _output_gain = 0.75; // linear
  float _matrix_gain = 0.35; // linear
  float _allpass_shift_frq_bas = 500.0; // hz
  float _allpass_shift_frq_mul = 1.2f; // x
  int   _allpass_count = 4;
  float _hipass_cutoff = 200.0; // hz
  void matrixHadamard(float fblevel);
  void matrixHouseholder(float fbgain=0.45);
  void update();
  fmtx4 _feedbackMatrix;
};
struct Fdn4Reverb : public DspBlock {
  using dataclass_t = Fdn4ReverbData;
  Fdn4Reverb(const Fdn4ReverbData*);
  ~Fdn4Reverb();

  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const Fdn4ReverbData* _mydata;
  delaycontext_ptr_t _delayA;
  delaycontext_ptr_t _delayB;
  delaycontext_ptr_t _delayC;
  delaycontext_ptr_t _delayD;
  BiQuad _hipassfilterL;
  BiQuad _hipassfilterR;
  fmtx4 _feedbackMatrix;
  std::vector<TrapAllpass> _allpassA;
  std::vector<TrapAllpass> _allpassB;
  std::vector<TrapAllpass> _allpassC;
  std::vector<TrapAllpass> _allpassD;
  TrapAllpass _allpassE;
  TrapAllpass _allpassF;
  fvec4 _inputGainsL;
  fvec4 _inputGainsR;
  fvec4 _outputGainsL;
  fvec4 _outputGainsR;
};
///////////////////////////////////////////////////////////////////////////////
// Feedback Delay Network Reverb (4 nodes) with rotation matrix
///////////////////////////////////////////////////////////////////////////////
struct Fdn4ReverbXData : public DspBlockData {
  DeclareConcreteX(Fdn4ReverbXData,DspBlockData);
  Fdn4ReverbXData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
  float _tscale = 1.0f;

  void update();

  fvec4 _inputGainsL;
  fvec4 _inputGainsR;
  fvec4 _outputGainsL;
  fvec4 _outputGainsR;
  fvec3 _axis;
  float _angle;
  float _speed = 0.0f;
  float _time_scale = 1.0; // x
  float _input_gain = 0.75;  // linear
  float _output_gain = 0.75; // linear
  float _matrix_gain = 0.35; // linear
  float _allpass_shift_frq = 4500.0; // hz
  float _hipass_cutoff = 200.0; // hz
};
struct Fdn4ReverbX : public DspBlock {
  using dataclass_t = Fdn4ReverbXData;
  Fdn4ReverbX(const Fdn4ReverbXData*);
  ~Fdn4ReverbX();
  
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
  void matrixHadamard(float fblevel);
  void matrixHouseholder(float fbgain);

  const Fdn4ReverbXData* _mydata;
  fvec3 _axis;
  float _angle;
  float _speed = 0.0f;

  delaycontext_ptr_t _delayA;
  delaycontext_ptr_t _delayB;
  delaycontext_ptr_t _delayC;
  delaycontext_ptr_t _delayD;
  BiQuad _filterA;
  BiQuad _filterB;
  BiQuad _filterC;
  BiQuad _filterD;
  TrapAllpass _allpassA;
  TrapAllpass _allpassB;
  TrapAllpass _allpassC;
  TrapAllpass _allpassD;
  TrapAllpass _allpassE;
  TrapAllpass _allpassF;

  fmtx4 _feedbackMatrix;
  fvec4 _inputGainsL;
  fvec4 _inputGainsR;
  fvec4 _outputGainsL;
  fvec4 _outputGainsR;
  fvec4 _delayTimes;
};

struct vec8f {
  vec8f();
  void broadcast(float inp);
  void broadcast(fvec2 inp);
  float _elements[8];
  float dotWith(const vec8f& oth) const;
  vec8f operator+(const vec8f& oth) const;
  vec8f operator-(const vec8f& oth) const;
  vec8f operator*(float scalar) const;
};
struct mtx8f {
  float _elements[8][8];
  vec8f column(int i) const;
  static mtx8f generateHadamard();
  static mtx8f householder(const vec8f& v);
  static mtx8f permute(int seed);
  vec8f transform(const vec8f& input) const;
  mtx8f concat(const mtx8f& rhs) const;
  void scramble();
  void newRot();
  void setToIdentity();
  void embed3DRotation(const fmtx3& rotationMatrix, const int dimsA[3], const int dimsB[3]);
  void dump() const;
};
struct mix8to2 {
    mix8to2();
    fvec2 mixdown(const vec8f& input) const;
    float _elements[2][8];
};

struct AllpassDelay {
  void setParams(float time, float lingain);
  float compute(float inp);
  void clear();

  DelayContext _delay;
  float _gain = 0.5f;
};

struct Fdn8ReverbData : public DspBlockData {
  DeclareConcreteX(Fdn8ReverbData,DspBlockData);
  Fdn8ReverbData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
  float _time_base  = 0.01 ; // sec
  float _input_gain = 0.75;  // linear
  float _output_gain = 0.75; // linear
  float _hipass_cutoff = 200.0; // hz
  void update();
  mtx8f _hadamard;
  mtx8f _householder;
  mtx8f _permute;
};
struct ParallelLowPass{
  ParallelLowPass();
  vec8f compute(const vec8f& input);
  void set(float cutoff);
  TrapSVF _filter[8];
  BiQuad _biquads[8];
};
struct ParallelHighPass{
  ParallelHighPass();
  vec8f compute(const vec8f& input);
  void set(float cutoff);
  OnePoleHighPass _filter[8];
  BiQuad _biquads[8];
};
struct ParallelDelay{
  ParallelDelay();
  ~ParallelDelay();
  vec8f output(float fi);
  void input(const vec8f& input);
  delaycontext_ptr_t _delay[8];
  BiQuad _dcblock[8];
  OnePoleHighPass _dcblock2[8];

};

struct Fdn8Reverb : public DspBlock {
  using dataclass_t = Fdn8ReverbData;
  Fdn8Reverb(const Fdn8ReverbData*);
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  struct DiffuserStep{
    DiffuserStep();
    void setTime(float time);
    void tick();
    vec8f process(const vec8f& input,float fi);
    mtx8f _hadamard;
    mtx8f _permute;
    mtx8f _rotstep;
    mtx8f _rot;
    ParallelDelay _delays;
    float _basetimes[8];
    float _modulation;
    float _phase = 0.0f;
    float _phaseinc = 0.0f;
    float _modrate = 0.0f;
    float _modamp = 0.0f;
    vec8f _nodenorm;
    size_t _seed = 0;
    float _fbgain = 0.75f;
  };

  const Fdn8ReverbData* _mydata;
  static constexpr size_t k_num_diffusers = 8;
  DiffuserStep _diffuser[k_num_diffusers];
  BiQuad _hipassfilterL;
  BiQuad _hipassfilterR;
  ParallelLowPass _fbLP;
  ParallelLowPass _fbLP2;
  ParallelLowPass _fbLP3;
  ParallelLowPass _fbLP4;
  ParallelLowPass _fbLP5;
  ParallelHighPass _fbHP;

  ParallelDelay _fbdelay;
  float _fbbasetimes[8];
  float _fbmodulations[8];
  float _fbmodphase = 0.0f;
  ParallelDelay _early_refl;
  mtx8f _householder;
  mix8to2 _stereomix;
  vec8f _nodenorm;
};
struct TestReverbData : public DspBlockData {
  DeclareConcreteX(TestReverbData,DspBlockData);
  TestReverbData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct TestReverb : public DspBlock {
  using dataclass_t = TestReverbData;
  TestReverb(const TestReverbData*);
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
  AllpassDelay _apdel[12];
  OnePoleLoPass _lopass[3];
  OnePoleHighPass _hipass;
  float _out = 0.0f;
  float _fb1 = 0.0f;
};
struct StereoDelayData : public DspBlockData {
  DeclareConcreteX(StereoDelayData,DspBlockData);
  StereoDelayData(std::string name="X");
  dspblk_ptr_t createInstance() const override;
};
struct StereoDelay : public DspBlock {
  using dataclass_t = StereoDelayData;
  StereoDelay(const StereoDelayData*);
  ~StereoDelay();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;
  delaycontext_ptr_t _delayL;
  delaycontext_ptr_t _delayR;
};
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::audio::singularity
