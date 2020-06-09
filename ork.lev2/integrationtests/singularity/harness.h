#include <QWindow>
#include <portaudio.h>
#include <ork/application/application.h>
#include <ork/kernel/string/deco.inl>
#include <ork/kernel/timer.h>
#include <ork/file/path.h>
#include <ork/lev2/ezapp.h>
#include <ork/lev2/aud/singularity/synth.h>
#include <ork/lev2/aud/singularity/synthdata.h>
#include <ork/lev2/aud/singularity/hud.h>
#include <ork/lev2/aud/singularity/krzobjects.h>
#include <ork/lev2/gfx/renderer/drawable.h>
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/kernel/timer.h>

using namespace std::string_literals;
using namespace ork;
using namespace ork::lev2;
using namespace ork::audio::singularity;

namespace ork::lev2 {
void startupAudio();
void tearDownAudio();
} // namespace ork::lev2

struct SingularityTestApp final : public OrkEzQtApp {
  SingularityTestApp(int& argc, char** argv);
  ~SingularityTestApp() override;
  hudvp_ptr_t _hudvp;
};
using singularitytestapp_ptr_t = std::shared_ptr<SingularityTestApp>;

singularitytestapp_ptr_t createEZapp(int& argc, char** argv);

inline void enqueue_audio_event(
    prgdata_constptr_t prog, //
    float time,
    float duration,
    int midinote,
    int velocity = 128) {
  auto s = synth::instance();

  if (time < s->_timeaccum) {
    time = s->_timeaccum;
  }

  s->addEvent(time, [=]() {
    // NOTE ON
    // printf("time<%g> note<%d> program<%s>\n", time, midinote, prog->_name.c_str());
    auto noteinstance = s->keyOn(midinote, velocity, prog);
    assert(noteinstance);
    // NOTE OFF
    s->addEvent(time + duration, [=]() { //
      s->keyOff(noteinstance);
    });
  });
}

prgdata_constptr_t testpattern(syndata_ptr_t syndat, int argc, char** argv);

////////////////////////////////////////////////////////////////////////////////
struct SingularityBenchMarkApp final : public OrkEzQtApp {
  static constexpr size_t KNUMFRAMES = 512;
  SingularityBenchMarkApp(int& argc, char** argv)
      : OrkEzQtApp(argc, argv) {
  }
  ~SingularityBenchMarkApp() override {
  }
  std::vector<int> _time_histogram;
  ork::lev2::freestyle_mtl_ptr_t _material;
  const FxShaderTechnique* _fxtechniqueMODC = nullptr;
  const FxShaderTechnique* _fxtechniqueVTXC = nullptr;
  const FxShaderParam* _fxparameterMVP      = nullptr;
  const FxShaderParam* _fxparameterMODC     = nullptr;
  ork::Timer _timer;
  float _inpbuf[KNUMFRAMES * 2];
  int _numiters     = 0;
  double _cur_time  = 0.0;
  double _prev_time = 0.0;
  lev2::Font* _font;
  int _charw             = 0;
  int _charh             = 0;
  double _underrunrate   = 0;
  int _numunderruns      = 0;
  double _maxvoices      = 32.0;
  double _accumnumvoices = 0.0;
};
using singularitybenchapp_ptr_t = std::shared_ptr<SingularityBenchMarkApp>;

singularitybenchapp_ptr_t createBenchmarkApp(int& argc, char** argv, prgdata_constptr_t program);
