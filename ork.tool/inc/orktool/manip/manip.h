////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/kernel/core/singleton.h>
#include <ork/lev2/gfx/camera/uicam.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/util/grid.h>
#include <ork/math/TransformNode.h>
#include <ork/object/AutoConnector.h>
#include <ork/rtti/RTTI.h>

namespace ork { namespace lev2 {

class ManipManager;

////////////////////////////////////////////////////////////////////////////////

struct IntersectionRecord {
  fvec3 mIntersectionPoint;
  fvec3 mOldIntersectionPoint;
  fvec3 mBaseIntersectionPoint;
  bool mbHasItersected;

  IntersectionRecord();

  fvec4 GetLocalSpaceDelta(const fmtx4& InvLocalMatrix);
};

enum EPlaneRec {
  EPLANE_XZ = 0,
  EPLANE_YZ,
  EPLANE_XY,
  EPLANE_END,
};

class Manip : public ork::Object {
  DECLARE_TRANSPARENT_CUSTOM_POLICY_RTTI(Manip, ork::Object, ork::rtti::AbstractPolicy)

public:
  fmtx4 InvMatrix;
  TransformNode mBaseTransform;

  IntersectionRecord mIntersection[EPLANE_END];
  IntersectionRecord* mActiveIntersection;

  fplane3 mPlaneXZ;
  fplane3 mPlaneYZ;
  fplane3 mPlaneXY;

  ManipManager& mManager;

  Manip(ManipManager& mgr);

  virtual void Draw(Context* pTARG) const        = 0;
  virtual bool UIEventHandler(const ui::Event& EV) = 0;

  fvec3 IntersectWithPlanes(const ork::fvec2& posubp);
  void SelectBestPlane(const ork::fvec2& posubp);
  void CalcPlanes();

  bool CheckIntersect(void) const;

  fcolor4 GetColor() const { return mColor; }

protected:
  fcolor4 mColor;
};

class ManipTrans : public Manip {
  RttiDeclareAbstract(ManipTrans, Manip);

public:
  ManipTrans(ManipManager& mgr);

  bool UIEventHandler(const ui::Event& EV) final;

protected:
  virtual void HandleMouseDown(const ork::fvec2& pos);
  virtual void HandleMouseUp(const ork::fvec2& pos);
  virtual void HandleDrag(const ork::fvec2& pos);
};

class ManipSingleTrans : public ManipTrans {
  RttiDeclareAbstract(ManipSingleTrans, ManipTrans);

public:
  ManipSingleTrans(ManipManager& mgr);

  virtual void DrawAxis(Context* pTARG) const;

protected:
  void Draw(Context* pTARG) const final;
  void HandleDrag(const ork::fvec2& pos) final;

  virtual ork::fvec3 GetNormal() const = 0;

  fmtx4 mmRotModel;
};

class ManipDualTrans : public ManipTrans {
  RttiDeclareAbstract(ManipDualTrans, ManipTrans);

public:
  ManipDualTrans(ManipManager& mgr);

  void Draw(Context* pTARG) const final;

protected:
  virtual void GetQuad(float ext, ork::fvec4& v0, ork::fvec4& v1, ork::fvec4& v2, ork::fvec4& v3) const = 0;

  void HandleDrag(const ork::fvec2& pos) final;
};

////////////////////////////////////////////////////////////////////////////////

class ManipTX : public ManipSingleTrans {
  RttiDeclareAbstract(ManipTX, ManipSingleTrans);

public:
  ManipTX(ManipManager& mgr);

protected:
  virtual ork::fvec3 GetNormal() const final { return ork::fvec3::UnitX(); };
};

////////////////////////////////////////////////////////////////////////////////

class ManipTY : public ManipSingleTrans {
  RttiDeclareAbstract(ManipTY, ManipSingleTrans);

public:
  ManipTY(ManipManager& mgr);

protected:
  virtual ork::fvec3 GetNormal() const final { return ork::fvec3::UnitY(); };
};

////////////////////////////////////////////////////////////////////////////////

class ManipTZ : public ManipSingleTrans {
  RttiDeclareAbstract(ManipTZ, ManipSingleTrans);

public:
  ManipTZ(ManipManager& mgr);

protected:
  virtual ork::fvec3 GetNormal() const final { return ork::fvec3::UnitZ(); };
};

class ManipTXY : public ManipDualTrans {
  RttiDeclareAbstract(ManipTXY, ManipDualTrans);

public:
  ManipTXY(ManipManager& mgr);

protected:
  void GetQuad(float ext, ork::fvec4& v0, ork::fvec4& v1, ork::fvec4& v2, ork::fvec4& v3) const final;
};

class ManipTXZ : public ManipDualTrans {
  RttiDeclareAbstract(ManipTXZ, ManipDualTrans);

public:
  ManipTXZ(ManipManager& mgr);

protected:
  void GetQuad(float ext, ork::fvec4& v0, ork::fvec4& v1, ork::fvec4& v2, ork::fvec4& v3) const final;
};

class ManipTYZ : public ManipDualTrans {
  RttiDeclareAbstract(ManipTYZ, ManipDualTrans);

public:
  ManipTYZ(ManipManager& mgr);

protected:
  void GetQuad(float ext, ork::fvec4& v0, ork::fvec4& v1, ork::fvec4& v2, ork::fvec4& v3) const final;
};

////////////////////////////////////////////////////////////////////////////////

class ManipRot : public Manip {
  RttiDeclareAbstract(ManipRot, Manip);

public: //
  ManipRot(ManipManager& mgr, const fvec4& LocRotMat);

