#include "harness.h"

int main(int argc, char** argv) {
  auto qtapp    = createEZapp(argc, argv);
  auto basepath = file::Path::share_dir() / "singularity" / "casioCZ";
  startupAudio();
  //////////////////////////////////////////////////////////////////////////////
  auto czdata = std::make_shared<CzData>();
  czdata->loadBank(basepath / "factoryA.bnk", "bank1");
  czdata->loadBank(basepath / "factoryB.bnk", "bank2");
  for (int i = 0; i < 64; i++) { // 2 32 patch banks
    auto prg = czdata->getProgram(i);
    printf("i<%d> prg<%p>\n", i, prg);
    enqueue_audio_event(prg, float(i), 1.0, 48);
  }
  //////////////////////////////////////////////////////////////////////////////
  qtapp->setRefreshPolicy({EREFRESH_FASTEST, 0});
  qtapp->exec();
  tearDownAudio();
  return 0;
}
