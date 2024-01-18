////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "harness.h"
#include <ork/lev2/aud/singularity/cz1.h>
#include <ork/lev2/aud/singularity/alg_oscil.h>
#include <ork/lev2/aud/singularity/alg_amp.h>

using namespace ork::audio::singularity;

int main(int argc, char** argv,char**envp) {
  auto initdata = std::make_shared<ork::AppInitData>(argc,argv,envp);
  auto app = createEZapp(initdata);
  //////////////////////////////////////////////////////////////////////////////
  // allocate program/layer data
  //////////////////////////////////////////////////////////////////////////////
  auto program   = std::make_shared<ProgramData>();
  auto layerdata = program->newLayer();
  auto czoscdata = std::make_shared<CzOscData>();
  program->_tags = "czx";
  program->_name = "test";
  //////////////////////////////////////
  // setup dsp graph
  //////////////////////////////////////
  auto czldc = configureCz1Algorithm(layerdata, 1);
  layerdata->_algdata = czldc._algdata;
  auto dcostage       = czldc._stage_dco;
  auto ampstage       = czldc._stage_amp;
  auto osc            = dcostage->appendTypedBlock<CZX>("dco", czoscdata, 0);
  auto amp            = ampstage->appendTypedBlock<AMP_MONOIO>("amp");
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
  DCAENV->addSegment("seg0", .2, .7);
  DCAENV->addSegment("seg1", .2, .7);
  DCAENV->addSegment("seg2", 1, 1);
  DCAENV->addSegment("seg3", 120, .3);
  DCAENV->addSegment("seg4", 120, 0);
  //
  DCWENV->_ampenv = false;
  DCWENV->addSegment("seg0", 0.1, .7);
  DCWENV->addSegment("seg1", 1, 1);
  DCWENV->addSegment("seg2", 2, .5);
  DCWENV->addSegment("seg3", 2, 1);
  DCWENV->addSegment("seg4", 2, 1);
  DCWENV->addSegment("seg5", 40, 1);
  DCWENV->addSegment("seg6", 40, 0);
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
  auto dcwmodulation        = osc->_paramd[0]->_mods;
  dcwmodulation->_src1      = DCWENV;
  dcwmodulation->_src1Scale = 1.0;
  // dcwmodulation->_src2      = LFO1;
  // dcwmodulation->_src2DepthCtrl = LFO2;
  dcwmodulation->_src2MinDepth = 0.5;
  dcwmodulation->_src2MaxDepth = 0.1;
  //////////////////////////////////////
  czoscdata->_dcoBaseWaveA = 6;
  czoscdata->_dcoBaseWaveB = 7;
  czoscdata->_dcoWindow    = 2;
  //////////////////////////////////////
  auto amp_param     = amp->_paramd[0];
  amp_param->_coarse = 0.0f;
  amp_param->useDefaultEvaluator();
  amp_param->_mods->_src1      = DCAENV;
  amp_param->_mods->_src1Scale = 1.0;
  //////////////////////////////////////
  // create and connect oscilloscope
  //////////////////////////////////////
  ui::anchor::Bounds nobounds;
  auto source   = layerdata->createScopeSource();
  auto scope    = create_oscilloscope(app->_hudvp, nobounds);
  auto analyzer = create_spectrumanalyzer(app->_hudvp, nobounds);
  source->connect(scope->_sink);
  source->connect(analyzer->_sink);
  scope->setRect(0, 0, 1280, 256);
  analyzer->setRect(0, 720 - 256, 1280, 256);
  //////////////////////////////////////
  // play test notes
  //////////////////////////////////////
  enqueue_audio_event(program, 1.5f, 240.0, 48);
  //////////////////////////////////////////////////////////////////////////////
  // test harness UI
  //////////////////////////////////////////////////////////////////////////////
  app->setRefreshPolicy({EREFRESH_FASTEST, 0});
  app->mainThreadLoop();
  return 0;
}
