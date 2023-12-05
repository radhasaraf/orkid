////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <math.h>
#include <ork/pch.h>
#include <ork/math/misc_math.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/material_pbr.inl>
#include <ork/lev2/gfx/gfxvtxbuf.inl>

///////////////////////////////////////////////////////////////////////////////

#define GRIDDIVS 32
#define NUM_GROUND_LINES 80
#define CONEDIVS 63
//#define CONEDIVS			10000
#define CONESIZE 1.0f

#define CIRCSEGS 32

namespace ork { namespace lev2 {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static const int inumgroundverts      = (NUM_GROUND_LINES * NUM_GROUND_LINES) * 6;
static const int NUM_CYLINDER_FACES   = 60;
static const int NUM_SPHERE_TRIANGLES = CIRCSEGS * (CIRCSEGS * 2);
static const int NUM_DOME_TRIANGLES   = 10 * 50 * 6;

GfxPrimitives::GfxPrimitives()
    : NoRttiSingleton<GfxPrimitives>()
    , mVtxBuf_Axis(6, 6)
    , mVtxBuf_AxisBox(36, 36)
    , mVtxBuf_AxisCone(1000, 400)
    , mVtxBuf_AxisLine(6 * 2 * 3, 6 * 2 * 3)

    , mVtxBuf_Box(6 * 2 * 3, 6 * 2 * 3)

    , mVtxBuf_Capsule((6 * NUM_CYLINDER_FACES + 2), (6 * NUM_CYLINDER_FACES + 2))
    , mVtxBuf_CircleStrip(1000, 1000)
    , mVtxBuf_CircleStripUI(1000, 1000)
    , mVtxBuf_CircleUI(1000, 1000)
    , mVtxBuf_Cone(1000, 400)
    , mVtxBuf_Cylinder(12 * NUM_CYLINDER_FACES + 2, 12 * NUM_CYLINDER_FACES + 2)

    , mVtxBuf_Diamond(3 * 6, 3 * 6)
    , mVtxBuf_DirCone(1000, 400)
    , mVtxBuf_Dome(NUM_DOME_TRIANGLES, NUM_DOME_TRIANGLES)

    , mVtxBuf_EQSphere(1024, 1024)
    , mVtxBuf_FullSphere(8192, 8192)
    , mVtxBuf_GridX100(1000, 400)
    , mVtxBuf_GroundPlane(6, 6)

    , mVtxBuf_PerlinTerrain(inumgroundverts, inumgroundverts)
    , mVtxBuf_SkySphere(32768, 32768) 
    , mVtxBuf_TriCircle(6 * (CIRCSEGS + 2), 6 * (CIRCSEGS + 2))

