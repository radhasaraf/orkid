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
  
static constexpr size_t kSPECTRALSIZE = 4096;

///////////////////////////////////////////////////////////////////////////////
struct ToFrequencyDomainData : public DspBlockData {
  DeclareConcreteX(ToFrequencyDomainData,DspBlockData);
  ToFrequencyDomainData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
};
struct ToFrequencyDomain : public DspBlock {
  using dataclass_t = ToFrequencyDomainData;
  ToFrequencyDomain(const dataclass_t* dbd);
  ~ToFrequencyDomain();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;

};
///////////////////////////////////////////////////////////////////////////////
struct SpectralShiftData : public DspBlockData {
  DeclareConcreteX(SpectralShiftData,DspBlockData);
  SpectralShiftData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
};
struct SpectralShift : public DspBlock {
  using dataclass_t = SpectralShiftData;
  SpectralShift(const dataclass_t* dbd);
  ~SpectralShift();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;

};
///////////////////////////////////////////////////////////////////////////////
struct SpectralScaleData : public DspBlockData {
  DeclareConcreteX(SpectralScaleData,DspBlockData);
  SpectralScaleData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
};
struct SpectralScale : public DspBlock {
  using dataclass_t = SpectralScaleData;
  SpectralScale(const dataclass_t* dbd);
  ~SpectralScale();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;

};
///////////////////////////////////////////////////////////////////////////////

struct SpectralImpulseResponse{

  SpectralImpulseResponse();

  SpectralImpulseResponse( std::vector<float>& impulseL, //
                           std::vector<float>& impulseR );

  void set( std::vector<float>& impulseL, //
            std::vector<float>& impulseR );

  void blend(const SpectralImpulseResponse& A, //
             const SpectralImpulseResponse& B, //
              float index );

  std::vector<float> _impulseL;
  std::vector<float> _impulseR;
  std::vector<float> _realL;
  std::vector<float> _realR;
  std::vector<float> _imagL;
  std::vector<float> _imagR;

};

using spectralimpulseresponse_ptr_t = std::shared_ptr<SpectralImpulseResponse>;

struct SpectralImpulseResponseDataSet{
  std::vector<spectralimpulseresponse_ptr_t> _impulses;
};

using spectralimpulseresponsedataset_ptr_t = std::shared_ptr<SpectralImpulseResponseDataSet>;

struct SpectralConvolveData : public DspBlockData {
  DeclareConcreteX(SpectralConvolveData,DspBlockData);
  SpectralConvolveData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
  spectralimpulseresponsedataset_ptr_t _impulse_dataset;
};
using spectralconvolvedata_ptr_t = std::shared_ptr<SpectralConvolveData>;
struct SpectralConvolve : public DspBlock {
  using dataclass_t = SpectralConvolveData;
  SpectralConvolve(const dataclass_t* dbd);
  ~SpectralConvolve();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;
  std::vector<float> _impulseL;
  std::vector<float> _impulseR;
  std::vector<float> _realL;
  std::vector<float> _realR;
  std::vector<float> _imagL;
  std::vector<float> _imagR;

};
///////////////////////////////////////////////////////////////////////////////
struct SpectralTestData : public DspBlockData {
  DeclareConcreteX(SpectralTestData,DspBlockData);
  SpectralTestData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
};
struct SpectralTest : public DspBlock {
  using dataclass_t = SpectralTestData;
  SpectralTest(const dataclass_t* dbd);
  ~SpectralTest();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;

};
///////////////////////////////////////////////////////////////////////////////
struct ToTimeDomainData : public DspBlockData {
  DeclareConcreteX(ToTimeDomainData,DspBlockData);
  ToTimeDomainData(std::string name="X",float feedback=0.0f);
  dspblk_ptr_t createInstance() const override;
};
struct ToTimeDomain : public DspBlock {
  using dataclass_t = ToTimeDomainData;
  ToTimeDomain(const dataclass_t* dbd);
  ~ToTimeDomain();
  void compute(DspBuffer& dspbuf) final;
  void doKeyOn(const KeyOnInfo& koi) final;

  const dataclass_t* _mydata;

};

} //namespace ork::audio::singularity {
