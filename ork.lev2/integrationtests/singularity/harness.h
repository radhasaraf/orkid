#include <QWindow>
#include <portaudio.h>
#include <ork/application/application.h>
#include <ork/kernel/string/deco.inl>
#include <ork/kernel/timer.h>
#include <ork/file/path.h>
#include <ork/lev2/ezapp.h>
#include <ork/lev2/aud/singularity/synth.h>
#include <ork/lev2/aud/singularity/synthdata.h>
#include <ork/lev2/aud/singularity/synth.h>
#include <ork/lev2/aud/singularity/krzobjects.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/material_freestyle.h>

using namespace std::string_literals;
using namespace ork;
using namespace ork::lev2;
using namespace ork::audio::singularity;

namespace ork::lev2 {
void startupAudio();
void tearDownAudio();
} // namespace ork::lev2

qtezapp_ptr_t createEZapp(int& argc, char** argv);

inline void enqueue_audio_event(
    const ProgramData* prog, //
    float time,
    float duration,
    int midinote) {
  synth::instance()->addEvent(time, [=]() {
    // NOTE ON
    printf("time<%g> note<%d> program<%s>\n", time, midinote, prog->_name.c_str());
    auto noteinstance = synth::instance()->keyOn(midinote, prog);
    assert(noteinstance);
    // NOTE OFF
    synth::instance()->addEvent(time + duration, [=]() { //
      synth::instance()->keyOff(noteinstance);
    });
  });
}