    , mVtxBuf_WireFrameBox(6 * 4 * 2, 6 * 4 * 2)
    , mVtxBuf_WireFrameCapsule(6 * 4 * 2, 6 * 4 * 2)
    , mVtxBuf_WireFrameCylinder(4 * NUM_CYLINDER_FACES + 2, 4 * NUM_CYLINDER_FACES + 2)
    , mVtxBuf_WireFrameDome(6 * (CIRCSEGS + 2), 6 * (CIRCSEGS + 2)) {
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::Init(Context* pTarg) {
  static bool binit = true;

  if (binit) {
    binit = false;
  } else
    return;

  GetRef().mMaterial.gpuInit(pTarg);
  GetRef().mMaterial.SetColorMode(ork::lev2::GfxMaterial3DSolid::EMODE_VERTEXMOD_COLOR);

  // orkprintf( "Inititializing Primitives\n" );

  fcolor4 Color0, Color1, Color2, Color3;

  ////////////////////////////////////////////////////
  // Axis

  F32 fLineSize = 1.0f;

  lev2::VtxWriter<SVtxV12N12B12T8C4> vwpbr;

  vwpbr.Lock(pTarg, &GetRef().mVtxBuf_Axis, 6);

  auto RED = fvec4::Red().ABGRU32();
  auto GRN = fvec4::Green().ABGRU32();
  auto BLU = fvec4::Blue().ABGRU32();
  vwpbr.AddVertex(SVtxV12N12B12T8C4(fvec3(0, 0, 0), fvec3(1, 0, 0), fvec3(0, 1, 0), fvec2(), RED));
  vwpbr.AddVertex(SVtxV12N12B12T8C4(fvec3(fLineSize, 0, 0), fvec3(1, 0, 0), fvec3(0, 1, 0), fvec2(), RED));
  vwpbr.AddVertex(SVtxV12N12B12T8C4(fvec3(0, 0, 0), fvec3(0, 1, 0), fvec3(1, 0, 0), fvec2(), GRN));
  vwpbr.AddVertex(SVtxV12N12B12T8C4(fvec3(0, fLineSize, 0), fvec3(0, 1, 0), fvec3(1, 0, 0), fvec2(), GRN));
  vwpbr.AddVertex(SVtxV12N12B12T8C4(fvec3(0, 0, 0), fvec3(0, 0, 1), fvec3(1, 0, 0), fvec2(), BLU));
  vwpbr.AddVertex(SVtxV12N12B12T8C4(fvec3(0, 0, fLineSize), fvec3(0, 0, 1), fvec3(1, 0, 0), fvec2(), BLU));

  vwpbr.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Grid

  lev2::VtxWriter<SVtxV12C4T16> vw;

  int iNumGridLines = GRIDDIVS;

  int icount = (iNumGridLines * 4) + 2;
  vw.Lock(pTarg, &GetRef().mVtxBuf_GridX100, icount);

  fLineSize = 100.0f;

  f32 fBas0 = -fLineSize;
  f32 fSca0 = (2.0f * fLineSize) / (F32)iNumGridLines;

  for (int i = 0; i < iNumGridLines; i++) {
    F32 fVal = fBas0 + (i * fSca0);

    U32 uColorX = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ABGRU32();
    U32 uColorZ = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ABGRU32();

    if (i == (GRIDDIVS >> 1)) {
      uColorX = fvec4(1.0f, 0.0f, 0.0f, 1.0f).ABGRU32();
      uColorZ = fvec4(0.0f, 0.0f, 1.0f, 1.0f).ABGRU32();
    }
    vw.AddVertex(SVtxV12C4T16(-fLineSize, 0.0f, fVal, 0, 0, uColorX));
    vw.AddVertex(SVtxV12C4T16(fLineSize, 0.0f, fVal, 0, 0, uColorX));
    vw.AddVertex(SVtxV12C4T16(fVal, 0.0f, -fLineSize, 0, 0, uColorZ));
    vw.AddVertex(SVtxV12C4T16(fVal, 0.0f, fLineSize, 0, 0, uColorZ));
  }

  vw.AddVertex(SVtxV12C4T16(0.0f, -fLineSize, 0.0f, 0, 0, fvec4(0.0f, 1.0f, 0.0f, 1.0f).ABGRU32()));
  vw.AddVertex(SVtxV12C4T16(0.0f, fLineSize, 0.0f, 0, 0, fvec4(0.0f, 1.0f, 0.0f, 1.0f).ABGRU32()));

  // SVtxV12C4T16 *pVTX = (SVtxV12C4T16*) GetRef().mVtxBuf_GridX100.GetVertexPointer();
  // SVtxV12C4T16 *pVTX0 = pVTX;

  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN); // pTarg->GBI()->UnLockVB( GetRef().mVtxBuf_GridX100 );

  ////////////////////////////////////////////////////
  // CircleStrip

  icount = (CONEDIVS * 6);
  vw.Lock(pTarg, &GetRef().mVtxBuf_CircleStrip, icount);

  U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();

  f32 fOuterSize = 1.0f;
  f32 fInnerSize = 0.85f;

  for (int i = 0; i < CONEDIVS; i++) {
    F32 fi  = i / (F32)CONEDIVS;
    F32 fi2 = (i + 1) / (F32)CONEDIVS;

    F32 fPhase  = PI2 * fi;
    F32 fPhase2 = PI2 * fi2;
    F32 fX0     = sinf(fPhase) * fOuterSize;
    F32 fZ0     = cosf(fPhase) * fOuterSize;
    F32 fX1     = sinf(fPhase2) * fOuterSize;
    F32 fZ1     = cosf(fPhase2) * fOuterSize;
    F32 fX2     = sinf(fPhase) * fInnerSize;
    F32 fZ2     = cosf(fPhase) * fInnerSize;
    F32 fX3     = sinf(fPhase2) * fInnerSize;
    F32 fZ3     = cosf(fPhase2) * fInnerSize;

    vw.AddVertex(SVtxV12C4T16(fX0, 0.0f, fZ0, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX3, 0.0f, fZ3, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(fX1, 0.0f, fZ1, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX0, 0.0f, fZ0, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX3, 0.0f, fZ3, 0, 0, uColor));
  }

  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN); //->GBI()->UnLockVB( GetRef().mVtxBuf_CircleStrip );

  ////////////////////////////////////////////////////
  // CircleStripUI

  vw.Lock(pTarg, &GetRef().mVtxBuf_CircleStripUI, CONEDIVS * 4);

  fOuterSize = 1.0f;
  fInnerSize = 0.5f;

  f32 fcenterx = 0.0f;
  f32 fcentery = 0.0f;

  for (int i = 0; i < CONEDIVS; i++) {
    F32 fi  = i / (F32)CONEDIVS;
    F32 fi2 = (i + 1) / (F32)CONEDIVS;

    F32 fPhase  = PI2 * fi;
    F32 fPhase2 = PI2 * fi2;
    F32 fX0     = fcenterx + sinf(fPhase) * fOuterSize;
    F32 fZ0     = fcentery + cosf(fPhase) * fOuterSize;
    F32 fX1     = fcenterx + sinf(fPhase2) * fOuterSize;
    F32 fZ1     = fcentery + cosf(fPhase2) * fOuterSize;
    F32 fX2     = fcenterx + sinf(fPhase) * fInnerSize;
    F32 fZ2     = fcentery + cosf(fPhase) * fInnerSize;
    F32 fX3     = fcenterx + sinf(fPhase2) * fInnerSize;
    F32 fZ3     = fcentery + cosf(fPhase2) * fInnerSize;

    f32 fC0 = 1.0f - (0.5f + cosf(fPhase) * 0.5f);
    f32 fC1 = 1.0f - (0.5f + cosf(fPhase2) * 0.5f);

    fC0 = 0.5f + (0.5f * fC0);
    fC1 = 0.5f + (0.5f * fC1);

    Color0.setXYZ(fC0, 0.0f, 0.0f);
    Color1.setXYZ(fC1, 0.0f, 0.0f);
    Color2.setXYZ(fC0, 0.0f, 1.0f);
    Color3.setXYZ(fC1, 0.0f, 1.0f);

    vw.AddVertex(SVtxV12C4T16(fX0, fZ0, 0.0f, 0, 0, Color2.ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX2, fZ2, 0.0f, 0, 0, Color0.ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX1, fZ1, 0.0f, 0, 0, Color3.ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX3, fZ3, 0.0f, 0, 0, Color1.ARGBU32()));
  }

  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // CircleUI

  vw.Lock(pTarg, &GetRef().mVtxBuf_CircleUI, CONEDIVS * 3);

  fOuterSize = 1.0f;

  fcenterx = 0.0f;
  fcentery = 0.0f;

  for (int i = 0; i < CONEDIVS; i++) {
    F32 fi  = i / (F32)CONEDIVS;
    F32 fi2 = (i + 1) / (F32)CONEDIVS;

    F32 fPhase  = PI2 * fi;
    F32 fPhase2 = PI2 * fi2;
    F32 fX0     = fcenterx + sinf(fPhase) * fOuterSize;
    F32 fZ0     = fcentery + cosf(fPhase) * fOuterSize;
    F32 fX1     = fcenterx + sinf(fPhase2) * fOuterSize;
    F32 fZ1     = fcentery + cosf(fPhase2) * fOuterSize;

    f32 fC0 = 0.5f + cosf(fPhase) * 0.5f;
    f32 fC1 = 0.5f + cosf(fPhase2) * 0.5f;
    f32 fCC = 0.5f;

    Color0.setXYZ(0.5f + (0.5f * fC0), 0.5f + (0.5f * fC0), 0.5f + (0.5f * fC0));
    Color1.setXYZ(0.5f + (0.5f * fC1), 0.5f + (0.5f * fC1), 0.5f + (0.5f * fC1));
    Color2.setXYZ(0.5f + (0.5f * fCC), 0.5f + (0.5f * fCC), 0.5f + (0.5f * fCC));

    vw.AddVertex(SVtxV12C4T16(fcenterx, fcentery, 0.0f, 0, 0, Color2.ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX1, fZ1, 0.0f, 0, 0, Color1.ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX0, fZ0, 0.0f, 0, 0, Color0.ARGBU32()));
    // GetRef().mVtxBuf_CircleStripUI.AddVertex( SVtxV12C4T16( fX3, fZ3, 0.0f, 0, 0, uColor ) );
  }

  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Cone

  vw.Lock(pTarg, &GetRef().mVtxBuf_Cone, CONEDIVS * 3);

  for (int i = 0; i < CONEDIVS; i++) {
    F32 fi      = i / (F32)CONEDIVS;
    F32 fi2     = (i + 1) / (F32)CONEDIVS;
    F32 fPhase  = PI2 * fi;
    F32 fPhase2 = PI2 * fi2;
    F32 fX      = sinf(fPhase) * CONESIZE;
    F32 fZ      = cosf(fPhase) * CONESIZE;
    F32 fX2     = sinf(fPhase2) * CONESIZE;
    F32 fZ2     = cosf(fPhase2) * CONESIZE;
    U32 uColor  = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();
    vw.AddVertex(SVtxV12C4T16(fX, 0.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.0f, CONESIZE, 0.0f, 0, 0, uColor));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // DirCone

  vw.Lock(pTarg, &GetRef().mVtxBuf_DirCone, CONEDIVS * 3);
  for (int i = 0; i < CONEDIVS; i++) {
    F32 fi                        = i / (F32)CONEDIVS;
    F32 fi2                       = (i + 1) / (F32)CONEDIVS;
    F32 fPhase                    = PI2 * fi;
    F32 fPhase2                   = PI2 * fi2;
    F32 fX                        = sinf(fPhase) * CONESIZE;
    F32 fZ                        = cosf(fPhase) * CONESIZE;
    F32 fX2                       = sinf(fPhase2) * CONESIZE;
    F32 fZ2                       = cosf(fPhase2) * CONESIZE;
    U32 uColor                    = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();
    static const float dirscaleXZ = 1.5f;
    vw.AddVertex(SVtxV12C4T16(fX * dirscaleXZ, 0.0f, fZ * dirscaleXZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2 * dirscaleXZ, 0.0f, fZ2 * dirscaleXZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.0f, CONESIZE * 20.0f, 0.0f, 0, 0, uColor));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Cylinder

  vw.Lock(pTarg, &GetRef().mVtxBuf_Cylinder, NUM_CYLINDER_FACES * 12);
  float baseAngle = PI2 / NUM_CYLINDER_FACES;
  for (int i = 0; i < NUM_CYLINDER_FACES; i++) {
    float angle  = baseAngle * i;
    float angle2 = baseAngle * (i + 1);

    F32 fX = cosf(angle);
    F32 fZ = sinf(angle);

    F32 fX2 = cosf(angle2);
    F32 fZ2 = sinf(angle2);

    U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();

    vw.AddVertex(SVtxV12C4T16(fX, 0.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX, 1.0f, fZ, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(fX, 1.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 1.0f, fZ2, 0, 0, uColor));

    // caps
    vw.AddVertex(SVtxV12C4T16(0.0f, 0.0f, 0.0f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX, 0.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.0f, 1.0f, 0.0f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX, 1.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 1.0f, fZ2, 0, 0, uColor));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Capsule is just a cylinder with no caps may go away
  vw.Lock(pTarg, &GetRef().mVtxBuf_Capsule, NUM_CYLINDER_FACES * 6);
  baseAngle = PI2 / NUM_CYLINDER_FACES;
  for (int i = 0; i < NUM_CYLINDER_FACES; i++) {
    float angle  = baseAngle * i;
    float angle2 = baseAngle * (i + 1);
    F32 fX       = cosf(angle);
    F32 fZ       = sinf(angle);
    F32 fX2      = cosf(angle2);
    F32 fZ2      = sinf(angle2);
    U32 uColor   = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();
    vw.AddVertex(SVtxV12C4T16(fX, 0.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX, 1.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX, 1.0f, fZ, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(fX2, 1.0f, fZ2, 0, 0, uColor));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Wired Dome

  vw.Lock(pTarg, &GetRef().mVtxBuf_WireFrameDome, CIRCSEGS * 6);
  F32 fiCIRCSEGS = (F32)CIRCSEGS;
  for (int i = 0; i < CIRCSEGS; i++) {
    F32 fPhase  = PI2 * i / fiCIRCSEGS;
    F32 fX      = sinf(fPhase);
    F32 fZ      = cosf(fPhase);
    F32 fPhase2 = PI2 * (i + 1) / fiCIRCSEGS;
    F32 fX2     = sinf(fPhase2);
    F32 fZ2     = cosf(fPhase2);
    vw.AddVertex(SVtxV12C4T16(fX, 0.0f, fZ, 0, 0, fcolor4::Green().ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, fcolor4::Green().ARGBU32()));
  }
  for (int i = 0; i < CIRCSEGS; i++) {
    F32 fPhase  = ((PI * i) + PI) / fiCIRCSEGS;
    F32 fX      = sinf(fPhase);
    F32 fZ      = cosf(fPhase);
    F32 fPhase2 = ((PI * (i + 1)) + PI) / fiCIRCSEGS;
    F32 fX2     = sinf(fPhase2);
    F32 fZ2     = cosf(fPhase2);
    vw.AddVertex(SVtxV12C4T16(fX, fZ, 0.0f, 0, 0, fcolor4::Blue().ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(fX2, fZ2, 0.0f, 0, 0, fcolor4::Blue().ARGBU32()));
  }
  for (int i = 0; i < CIRCSEGS; i++) {
    F32 fPhase  = PI * i / fiCIRCSEGS;
    F32 fX      = sinf(fPhase);
    F32 fZ      = cosf(fPhase);
    F32 fPhase2 = PI * (i + 1) / fiCIRCSEGS;
    F32 fX2     = sinf(fPhase2);
    F32 fZ2     = cosf(fPhase2);
    vw.AddVertex(SVtxV12C4T16(0.0f, fX, fZ, 0, 0, fcolor4::Red().ARGBU32()));
    vw.AddVertex(SVtxV12C4T16(0.0f, fX2, fZ2, 0, 0, fcolor4::Red().ARGBU32()));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Dome

  // caps
  const int kiUD = 50;
  const int kiVD = 10;
  // f32 fZ = 0.0f;

  const float kinvU    = 1.0f / float(kiUD);
  const float kinvV    = (0.5f / float(kiVD));
  const float kytopoff = 0.0f;
  const float kybotoff = -0.0f;

  vw.Lock(pTarg, &GetRef().mVtxBuf_Dome, kiUD * kiVD * 6);

  for (int iU = 0; iU < kiUD; iU++) {
    f32 fU1 = iU * kinvU;
    f32 fU2 = fU1 + kinvU;

    float fsinU1 = sinf(fU1 * PI2);
    float fsinU2 = sinf(fU2 * PI2);
    float fcosU1 = cosf(fU1 * PI2);
    float fcosU2 = cosf(fU2 * PI2);

    for (int iV = 0; iV < kiVD; iV++) {
      f32 fV1 = iV * kinvV;
      f32 fV2 = fV1 + kinvV;

      float fsinV1 = sinf(fV1 * PI);
      float fsinV2 = sinf(fV2 * PI);
      float fcosV1 = cosf(fV1 * PI);
      float fcosV2 = cosf(fV2 * PI);

      /////////////////////////
      // lower 2
      float fX1  = fsinU1 * fcosV1;
      float fY1  = fsinV1 + kytopoff;
      float fbY1 = -fsinV1 - kytopoff;
      float fZ1  = fcosU1 * fcosV1;

      float fX2  = fsinU2 * fcosV1;
      float fY2  = fsinV1 + kytopoff;
      float fbY2 = -fsinV1 - kytopoff;
      float fZ2  = fcosU2 * fcosV1;

      /////////////////////////
      // upper 2
      float fX3  = fsinU1 * fcosV2;
      float fY3  = fsinV2 + kytopoff;
      float fbY3 = -fsinV2 - kytopoff;
      float fZ3  = fcosU1 * fcosV2;

      float fX4  = fsinU2 * fcosV2;
      float fY4  = fsinV2 + kytopoff;
      float fbY4 = -fsinV2 - kytopoff;
      float fZ4  = fcosU2 * fcosV2;
      /////////////////////////
#if 0
			GetRef().mVtxBuf_Capsule.AddVertex( SVtxV12C4T16( fX1, fbY1, fZ1, 0.0f, 0.0f, 0x11111111 ) );
			GetRef().mVtxBuf_Capsule.AddVertex( SVtxV12C4T16( fX2, fbY2, fZ2, 0.0f, 0.0f, 0x44444444 ) );
			GetRef().mVtxBuf_Capsule.AddVertex( SVtxV12C4T16( fX3, fbY3, fZ3, 0.0f, 0.0f, 0x77777777 ) );

			GetRef().mVtxBuf_Capsule.AddVertex( SVtxV12C4T16( fX4, fbY4, fZ4, 0.0f, 0.0f, 0xaaaaaaaa ) );
			GetRef().mVtxBuf_Capsule.AddVertex( SVtxV12C4T16( fX3, fbY3, fZ3, 0.0f, 0.0f, 0xcccccccc ) );
			GetRef().mVtxBuf_Capsule.AddVertex( SVtxV12C4T16( fX2, fbY2, fZ2, 0.0f, 0.0f, 0xffffffff ) );
#else
      vw.AddVertex(SVtxV12C4T16(fX3, fY3, fZ3, 0.0f, 0.0f, 0x11111111));
      vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ2, 0.0f, 0.0f, 0x44444444));
      vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ1, 0.0f, 0.0f, 0x77777777));

      vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ2, 0.0f, 0.0f, 0xaaaaaaaa));
      vw.AddVertex(SVtxV12C4T16(fX3, fY3, fZ3, 0.0f, 0.0f, 0xcccccccc));
      vw.AddVertex(SVtxV12C4T16(fX4, fY4, fZ4, 0.0f, 0.0f, 0xffffffff));
#endif
    }
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Box

  vw.Lock(pTarg, &GetRef().mVtxBuf_Box, 12 * 3);
  {
    U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Axis Line

  lev2::VtxWriter<SVtxV12N12B12T8C4> vwp;

  using axis_vtx_t = SVtxV12N12B12T8C4;

  vwp.Lock(pTarg, &GetRef().mVtxBuf_AxisLine, 36);
  {
    U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();

    float width  = 0.05f;
    float length = 0.8f;
    fvec3 N, B;
    fvec2 UV;

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
  }
  vwp.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // Axis Cone
  vwp.Lock(pTarg, &GetRef().mVtxBuf_AxisCone, CONEDIVS * 3);
  for (int i = 0; i < CONEDIVS; i++) {
    F32 fi  = i / (F32)CONEDIVS;
    F32 fi2 = (i + 1) / (F32)CONEDIVS;

    F32 fPhase  = PI2 * fi;
    F32 fPhase2 = PI2 * fi2;
    F32 fX      = sinf(fPhase) / 20.0f;
    F32 fZ      = cosf(fPhase) / 20.0f;
    F32 fX2     = sinf(fPhase2) / 20.0f;
    F32 fZ2     = cosf(fPhase2) / 20.0f;

    U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();
    fvec3 N, B;
    fvec2 UV;

    vwp.AddVertex(axis_vtx_t(fvec3(fX, 15.0f / 20.0f, fZ), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(fX2, 15.0f / 20.0f, fZ2), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(0.0f, 20.0f / 20.0f, 0.0f), N, B, UV, uColor));
  }
  vwp.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  // sm - for trans manip pick buffer
  ////////////////////////////////////////////////////
  // Axis Box

  vwp.Lock(pTarg, &GetRef().mVtxBuf_AxisBox, 36);
  {
    U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();

    float width  = 0.1f;
    float length = 1.0f;

    fvec3 N, B;
    fvec2 UV;

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));

    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, 0, -width), N, B, UV, uColor));
    vwp.AddVertex(axis_vtx_t(fvec3(-width, length, -width), N, B, UV, uColor));
  }
  vwp.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  vw.Lock(pTarg, &GetRef().mVtxBuf_WireFrameBox, 24);
  {
    U32 uColor = fvec4(0.5f, 0.5f, 0.5f, 1.0f).ARGBU32();

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, 0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(0.5f, -0.5f, -0.5f, 0, 0, uColor));

    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, 0.5f, 0, 0, uColor));
    vw.AddVertex(SVtxV12C4T16(-0.5f, -0.5f, -0.5f, 0, 0, uColor));
  }
  vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  ////////////////////////////////////////////////////
  // TriCircle
  {
    vw.Lock(pTarg, &GetRef().mVtxBuf_TriCircle, CIRCSEGS * 6);

    F32 fiCIRCSEGS = (F32)CIRCSEGS;

    for (int i = 0; i < CIRCSEGS; i++) {
      F32 fPhase  = PI2 * i / fiCIRCSEGS;
      F32 fX      = sinf(fPhase);
      F32 fZ      = cosf(fPhase);
      F32 fPhase2 = PI2 * (i + 1) / fiCIRCSEGS;
      F32 fX2     = sinf(fPhase2);
      F32 fZ2     = cosf(fPhase2);
      vw.AddVertex(SVtxV12C4T16(fX, 0.0f, fZ, 0, 0, fcolor4::Green().ARGBU32()));
      vw.AddVertex(SVtxV12C4T16(fX2, 0.0f, fZ2, 0, 0, fcolor4::Green().ARGBU32()));
    }
    for (int i = 0; i < CIRCSEGS; i++) {
      F32 fPhase  = PI2 * i / fiCIRCSEGS;
      F32 fX      = sinf(fPhase);
      F32 fZ      = cosf(fPhase);
      F32 fPhase2 = PI2 * (i + 1) / fiCIRCSEGS;
      F32 fX2     = sinf(fPhase2);
      F32 fZ2     = cosf(fPhase2);
      vw.AddVertex(SVtxV12C4T16(fX, fZ, 0.0f, 0, 0, fcolor4::Blue().ARGBU32()));
      vw.AddVertex(SVtxV12C4T16(fX2, fZ2, 0.0f, 0, 0, fcolor4::Blue().ARGBU32()));
    }
    for (int i = 0; i < CIRCSEGS; i++) {
      F32 fPhase  = PI2 * i / fiCIRCSEGS;
      F32 fX      = sinf(fPhase);
      F32 fZ      = cosf(fPhase);
      F32 fPhase2 = PI2 * (i + 1) / fiCIRCSEGS;
      F32 fX2     = sinf(fPhase2);
      F32 fZ2     = cosf(fPhase2);
      vw.AddVertex(SVtxV12C4T16(0.0f, fX, fZ, 0, 0, fcolor4::Red().ARGBU32()));
      vw.AddVertex(SVtxV12C4T16(0.0f, fX2, fZ2, 0, 0, fcolor4::Red().ARGBU32()));
    }
    vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);
  }
  ////////////////////////////////////////////////////
  // GroundPlane
  {
    vw.Lock(pTarg, &GetRef().mVtxBuf_GroundPlane, 6);

    ///////////////////////////////////////////

    const U32 color = 0xffffffff;

    vw.AddVertex(SVtxV12C4T16(-1.0f, 0.0f, -1.0f, 0, 0, color));
    vw.AddVertex(SVtxV12C4T16(1.0f, 0.0f, -1.0f, 0, 0, color));
    vw.AddVertex(SVtxV12C4T16(1.0f, 0.0f, 1.0f, 0, 0, color));

    vw.AddVertex(SVtxV12C4T16(-1.0f, 0.0f, -1.0f, 0, 0, color));
    vw.AddVertex(SVtxV12C4T16(1.0f, 0.0f, 1.0f, 0, 0, color));
    vw.AddVertex(SVtxV12C4T16(-1.0f, 0.0f, 1.0f, 0, 0, color));

    vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

  } ////////////////////////////////////////////////////
  // Diamond
  {

    vw.Lock(pTarg, &GetRef().mVtxBuf_Diamond, 18);

    f32 fscale = 1.0f;

    f32 p0X = sinf(PI2 * 0.0f) * fscale;
    f32 p0Z = cosf(PI2 * 0.0f) * fscale;
    f32 p1X = sinf(PI2 * 0.33f) * fscale;
    f32 p1Z = cosf(PI2 * 0.33f) * fscale;
    f32 p2X = sinf(PI2 * 0.66f) * fscale;
    f32 p2Z = cosf(PI2 * 0.66f) * fscale;

    ///////////////////////////////////////////
    // Top Faces

    U32 ucolor = 0xffffffff;

    vw.AddVertex(SVtxV12C4T16(p0X, 0.0f, p0Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(p1X, 0.0f, p1Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(0.0f, fscale, 0.0f, 0, 0, ucolor));

    vw.AddVertex(SVtxV12C4T16(p1X, 0.0f, p1Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(p2X, 0.0f, p2Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(0.0f, fscale, 0.0f, 0, 0, ucolor));

    vw.AddVertex(SVtxV12C4T16(p2X, 0.0f, p2Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(p0X, 0.0f, p0Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(0.0f, fscale, 0.0f, 0, 0, ucolor));

    ///////////////////////////////////////////
    // Bottom Faces

    vw.AddVertex(SVtxV12C4T16(p0X, 0.0f, p0Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(p1X, 0.0f, p1Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(0.0f, -fscale, 0.0f, 0, 0, ucolor));

    vw.AddVertex(SVtxV12C4T16(p1X, 0.0f, p1Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(p2X, 0.0f, p2Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(0.0f, -fscale, 0.0f, 0, 0, ucolor));

    vw.AddVertex(SVtxV12C4T16(p2X, 0.0f, p2Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(p0X, 0.0f, p0Z, 0, 0, ucolor));
    vw.AddVertex(SVtxV12C4T16(0.0f, -fscale, 0.0f, 0, 0, ucolor));

    vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);
  }
  ////////////////////////////////////////////////////
  // EQSphere (even dispersion of points across sphere, for normal compression )
  ////////////////////////////////////////////////////

  {

    f32 fiCIRCSEGS = (F32)256;
    f32 famp       = 50.0f;

    int iringdiv  = 1;
    int ipaccum   = 0;
    int inumrings = 21;

    u16 normtabX[256];
    u16 normtabY[256];
    u16 normtabZ[256];

    vw.Lock(pTarg, &GetRef().mVtxBuf_EQSphere, inumrings * inumrings);

    for (int iring = 0; iring < inumrings; iring++) {
      f32 fringdivphase = ((f32)iring) / ((f32)inumrings - 1);

      f32 fringdiv = (1.0f - (0.5f + 0.5f * cosf(fringdivphase * PI2)));

      fringdiv = sqrtf(fringdiv);

      iringdiv = (int)(fringdiv * 20.5f);

      iringdiv = (iringdiv < 1) ? 1 : iringdiv;

      // orkprintf( "[iring %03d] [iringdiv %02d] [ipaccum %02d]\n", iring, iringdiv, ipaccum );

      // f32 fY = (fringdivphase-0.5f)*famp*2.0f;
      f32 fY   = cosf(fringdivphase * PI);
      f32 fmul = sinf(fringdivphase * PI);

      // orkprintf( "///////////////////////////////\n [ring %d] \n\n", iring );

      for (int iv = 0; iv < iringdiv; iv++) {
        f32 fphase = ((f32)iv) / ((f32)iringdiv);

        F32 fX = sinf(fphase * PI2) * fmul;
        F32 fZ = cosf(fphase * PI2) * fmul;

        u32 ucolor = 0xff000000 | (((u32)ipaccum) << 8);

        // orkprintf( "[idx %03d] [%1.4f %1.4f %1.4f]\n", ipaccum, fX, fY, fZ );

        vw.AddVertex(SVtxV12C4T16(fX * famp, fY * famp, fZ * famp, 0, 0, ucolor));

        s16 sX = (s16)(fX * 32767.0f);
        s16 sY = (s16)(fY * 32767.0f);
        s16 sZ = (s16)(fZ * 32767.0f);

        normtabX[ipaccum] = *reinterpret_cast<u16*>(&sX);
        normtabY[ipaccum] = *reinterpret_cast<u16*>(&sY);
        normtabZ[ipaccum] = *reinterpret_cast<u16*>(&sZ);

        ipaccum++;
      }
    }
    vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);
  }

  ////////////////////////////////////////////////////
  // FullSphere
  ////////////////////////////////////////////////////

  {
    //////////////////////////////////////////////
    // compute points on sphere
    //////////////////////////////////////////////
    const int inumrings = 36;
    typedef std::vector<fvec3> ring_t;
    std::vector<ring_t> _rings;
    for (int U = 0; U < inumrings; U++) {
      float fringdivphase = ((float)U) / ((float)inumrings - 1);
      float y             = cosf(fringdivphase * PI);
      float fmul          = sinf(fringdivphase * PI);
      ring_t outring;
      for (int V = 0; V < inumrings; V++) {
        float fphase = ((float)V) / ((float)inumrings);
        F32 x        = sinf(fphase * PI2) * fmul;
        F32 z        = cosf(fphase * PI2) * fmul;
        fvec3 vertex = fvec3(x, y, z);
        outring.push_back(vertex);
      }
      _rings.push_back(outring);
    }
    //////////////////////////////////////////////
    // generate primitive
    //////////////////////////////////////////////
    size_t numvtx  = (inumrings * inumrings * 6);
    uint32_t color = 0xffffffff;
    vw.Lock(pTarg, &GetRef().mVtxBuf_FullSphere, numvtx);
    for (int U = 0; U < inumrings; U++) {
      const auto& RingA = _rings[U];
      const auto& RingB = _rings[(U + 1) % _rings.size()];
      for (int V = 0; V < inumrings; V++) {
        const auto& VA = RingA[V];
        const auto& VB = RingA[(V + 1) % inumrings];
        const auto& VC = RingB[V];
        const auto& VD = RingB[(V + 1) % inumrings];

        vw.AddVertex(SVtxV12C4T16(VA, fvec2(), color));
        vw.AddVertex(SVtxV12C4T16(VB, fvec2(), color));
        vw.AddVertex(SVtxV12C4T16(VC, fvec2(), color));

        vw.AddVertex(SVtxV12C4T16(VB, fvec2(), color));
        vw.AddVertex(SVtxV12C4T16(VD, fvec2(), color));
        vw.AddVertex(SVtxV12C4T16(VC, fvec2(), color));

        numvtx += 6;
      }
    }
    // printf("numvtx<%zu>\n", numvtx);
    vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);
    //////////////////////////////////////////////
  }

  ////////////////////////////////////////////////////
  // SkySphere (portion of sphere directly infront of camera)

  {

    const int kiUD = 100;
    const int kiVD = 20;
    // float fZ = 0.0f;

    vw.Lock(pTarg, &GetRef().mVtxBuf_SkySphere, kiUD * kiVD * 6);

    const float kinvU = 1.0f / float(kiUD);
    const float kinvV = (0.5f / float(kiVD));

    for (int iU = 0; iU < kiUD; iU++) {
      f32 fU1 = iU * kinvU;
      f32 fU2 = fU1 + kinvU;

      float fsinU1 = sinf(fU1 * PI2);
      float fsinU2 = sinf(fU2 * PI2);
      float fcosU1 = cosf(fU1 * PI2);
      float fcosU2 = cosf(fU2 * PI2);

      for (int iV = 0; iV < kiVD; iV++) {
        f32 fV1 = iV * kinvV;
        f32 fV2 = fV1 + kinvV;

        float fsinV1 = sinf(fV1 * PI);
        float fsinV2 = sinf(fV2 * PI);
        float fcosV1 = cosf(fV1 * PI);
        float fcosV2 = cosf(fV2 * PI);

        /////////////////////////
        // lower 2
        float fX1  = fsinU1 * fcosV1;
        float fY1  = fsinV1;
        float fZ1  = fcosU1 * fcosV1;
        float fuU1 = (fX1 + 1.0f) * 0.5f;
        float fuV1 = (fZ1 + 1.0f) * 0.5f;

        float fX2  = fsinU2 * fcosV1;
        float fY2  = fsinV1;
        float fZ2  = fcosU2 * fcosV1;
        float fuU2 = (fX2 + 1.0f) * 0.5f;
        float fuV2 = (fZ2 + 1.0f) * 0.5f;

        /////////////////////////
        // upper 2
        float fX3  = fsinU1 * fcosV2;
        float fY3  = fsinV2;
        float fZ3  = fcosU1 * fcosV2;
        float fuU3 = (fX3 + 1.0f) * 0.5f;
        float fuV3 = (fZ3 + 1.0f) * 0.5f;

        float fX4  = fsinU2 * fcosV2;
        float fY4  = fsinV2;
        float fZ4  = fcosU2 * fcosV2;
        float fuU4 = (fX4 + 1.0f) * 0.5f;
        float fuV4 = (fZ4 + 1.0f) * 0.5f;
        /////////////////////////

        vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ1, fuU1, fuV1, 0x11111111));
        vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ2, fuU2, fuV2, 0x44444444));
        vw.AddVertex(SVtxV12C4T16(fX3, fY3, fZ3, fuU3, fuV3, 0x77777777));

        vw.AddVertex(SVtxV12C4T16(fX4, fY4, fZ4, fuU4, fuV4, 0xaaaaaaaa));
        vw.AddVertex(SVtxV12C4T16(fX3, fY3, fZ3, fuU3, fuV3, 0xcccccccc));
        vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ2, fuU2, fuV2, 0xffffffff));
      }
    }

    vw.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

    GetRef().mVtxBuf_SkySphere.SetNumVertices(kiUD * kiVD * 6);
  }

  ////////////////////////////////////////////////////
  // mVtxBuf_PerlinTerrain
  {

    const int iNumGroundLines = NUM_GROUND_LINES;

    f32 fLineSize        = 100.0f;
    f32 fnoisespacescale = 10.0f;

    f32 fBas0 = -fLineSize;
    f32 fSca0 = (2.0f * fLineSize) / (F32)iNumGroundLines;

    f32 fBias     = 1.1021f;
    f32 fAmp      = 15.0f;
    f32 fFrq      = 0.023f;
    f32 fAmpScale = 0.75f;
    f32 fFrqScale = 1.3f;

    int inumoctaves = 24;

    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////
    // Compute Heightfield

    float fmin = std::numeric_limits<float>::max();
    float fmax = -std::numeric_limits<float>::max();

    struct sheightfield {
      const int minumgl;
      fvec3* mpxyz;
      fvec2* mpuv;

      sheightfield(int igl)
          : minumgl(igl) {
        mpxyz = new fvec3[igl * igl];
        mpuv  = new fvec2[igl * igl];
      }

      fvec3& XYZ(int ix, int iy) {
        return mpxyz[ix * minumgl + iy];
      }
      fvec2& UV(int ix, int iy) {
        return mpuv[ix * minumgl + iy];
      }

      fvec3 Normal(int ix1, int iz1) {
        int ix0 = (ix1 - 1);
        if (ix0 < 0)
          ix0 += minumgl;
        int iz0 = (iz1 - 1);
        if (iz0 < 0)
          iz0 += minumgl;
        int ix2 = (ix1 + 1) % minumgl;
        int iz2 = (iz1 + 1) % minumgl;

        fvec3 d0 = XYZ(ix0, iz0) - XYZ(ix1, iz1);
        fvec3 d1 = XYZ(ix1, iz0) - XYZ(ix1, iz1);
        fvec3 d2 = XYZ(ix2, iz0) - XYZ(ix1, iz1);
        fvec3 d3 = XYZ(ix2, iz1) - XYZ(ix1, iz1);
        fvec3 d4 = XYZ(ix2, iz2) - XYZ(ix1, iz1);
        fvec3 d5 = XYZ(ix1, iz2) - XYZ(ix1, iz1);
        fvec3 d6 = XYZ(ix0, iz2) - XYZ(ix1, iz1);
        fvec3 d7 = XYZ(ix0, iz1) - XYZ(ix1, iz1);

        fvec3 c0 = d0.crossWith(d1);
        fvec3 c1 = d1.crossWith(d2);
        fvec3 c2 = d2.crossWith(d3);
        fvec3 c3 = d3.crossWith(d4);
        fvec3 c4 = d4.crossWith(d5);
        fvec3 c5 = d5.crossWith(d6);
        fvec3 c6 = d6.crossWith(d7);
        fvec3 c7 = d7.crossWith(d0);

        fvec3 vdx = (c0 + c1 + c2 + c3 + c4 + c5 + c6 + c7).normalized();
        return vdx;
      }
    };

    sheightfield heightfield(iNumGroundLines);

    for (int iX = 0; iX < iNumGroundLines; iX++) {
      F32 fx = fBas0 + ((F32)iX * fSca0);

      F32 fu = (F32)iX / (F32)iNumGroundLines;

      for (int iZ = 0; iZ < iNumGroundLines; iZ++) {
        F32 fz = fBas0 + ((F32)iZ * fSca0);

        F32 fv = (F32)iZ / (F32)iNumGroundLines;

        f32 fy = 0.0f;

        for (int iOctave = 0; iOctave < inumoctaves; iOctave++) {
          f32 fascal = fAmp * powf(fAmpScale, (f32)(iOctave));
          f32 ffscal = fFrq * powf(fFrqScale, (f32)(iOctave));

          fy += math::Perlin2D::PlaneNoiseFunc(fx, fz, 0.0f, 0.0f, fascal, ffscal);
        }

        if (float(fy) < fmin)
          fmin = float(fy);
        if (float(fy) > fmax)
          fmax = float(fy);

        heightfield.XYZ(iX, iZ).x = (fx);
        heightfield.XYZ(iX, iZ).y = (fy);
        heightfield.XYZ(iX, iZ).z = (fz);

        heightfield.UV(iX, iZ).x = (fu);
        heightfield.UV(iX, iZ).y = (fv);
      }
    }

    float frange = fmax - fmin;

    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////
    // Detail Heightfield

    fvec4 ColorA(0.2f, 0.3f, 0.1f, 1.0f);
    fvec4 ColorB(0.4f, 0.5f, 0.3f, 1.0f);
    fvec4 ColorC(1.0f, 1.0f, 1.0f, 1.0f);

    vwp.Lock(pTarg, &GetRef().mVtxBuf_PerlinTerrain, (iNumGroundLines - 1) * (iNumGroundLines - 1) * 6);

    for (int iX1 = 0; iX1 < iNumGroundLines - 1; iX1++) {
      int iX2 = (iX1 + 1) % iNumGroundLines;

      for (int iZ1 = 0; iZ1 < iNumGroundLines - 1; iZ1++) {
        int iZ2 = (iZ1 + 1) % iNumGroundLines;

        const fvec3& pX1Z1 = heightfield.XYZ(iX1, iZ1);
        const fvec3& pX2Z1 = heightfield.XYZ(iX2, iZ1);
        const fvec3& pX1Z2 = heightfield.XYZ(iX1, iZ2);
        const fvec3& pX2Z2 = heightfield.XYZ(iX2, iZ2);

        const fvec2& tX1Z1 = heightfield.UV(iX1, iZ1);
        const fvec2& tX2Z1 = heightfield.UV(iX2, iZ1);
        const fvec2& tX1Z2 = heightfield.UV(iX1, iZ2);
        const fvec2& tX2Z2 = heightfield.UV(iX2, iZ2);

        /////////////////////////////////////////////////////

        float fX1 = pX1Z1.x;
        float fX2 = pX2Z1.x;
        float fZ1 = pX1Z1.z;
        float fZ2 = pX1Z2.z;

        float fTU1 = tX1Z1.x;
        float fTU2 = tX2Z1.x;
        float fTV1 = tX1Z1.y;
        float fTV2 = tX1Z2.y;

        /////////////////////////////////////////////////////
        // get heights

        F32 fYH_X1Z1 = pX1Z1.y;
        F32 fYH_X1Z2 = pX1Z2.y;
        F32 fYH_X2Z1 = pX2Z1.y;
        F32 fYH_X2Z2 = pX2Z2.y;

        /////////////////////////////////////////////////////
        // calc height lerps

        float YLerpX1Z1 = powf((float(fYH_X1Z1) - fmin) / frange, 2.0f);
        float YLerpX2Z1 = powf((float(fYH_X2Z1) - fmin) / frange, 2.0f);
        float YLerpX2Z2 = powf((float(fYH_X2Z2) - fmin) / frange, 2.0f);
        float YLerpX1Z2 = powf((float(fYH_X1Z2) - fmin) / frange, 2.0f);

        /////////////////////////////////////////////////////
        // calc normals

        fvec3 nX1Z1 = heightfield.Normal(iX1, iZ1);
        fvec3 nX2Z1 = heightfield.Normal(iX2, iZ1);
        fvec3 nX1Z2 = heightfield.Normal(iX1, iZ2);
        fvec3 nX2Z2 = heightfield.Normal(iX2, iZ2);

        fvec3 Up(0.0f, 1.0f, 0.0f);
        float DotX1Z1 = powf(Up.dotWith(nX1Z1), 6.0f);
        float DotX2Z1 = powf(Up.dotWith(nX2Z1), 6.0f);
        float DotX1Z2 = powf(Up.dotWith(nX1Z2), 6.0f);
        float DotX2Z2 = powf(Up.dotWith(nX2Z2), 6.0f);

        /////////////////////////////////////////////////////

        fvec4 caX1Z1, caX2Z1, caX2Z2, caX1Z2;
        fvec4 cX1Z1, cX2Z1, cX2Z2, cX1Z2;
        caX1Z1.lerp(ColorB, ColorA, DotX1Z1);
        caX2Z1.lerp(ColorB, ColorA, DotX2Z1);
        caX2Z2.lerp(ColorB, ColorA, DotX2Z2);
        caX1Z2.lerp(ColorB, ColorA, DotX1Z2);

        cX1Z1.lerp(caX1Z1, ColorC, YLerpX1Z1);
        cX2Z1.lerp(caX2Z1, ColorC, YLerpX2Z1);
        cX2Z2.lerp(caX2Z2, ColorC, YLerpX2Z2);
        cX1Z2.lerp(caX1Z2, ColorC, YLerpX1Z2);

        /////////////////////////////////////////////////////

        vwp.AddVertex(SVtxV12N12B12T8C4(fvec3(fX1, fYH_X1Z1, fZ1), nX1Z1, nX1Z1, fvec2(fTU1, fTV1), pTarg->fcolor4ToU32(cX1Z1)));
        vwp.AddVertex(SVtxV12N12B12T8C4(fvec3(fX1, fYH_X1Z2, fZ2), nX1Z2, nX1Z2, fvec2(fTU1, fTV2), pTarg->fcolor4ToU32(cX1Z2)));
        vwp.AddVertex(SVtxV12N12B12T8C4(fvec3(fX2, fYH_X2Z2, fZ2), nX2Z2, nX2Z2, fvec2(fTU2, fTV2), pTarg->fcolor4ToU32(cX2Z2)));

        vwp.AddVertex(SVtxV12N12B12T8C4(fvec3(fX1, fYH_X1Z1, fZ1), nX1Z1, nX1Z1, fvec2(fTU1, fTV1), pTarg->fcolor4ToU32(cX1Z1)));
        vwp.AddVertex(SVtxV12N12B12T8C4(fvec3(fX2, fYH_X2Z2, fZ2), nX2Z2, nX2Z2, fvec2(fTU2, fTV2), pTarg->fcolor4ToU32(cX2Z2)));
        vwp.AddVertex(SVtxV12N12B12T8C4(fvec3(fX2, fYH_X2Z1, fZ1), nX2Z1, nX2Z1, fvec2(fTU2, fTV1), pTarg->fcolor4ToU32(cX2Z1)));
      }
    }
    vwp.UnLock(pTarg, EULFLG_ASSIGNVBLEN);

    // GetRef().mVtxBuf_GroundPlane.AddVertex( SVtxV12C4T16( 0.0f, -fLineSize, 0.0f, 0, 0, 0xff00ff00 ) );
    // GetRef().mVtxBuf_GroundPlane.AddVertex( SVtxV12C4T16( 0.0f, fLineSize, 0.0f, 0, 0, 0xff00ff00 ) );
  }
  ////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderAxis(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(default3DMaterial(pTarg).get(), GetRef().mVtxBuf_Axis,PrimitiveType::LINES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderGridX100(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_GridX100,PrimitiveType::LINES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderCone(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Cone,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderDirCone(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_DirCone,PrimitiveType::TRIANGLES);
}
///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderCircleStrip(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_CircleStrip,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderCircleUI(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_CircleUI,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderCircleStripUI(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_CircleStripUI,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderTriCircle(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_TriCircle,PrimitiveType::LINES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderDiamond(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Diamond,PrimitiveType::TRIANGLES);
}