  void Draw(Context* pTARG) const final;
  bool UIEventHandler(const ui::Event& EV) final;

  virtual F32 CalcAngle(fvec4& inv_isect, fvec4& inv_lisect) const = 0;

  ////////////////////////////////////////

  fmtx4 mmRotModel;
  const fvec4 mLocalRotationAxis;
};

////////////////////////////////////////////////////////////////////////////////

class ManipRX : public ManipRot {
  RttiDeclareAbstract(ManipRX, ManipRot);

public: //
  ManipRX(ManipManager& mgr);

  F32 CalcAngle(fvec4& inv_isect, fvec4& inv_lisect) const final;
};

////////////////////////////////////////////////////////////////////////////////

class ManipRY : public ManipRot {
  RttiDeclareAbstract(ManipRY, ManipRot);

public: //
  ManipRY(ManipManager& mgr);

  F32 CalcAngle(fvec4& inv_isect, fvec4& inv_lisect) const final;
};

////////////////////////////////////////////////////////////////////////////////

class ManipRZ : public ManipRot {
  RttiDeclareAbstract(ManipRZ, ManipRot);

public: //
  ManipRZ(ManipManager& mgr);

  F32 CalcAngle(fvec4& inv_isect, fvec4& inv_lisect) const final;
};

////////////////////////////////////////////////////////////////////////////////

enum EManipEnable {
  EMANIPMODE_OFF = 0,
  EMANIPMODE_ON,
};

class GfxMaterialManip : public GfxMaterial {
  ManipManager& mManager;

public:
  GfxMaterialManip(Context*, ManipManager& mgr);
  ~GfxMaterialManip() final{};
  void Init(Context* pTarg) final;

  int BeginBlock(Context* pTarg, const RenderContextInstData& MatCtx) final;
  void EndBlock(Context* pTarg) final;
  void Update(void) final {}
  bool BeginPass(Context* pTarg, int iPass = 0) final;
  void EndPass(Context* pTarg) final;
  void UpdateMVPMatrix(Context* pTARG) final;

protected:
  FxShader* hModFX;
  const FxShaderTechnique* hTekStd;
  const FxShaderTechnique* hTekLuc;
  const FxShaderTechnique* hTekPick;

  // const FxShaderTechnique*	hTekCurrent;

  const FxShaderParam* hMVP;
  const FxShaderParam* hTEX;
  const FxShaderParam* hCOLOR;

  fvec4 mColor;

  bool mbNoDepthTest;
};

////////////////////////////////////////////////////////////////////////////////

class ManipManager : public ork::AutoConnector {
  RttiDeclareAbstract(ManipManager, ork::AutoConnector);

  ////////////////////////////////////////////////////////////
  DeclarePublicAutoSlot(ObjectDeSelected);
  DeclarePublicAutoSlot(ObjectSelected);
  DeclarePublicAutoSlot(ObjectDeleted);
  DeclarePublicAutoSlot(ClearSelection);
  ////////////////////////////////////////////////////////////

public:
  ////////////////////////////////////////////////////////////
  void SlotObjectDeSelected(ork::Object* pOBJ);
  void SlotObjectSelected(ork::Object* pOBJ);
  void SlotObjectDeleted(ork::Object* pOBJ);
  void SlotClearSelection();
  ////////////////////////////////////////////////////////////

