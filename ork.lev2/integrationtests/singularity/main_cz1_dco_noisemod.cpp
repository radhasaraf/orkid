#include "harness.h"
#include <ork/lev2/aud/singularity/cz1.h>
#include <ork/lev2/aud/singularity/alg_oscil.h>
#include <ork/lev2/aud/singularity/alg_amp.h>

using namespace ork::audio::singularity;

int main(int argc, char** argv) {
  auto qtapp = createEZapp(argc, argv);
  //////////////////////////////////////////////////////////////////////////////
  // allocate program/layer data
  //////////////////////////////////////////////////////////////////////////////
  auto program   = std::make_shared<ProgramData>();
  auto layerdata = program->newLayer();
  auto czoscdata = std::make_shared<CzOscData>();
  program->_role = "czx";
  program->_name = "test";
  //////////////////////////////////////
  // setup dsp graph
  //////////////////////////////////////
  layerdata->_algdata  = configureCz1Algorithm(1);
  auto dcostage        = layerdata->stageByName("DCO");
  auto ampstage        = layerdata->stageByName("AMP");
  auto osc             = dcostage->appendTypedBlock<CZX>(czoscdata, 0);
  auto amp             = ampstage->appendTypedBlock<AMP_MONOIO>();
  czoscdata->_noisemod = true;
  //////////////////////////////////////
  // setup modulators
  //////////////////////////////////////
  auto DCAENV = layerdata->appendController<RateLevelEnvData>("DCAENV");
  auto DCWENV = layerdata->appendController<RateLevelEnvData>("DCWENV");
  auto LFO2   = layerdata->appendController<LfoData>("MYLFO2");
  auto LFO1   = layerdata->appendController<LfoData>("MYLFO1");
  //////////////////////////////////////
  // setup envelope
  //////////////////////////////////////
  DCAENV->_ampenv = true;
  DCAENV->_segments.push_back({.2, .7});  // atk1
  DCAENV->_segments.push_back({1, 1});    // atk2
  DCAENV->_segments.push_back({120, .3}); // atk3
  DCAENV->_segments.push_back({120, 0});  // atk3
                                          //
  DCWENV->_ampenv = false;
  DCWENV->_segments.push_back({0.1, .7}); // atk1
  DCWENV->_segments.push_back({1, 1});    // atk2
  DCWENV->_segments.push_back({2, .5});   // atk3
  DCWENV->_segments.push_back({2, 1});    // dec
  DCWENV->_segments.push_back({2, 1});    // rel1
  DCWENV->_segments.push_back({40, 1});   // rel2
  DCWENV->_segments.push_back({40, 0});   // rel3
  //////////////////////////////////////
  // setup LFO
  //////////////////////////////////////
  LFO1->_minRate = 0.25;
  LFO1->_maxRate = 0.25;
  LFO1->_shape   = "Sine";
  LFO2->_minRate = 3.3;
  LFO2->_maxRate = 3.3;
  LFO2->_shape   = "Sine";
  //////////////////////////////////////
  // setup modulation routing
  //////////////////////////////////////
  auto& modulation_index_param      = osc->_paramd[0]._mods;
  modulation_index_param._src1      = DCWENV;
  modulation_index_param._src1Depth = 1.0;
  modulation_index_param._src2      = LFO1;
  // modulation_index_param._src2DepthCtrl = LFO2;
  modulation_index_param._src2MinDepth = 0.5;
  modulation_index_param._src2MaxDepth = 0.1;
  //////////////////////////////////////
  auto& amp_param   = amp->_paramd[0];
  amp_param._coarse = 0.0f;
  amp_param.useDefaultEvaluator();
  amp_param._mods._src1      = DCAENV;
  amp_param._mods._src1Depth = 1.0;
  //////////////////////////////////////
  // play test notes
  //////////////////////////////////////
  for (int i = 24; i < 84; i++)
    enqueue_audio_event(program, 0.5f + float(i - 24) * 0.5, 2.0, i);
  //////////////////////////////////////////////////////////////////////////////
  // test harness UI
  //////////////////////////////////////////////////////////////////////////////
  qtapp->setRefreshPolicy({EREFRESH_FASTEST, 0});
  qtapp->exec();
  return 0;
}