void GfxPrimitives::RenderCylinder(Context* pTarg, bool drawoutline) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Cylinder,PrimitiveType::TRIANGLES);
}

void GfxPrimitives::RenderCapsule(Context* pTarg, float radius) {
  fmtx4 MatScale, MatTrans, MatRotate;

  MatScale.scale(radius, radius, radius);

  // Top dome
  fvec3 trans(0.0f, 1.0f, 0.0f);
  trans = trans.transform(pTarg->MTXI()->RefMMatrix());
  MatTrans.setTranslation(trans);

  pTarg->MTXI()->PushMMatrix(fmtx4::multiply_ltor(MatScale,MatRotate,MatTrans));
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Dome,PrimitiveType::TRIANGLES);
  pTarg->MTXI()->PopMMatrix();

  // Bottom dome
  MatRotate.rotateOnZ(PI);
  MatTrans.setTranslation(pTarg->MTXI()->RefMMatrix().translation());
  pTarg->MTXI()->PushMMatrix(fmtx4::multiply_ltor(MatScale,MatRotate,MatTrans));
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Dome,PrimitiveType::TRIANGLES);
  pTarg->MTXI()->PopMMatrix();

  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Capsule,PrimitiveType::TRIANGLES);
}

void GfxPrimitives::RenderBox(Context* pTarg, bool drawoutline) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_Box,PrimitiveType::TRIANGLES);
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_WireFrameBox,PrimitiveType::LINES);
}

