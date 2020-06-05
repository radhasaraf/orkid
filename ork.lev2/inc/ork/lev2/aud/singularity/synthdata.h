#pragma once

#include "krztypes.h"
#include "modulation.h"
#include <ork/kernel/svariant.h>
#include <ork/file/path.h>

namespace ork::audio::singularity {
file::Path basePath();

///////////////////////////////////////////////////////////////////////////////

struct ProgramData {
  lyrdata_ptr_t newLayer();
  lyrdata_ptr_t getLayer(int i) const {
    return _layerdatas[i];
  }
  std::string _name;
  std::string _role;
  std::vector<lyrdata_ptr_t> _layerdatas;
};

///////////////////////////////////////////////////////////////////////////////

struct programInst;

struct SynthData {
  SynthData();
  virtual ~SynthData() {
  }

  float seqTime(float dur);
  prgdata_constptr_t getProgram(int progID) const;
  prgdata_constptr_t getProgramByName(const std::string& named) const;

  bankdata_ptr_t _bankdata;
  programInst* _prog;
  float _synsr;
  float _seqCursor;
  std::string _staticBankName;
};

void hexdumpbytes(std::vector<uint8_t> bytes);

} // namespace ork::audio::singularity
