////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/aud/audiodevice.h>
#include <ork/file/file.h>
#include <ork/file/path.h>
#include <ork/file/chunkfile.h>
#include <ork/kernel/string/StringBlock.h>
#include <ork/kernel/orklut.hpp>
#include <ork/kernel/fixedlut.hpp>
#include <ork/kernel/tempstring.h>
#include <ork/asset/FileAssetLoader.h>
#include <ork/asset/FileAssetNamer.h>
#include <ork/math/audiomath.h>
#include <ork/reflect/properties/register.h>

///////////////////////////////////////////////////////////////////////////////

#include "null/audiodevice_null.h"
#if defined(ENABLE_ALSA)
#include "alsa/audiodevice_alsa.h"
#define NativeDevice AudioDeviceAlsa
#elif defined(ENABLE_PORTAUDIO)
#include "portaudio/audiodevice_pa.h"
#define NativeDevice AudioDevicePa
#endif

bool gb_audio_filter = false;

using namespace ork::audiomath;

///////////////////////////////////////////////////////////////////////////////

template class ork::orklut<int, ork::PoolString>;

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace lev2 {

///////////////////////////////////////////////////////////////////////////////

audiodevice_ptr_t AudioDevice::instance(void) {
  static audiodevice_ptr_t device = std::make_shared<NativeDevice>();
  return device;
}

///////////////////////////////////////////////////////////////////////////////

AudioDevice::AudioDevice() {
}

AudioDevice::~AudioDevice() {
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2