void GfxPrimitives::RenderAxisLineCone(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_AxisCone,PrimitiveType::TRIANGLES);
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_AxisLine,PrimitiveType::TRIANGLES);
}

void GfxPrimitives::RenderAxisBox(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_AxisBox,PrimitiveType::TRIANGLES);
}

/*
void GfxPrimitives::RenderHalfSphere( Context *pTarg )
{
    pTarg->GBI()->DrawPrimitive( GetRef().mVtxBuf_Dome );
}
*/

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderEQSphere(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_EQSphere,PrimitiveType::POINTS);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderSkySphere(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_SkySphere,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderGroundPlane(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_GroundPlane,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderPerlinTerrain(Context* pTarg) {
  pTarg->GBI()->DrawPrimitive(&GetRef().mMaterial, GetRef().mVtxBuf_PerlinTerrain,PrimitiveType::TRIANGLES);
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderOrthoQuad(
    Context* pTarg,
    f32 fX1,
    f32 fX2,
    f32 fY1,
    f32 fY2,
    f32 iminU,
    f32 imaxU,
    f32 iminV,
    f32 imaxV) {
  auto vb = &GfxEnv::GetSharedDynamicVB();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTarg, vb, 6);

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, 0.0f, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY1, 0.0f, imaxU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, 0.0f, imaxU, imaxV, 0xffffffff));

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, 0.0f, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, 0.0f, imaxU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX1, fY2, 0.0f, iminU, imaxV, 0xffffffff));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////
  // Render Using Ortho Matrix

  fmtx4 MatTrans, MatScale;
  MatTrans.translate(-1.0f, 1.0f, 0.0f);
  MatScale.scale(2.0f, -2.0f, 0.0f);
  // MatTrans.setTranslation( -1.0f, 1.0f, 0.0f );
  // MatScale.scale( 2.0f, -2.0f, 0.0f );

  fmtx4 OrthoMat = pTarg->MTXI()->GetUIOrthoProjectionMatrix();
  pTarg->MTXI()->PushPMatrix(OrthoMat);
  pTarg->MTXI()->PushVMatrix(fmtx4::multiply_ltor(MatTrans,MatScale));
  pTarg->MTXI()->PushMMatrix(fmtx4::Identity());
  {
    // pTarg->GBI()->DrawPrimitive(vw, PrimitiveType::TRIANGLES);
  }
  pTarg->MTXI()->PopPMatrix(); // back to ortho
  pTarg->MTXI()->PopVMatrix(); // back to ortho
  pTarg->MTXI()->PopMMatrix(); // back to ortho

  ///////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::
    RenderQuadAtX(Context* pTarg, f32 fY1, f32 fY2, f32 fZ1, f32 fZ2, f32 fX, f32 iminU, f32 imaxU, f32 iminV, f32 imaxV) {
  auto vb = &GfxEnv::GetSharedDynamicVB();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTarg, vb, 6);

  vw.AddVertex(SVtxV12C4T16(fX, fY1, fZ1, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX, fY2, fZ1, imaxU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX, fY2, fZ2, imaxU, imaxV, 0xffffffff));

  vw.AddVertex(SVtxV12C4T16(fX, fY1, fZ1, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX, fY2, fZ2, imaxU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX, fY1, fZ2, iminU, imaxV, 0xffffffff));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////

  // pTarg->GBI()->DrawPrimitive(vw, PrimitiveType::TRIANGLES);

  ///////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::
    RenderQuadAtY(Context* pTarg, f32 fX1, f32 fX2, f32 fZ1, f32 fZ2, f32 fY, f32 iminU, f32 imaxU, f32 iminV, f32 imaxV) {
  auto vb = &GfxEnv::GetSharedDynamicVB();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTarg, vb, 6);

  vw.AddVertex(SVtxV12C4T16(fX2, fY, fZ2, imaxU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY, fZ1, imaxU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX1, fY, fZ1, iminU, iminV, 0xffffffff));

  vw.AddVertex(SVtxV12C4T16(fX1, fY, fZ2, iminU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY, fZ2, imaxU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX1, fY, fZ1, iminU, iminV, 0xffffffff));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////

  // pTarg->GBI()->DrawPrimitive(vw, PrimitiveType::TRIANGLES);

  ///////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderQuadAtZ(
    GfxMaterial* mtl,
    Context* pTarg,
    f32 fX1,
    f32 fX2,
    f32 fY1,
    f32 fY2,
    f32 fZ,
    f32 iminU,
    f32 imaxU,
    f32 iminV,
    f32 imaxV) {
  auto vb = &GfxEnv::GetSharedDynamicVB();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTarg, vb, 6);

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY1, fZ, imaxU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ, imaxU, imaxV, 0xffffffff));

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ, imaxU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(fX1, fY2, fZ, iminU, imaxV, 0xffffffff));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////

  pTarg->GBI()->DrawPrimitive(mtl, vw, PrimitiveType::TRIANGLES);

  ///////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderQuadAtZV16T16C16(
    GfxMaterial* mtl,
    Context* pTarg,
    f32 fX1,
    f32 fX2,
    f32 fY1,
    f32 fY2,
    f32 fZ,
    f32 iminU,
    f32 imaxU,
    f32 iminV,
    f32 imaxV) {
  auto vb = &GfxEnv::GetSharedDynamicV16T16C16();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV16T16C16> vw;
  vw.Lock(pTarg, vb, 6);

  vw.AddVertex(SVtxV16T16C16(fvec4(fX1, fY1, fZ), fvec4(1, 1, 1, 1), fvec4(iminU, iminV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX2, fY1, fZ), fvec4(1, 1, 1, 1), fvec4(imaxU, iminV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX2, fY2, fZ), fvec4(1, 1, 1, 1), fvec4(imaxU, imaxV, 0, 0)));

  vw.AddVertex(SVtxV16T16C16(fvec4(fX1, fY1, fZ), fvec4(1, 1, 1, 1), fvec4(iminU, iminV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX2, fY2, fZ), fvec4(1, 1, 1, 1), fvec4(imaxU, imaxV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX1, fY2, fZ), fvec4(1, 1, 1, 1), fvec4(iminU, imaxV, 0, 0)));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////

  pTarg->GBI()->DrawPrimitive(mtl, vw, PrimitiveType::TRIANGLES);

  ///////////////////////////////////////////
}
void GfxPrimitives::RenderEMLQuadAtZV16T16C16(
    Context* pTarg,
    f32 fX1,
    f32 fX2,
    f32 fY1,
    f32 fY2,
    f32 fZ,
    f32 iminU,
    f32 imaxU,
    f32 iminV,
    f32 imaxV) {
  auto vb = &GfxEnv::GetSharedDynamicV16T16C16();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV16T16C16> vw;
  vw.Lock(pTarg, vb, 6);

  vw.AddVertex(SVtxV16T16C16(fvec4(fX1, fY1, fZ), fvec4(1, 1, 1, 1), fvec4(iminU, iminV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX2, fY1, fZ), fvec4(1, 1, 1, 1), fvec4(imaxU, iminV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX2, fY2, fZ), fvec4(1, 1, 1, 1), fvec4(imaxU, imaxV, 0, 0)));

  vw.AddVertex(SVtxV16T16C16(fvec4(fX1, fY1, fZ), fvec4(1, 1, 1, 1), fvec4(iminU, iminV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX2, fY2, fZ), fvec4(1, 1, 1, 1), fvec4(imaxU, imaxV, 0, 0)));
  vw.AddVertex(SVtxV16T16C16(fvec4(fX1, fY2, fZ), fvec4(1, 1, 1, 1), fvec4(iminU, imaxV, 0, 0)));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////

  pTarg->GBI()->DrawPrimitiveEML(vw, PrimitiveType::TRIANGLES);

  ///////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

void GfxPrimitives::RenderQuad(Context* pTarg, fvec4& V0, fvec4& V1, fvec4& V2, fvec4& V3) {
  auto vb = &GfxEnv::GetSharedDynamicVB();

  ///////////////////////////////////////////
  // SET VERTICES (range 0..1)

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTarg, vb, 6);

  f32 iminU = 0.0f;
  f32 iminV = 0.0f;
  f32 imaxU = 1.0f;
  f32 imaxV = 1.0f;

  vw.AddVertex(SVtxV12C4T16(V0.x, V0.y, V0.z, iminU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(V1.x, V1.y, V1.z, imaxU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(V2.x, V2.y, V2.z, iminU, imaxV, 0xffffffff));

  vw.AddVertex(SVtxV12C4T16(V1.x, V1.y, V1.z, imaxU, iminV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(V2.x, V2.y, V2.z, iminU, imaxV, 0xffffffff));
  vw.AddVertex(SVtxV12C4T16(V3.x, V3.y, V3.z, imaxU, imaxV, 0xffffffff));

  vw.UnLock(pTarg);

  ///////////////////////////////////////////

  // pTarg->GBI()->DrawPrimitive(vw, PrimitiveType::TRIANGLES);

  ///////////////////////////////////////////
}
}} // namespace ork::lev2
