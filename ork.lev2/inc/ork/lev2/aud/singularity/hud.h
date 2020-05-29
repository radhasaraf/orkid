#pragma once

#include <ork/lev2/aud/singularity/synth.h>
#include <ork/kernel/svariant.h>
#include <string>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include "synthdata.h"
#include "synth.h"
#include "fft.h"
#include <ork/lev2/gfx/dbgfontman.h>
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/lev2/ui/surface.h>
#include <ork/lev2/ui/viewport.h>
#include <ork/lev2/ui/panel.h>
#include <ork/lev2/ui/event.h>
#include <ork/lev2/ezapp.h> // todo move updatedata_ptr_t out..

namespace ork::audio::singularity {

float hud_contentscale();
int hud_lineheight();

///////////////////////////////////////////////////////////////////////////////

using vtx_t        = lev2::SVtxV16T16C16;
using vtxbuf_t     = lev2::DynamicVertexBuffer<vtx_t>;
using vtxbuf_ptr_t = std::shared_ptr<vtxbuf_t>;
vtxbuf_ptr_t get_vertexbuffer(lev2::Context* context);
lev2::freestyle_mtl_ptr_t hud_material(lev2::Context* context);

///////////////////////////////////////////////////////////////////////////////

typedef ork::svar1024_t svar_t;
void drawtext(
    ui::Surface* surface, //
    lev2::Context* ctx,   //
    const std::string& str,
    float x,
    float y,
    float scale,
    float r,
    float g,
    float b);

struct HudLine {

  fvec2 _from;
  fvec2 _to;
  fvec3 _color;
};
using hudlines_t = std::vector<HudLine>;

void drawHudLines(
    ui::Surface* surface,   //
    lev2::Context* context, //
    const hudlines_t& lines);

///////////////////////////////////////////////////////////////////////////////

struct Rect {
  int X1;
  int Y1;
  int W;
  int H;
  int VPW;
  int VPH;

  void PushOrtho(lev2::Context* context) const;
  void PopOrtho(lev2::Context* context) const;
};

///////////////////////////////////////////////////////////////////////////////

struct ItemDrawReq {
  synth* s;
  int ldi;
  int ienv;
  lyrdata_constptr_t ld;
  const Layer* l;
  ork::svar256_t _data;
  Rect rect;

  bool shouldCollectSample() const {
    return ((s->_lnoteframe >> 3) % 3 == 0);
  }
};

///////////////////////////////////////////////////////////////////////////////
struct HudPanel {
  ui::panel_ptr_t _uipanel;
  ui::surface_ptr_t _uisurface;
};
///////////////////////////////////////////////////////////////////////////////
struct ScopeBuffer {
  ScopeBuffer(int tbufindex = 0);
  float _samples[koscopelength];
  int _tbindex; // triple buffer index
};
///////////////////////////////////////////////////////////////////////////////
struct ScopeSource {
  void updateMono(int numframes, const float* mono);
  void updateStereo(int numframes, const float* left, const float* right);
  void updateController(const ControllerInst* controller);
  void connect(scopesink_ptr_t sink);
  void disconnect(scopesink_ptr_t sink);
  std::unordered_set<scopesink_ptr_t> _sinks;
  ScopeBuffer _scopebuffer;
  const ControllerInst* _controller = nullptr;
};
struct ScopeSink {
  void sourceUpdated(const ScopeSource& src);
  std::function<void(const ScopeSource&)> _onupdate;
};
struct SignalScope {
  void setRect(int iX, int iY, int iW, int iH, bool snap = false);
  hudpanel_ptr_t _hudpanel;
  scopesink_ptr_t _sink;
};
///////////////////////////////////////////////////////////////////////////////
signalscope_ptr_t create_oscilloscope(hudvp_ptr_t vp);
signalscope_ptr_t create_spectrumanalyzer(hudvp_ptr_t vp);
signalscope_ptr_t create_envelope_analyzer(hudvp_ptr_t vp);
///////////////////////////////////////////////////////////////////////////////
struct HudViewport final : public ui::Viewport {
  HudViewport();
  void DoDraw(ui::drawevent_constptr_t drwev) override;
  void onUpdateThreadTick(ui::updatedata_ptr_t updata);
  std::unordered_set<hudpanel_ptr_t> _hudpanels;
};

///////////////////////////////////////////////////////////////////////////////

void DrawEnv(lev2::Context* context, const ItemDrawReq& EDR);
void DrawAsr(lev2::Context* context, const ItemDrawReq& EDR);
void DrawLfo(lev2::Context* context, const ItemDrawReq& EDR);
void DrawFun(lev2::Context* context, const ItemDrawReq& EDR);
float FUNH(float vpw, float vph);
float FUNW(float vpw, float vph);
float FUNX(float vpw, float vph);
float ENVW(float vpw, float vph);
float ENVH(float vpw, float vph);
float ENVX(float vpw, float vph);
float DSPW(float vpw, float vph);
float DSPX(float vpw, float vph);
void DrawBorder(lev2::Context* context, int X1, int Y1, int X2, int Y2, int color = 0);

///////////////////////////////////////////////////////////////////////////////

static const float fontscale = 0.40;

} // namespace ork::audio::singularity