  friend class ManipRX;
  friend class ManipRY;
  friend class ManipRZ;
  friend class ManipTX;
  friend class ManipTY;
  friend class ManipTZ;
  friend class ManipTXY;
  friend class ManipTXZ;
  friend class ManipTYZ;
  friend class Manip;
  friend class ManipRot;
  friend class ManipTrans;
  friend class ManipSingleTrans;
  friend class ManipDualTrans;

  //////////////////////////////////

  enum EManipMode {
    EMANIPMODE_WORLD_TRANS = 0,
    EMANIPMODE_LOCAL_ROTATE,
  };

  enum EUIMode {
    EUIMODE_STD = 0,
    EUIMODE_PLACE,
    EUIMODE_MANIP_WORLD_TRANSLATE,
    EUIMODE_MANIP_LOCAL_TRANSLATE,
    EUIMODE_MANIP_LOCAL_ROTATE,
  };

  EUIMode meUIMode;

  //////////////////////////////////

  ManipManager();
  static void ClassInit();
  void ManipObjects(void) { mbDoComponents = false; }
  void ManipComponents(void) { mbDoComponents = true; }

  //////////////////////////////////

  void SetManipMode(EManipMode emode) { meManipMode = emode; }
  EManipMode GetManipMode(void) { return meManipMode; }
  bool IsVisible() { return meUIMode != EUIMODE_STD; }
  EUIMode GetUIMode(void) const { return meUIMode; }
  void SetUIMode(EUIMode emode) { meUIMode = emode; }

  void AttachObject(ork::Object* pObject);
  void ReleaseObject(void);
  void DetachObject(void);

  void Setup(IRenderer* prend);
  void Queue(IRenderer* prend);

  void DrawManip(Manip* manip, Context* pTARG);
  void DrawCurrentManipSet(Context* pTARG);

  void SetHover(Manip* manip) { mpHoverManip = manip; }
  Manip* GetHover() { return mpHoverManip; }

  void SetDualAxis(bool dual) { mDualAxis = dual; }
  bool IsDualAxis() { return mDualAxis; }

  void ApplyTransform(const TransformNode& SetMat);
  const TransformNode& GetCurTransform() { return mCurTransform; }

  void EnableManip(Manip* pOBJ);
  void DisableManip(void);

  bool UIEventHandler(const ui::Event& EV);

  UiCamera* getActiveCamera(void) const { return mpActiveCamera; }
  void SetActiveCamera(UiCamera* pCam) { mpActiveCamera = pCam; }

  f32 GetManipScale(void) const { return mfManipScale; }

  GfxMaterial* GetMaterial(void) { return mpManipMaterial; }

  void CalcObjectScale(void);

  void SetWorldTrans(bool bv) { mbWorldTrans = bv; }
  void SetGridSnap(bool bv) { mbGridSnap = bv; }

  void RebaseMatrices(void);

  Grid3d& Grid() { return mGrid; }

  void SetViewScale(float fvs) { mfViewScale = fvs; }
  float CalcViewScale(float fW, float fH, const CameraMatrices* camdat) const;

  void SetDrawMode(int imode) { miDrawMode = imode; }
  int GetDrawMode() const { return miDrawMode; }

private:
  bool mbWorldTrans;
  bool mbGridSnap;

  bool mDualAxis;

  TransformNode mParentTransform;

  GfxMaterialManip* mpManipMaterial;
  Manip* mpTXManip;
  Manip* mpTYManip;
  Manip* mpTZManip;
  Manip* mpTXYManip;
  Manip* mpTXZManip;
  Manip* mpTYZManip;
  Manip* mpRXManip;
  Manip* mpRYManip;
  Manip* mpRZManip;
  Manip* mpCurrentManip;
  Manip* mpHoverManip;
  EManipMode meManipMode;
  EManipEnable meManipEnable;
  float mfViewScale;
  int miDrawMode;

  ManipHandler mManipHandler;
  UiCamera* mpActiveCamera;

  fvec4 mPickCenter;
  fvec4 mPickAccum;

  bool mbDoComponents;

  f32 mfManipScale;

  // float				mfGridSnap;
  float mfBaseManipSize;

  IManipInterface* mpCurrentInterface;
  Object* mpCurrentObject;

  TransformNode mOldTransform;
  TransformNode mCurTransform;

  float mObjScale;
  float mObjInvScale;

  Grid3d mGrid;
};

}} // namespace ork::lev2
