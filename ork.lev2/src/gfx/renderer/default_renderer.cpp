////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/renderer/renderable.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/lighting/gfx_lighting.h>
#include <ork/pch.h>

#include <ork/kernel/Array.hpp>
#include <ork/kernel/timer.h>

namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

DefaultRenderer::DefaultRenderer(Context* ptarg)
    : IRenderer(ptarg) {
}

///////////////////////////////////////////////////////////////////////////////

void DefaultRenderer::RenderModelGroup(const modelgroup_t& mdlgroup) const {
  for (auto r : mdlgroup)
    RenderModel(*r);
}

///////////////////////////////////////////////////////////////////////////////

void DefaultRenderer::RenderModel(const ModelRenderable& mdl_renderable, RenderGroupState rgs) const {
  auto context = GetTarget();
  auto minst   = mdl_renderable.GetModelInst();
  auto model   = minst->xgmModel();
  context->debugPushGroup(FormatString("DefaultRenderer::RenderModel model<%p> minst<%p>", model, minst.get()));
  /////////////////////////////////////////////////////////////
  float fscale        = mdl_renderable.GetScale();
  const fvec3& offset = mdl_renderable.GetOffset();
  const fvec3& rotate = mdl_renderable.GetRotate();
  fmtx4 smat, tmat, rmat;
  smat.SetScale(fscale);
  tmat.SetTranslation(offset);
  rmat.SetRotateY(rotate.GetY() + rotate.GetZ());
  fmtx4 wmat = mdl_renderable.GetMatrix();
  /////////////////////////////////////////////////////////////
  // compute world matrix
  /////////////////////////////////////////////////////////////
  fmtx4 nmat = tmat * rmat * smat * wmat;
  if (minst->IsBlenderZup()) { // zup to yup conversion matrix
    fmtx4 rmatx, rmaty;
    rmatx.RotateX(3.14159f * -0.5f);
    rmaty.RotateX(3.14159f);
    nmat = (rmatx * rmaty) * nmat;
  }
  /////////////////////////////////////////////////////////////
  RenderContextInstData RCID;
  RenderContextInstModelData RCID_MD;
  RCID.SetMaterialInst(&minst->RefMaterialInst());
  RCID_MD.mMesh    = mdl_renderable.mesh();
  RCID_MD.mSubMesh = mdl_renderable.subMesh();
  RCID_MD._cluster = mdl_renderable.GetCluster();
  RCID.SetMaterialIndex(0);
  RCID.SetRenderer(this);
  RCID._dagrenderable = &mdl_renderable;
  // context->debugMarker(FormatString("toolrenderer::RenderModel isskinned<%d> owner_as_ent<%p>", int(model->isSkinned()),
  // as_ent));
  ///////////////////////////////////////
  // printf( "Renderer::RenderModel() rable<%p>\n", & ModelRen );
  bool model_is_skinned = model->isSkinned();
  RCID._isSkinned       = model_is_skinned;
  RCID_MD.SetSkinned(model_is_skinned);
  RCID_MD.SetModelInst(minst);
  auto ObjColor = fvec4::White();
  if (model_is_skinned) {
    model->RenderSkinned(minst.get(), ObjColor, nmat, GetTarget(), RCID, RCID_MD);
  } else {
    model->RenderRigid(ObjColor, nmat, GetTarget(), RCID, RCID_MD);
  }
  context->debugPopGroup();
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
