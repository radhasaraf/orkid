///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2020, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/lev2/gfx/proctex/proctex.h>
#include <ork/reflect/properties/DirectTypedMap.h>
#include <ork/reflect/properties/DirectTyped.hpp>

#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/lev2/gfx/material_freestyle.h>
#include <ork/reflect/properties/AccessorObject.h>
#include <ork/reflect/properties/registerX.inl>
#include <ork/reflect/enum_serializer.inl>
#include <ork/file/file.h>
#include <ork/stream/FileInputStream.h>
#include <ork/stream/FileOutputStream.h>
#include <ork/reflect/serialize/JsonSerializer.h>
#include <ork/reflect/serialize/JsonDeserializer.h>
#include <ork/reflect/enum_serializer.inl>
#include <ork/asset/AssetManager.h>

ImplementReflectionX(ork::proctex::ProcTex, "proctex");
ImplementReflectionX(ork::proctex::ImgModule, "proctex::ImgModule");
ImplementReflectionX(ork::proctex::Img32Module, "proctex::Img32Module");
ImplementReflectionX(ork::proctex::Img64Module, "proctex::Img64Module");
ImplementReflectionX(ork::proctex::Module, "proctex::Module");

using namespace ork::lev2;

namespace ork { namespace dataflow {
template <> void outplug<ork::proctex::ImgBase>::describeX(class_t* clazz) {
}
template <> void inplug<ork::proctex::ImgBase>::describeX(class_t* clazz) {
}
template <> int MaxFanout<ork::proctex::ImgBase>() {
  return 0;
}
template <> const ork::proctex::ImgBase& outplug<ork::proctex::ImgBase>::GetInternalData() const {
  OrkAssert(mOutputData != 0);
  return *mOutputData;
}
template <> const ork::proctex::ImgBase& outplug<ork::proctex::ImgBase>::GetValue() const {
  return GetInternalData();
}
}} // namespace ork::dataflow

namespace ork {
file::Path SaveFileRequester(const std::string& title, const std::string& ext);
}

namespace ork::proctex {

BeginEnumRegistration(ProcTexType);
RegisterEnum(ProcTexType, REALTIME);
RegisterEnum(ProcTexType, EXPORT);
EndEnumRegistration();

Img32 ImgModule::gNoCon;
ork::MpMcBoundedQueue<Buffer*> ProcTexContext::gBuf32Q;
ork::MpMcBoundedQueue<Buffer*> ProcTexContext::gBuf64Q;

///////////////////////////////////////////////////////////////////////////////
Buffer::Buffer(ork::lev2::EBufferFormat efmt)
    : mRtGroup(nullptr)
    , miW(256)
    , miH(256) {
  _basename = "ptex::Reg32";
}
void Buffer::SetBufferSize(int w, int h) {
  if (w != miW && h != miH) {

    miW = w;
    miH = h;
    delete mRtGroup;
    mRtGroup = nullptr;
  }
}

///////////////////////////////////////////////////////////////////////////////
void Buffer::PtexBegin(lev2::Context* ptgt, bool push_full_vp, bool clear_all) {
  mTarget  = ptgt;
  auto FBI = mTarget->FBI();
  FBI->SetAutoClear(false);
  auto rtg = GetRtGroup(ptgt);

  FBI->PushRtGroup(rtg);
  ViewportRect vprect_full(0, 0, miW, miH);

  // printf( "  buffer<%p> w<%d> h<%d> rtg<%p> begin\n", this, miW,miH,rtg);

  if (push_full_vp) {
    FBI->pushViewport(vprect_full);
    FBI->pushScissor(vprect_full);
  }
  if (clear_all)
    FBI->Clear(fcolor3::Black(), 1.0f);
}
///////////////////////////////////////////////////////////////////////////////
void Buffer::PtexEnd(bool pop_vp) {
  if (pop_vp) {
    mTarget->FBI()->popViewport();
    mTarget->FBI()->popScissor();
  }

  // printf( "  buffer<%p> end\n", this);

  mTarget->FBI()->PopRtGroup();
  mTarget = nullptr;
}
///////////////////////////////////////////////////////////////////////////////
lev2::RtGroup* Buffer::GetRtGroup(lev2::Context* ptgt) {
  if (mRtGroup == nullptr) {
    mRtGroup             = new RtGroup(ptgt, miW, miH);
    mRtGroup->_autoclear = false;

    auto mrt = mRtGroup->createRenderTarget(lev2::EBufferFormat::RGBA8);

    mrt->_debugName = FormatString("%s<%p>", _basename.c_str(), this);

    mrt->_mipgen = RtBuffer::EMG_AUTOCOMPUTE;

    ptgt->FBI()->PushRtGroup(mRtGroup);
    ptgt->FBI()->PopRtGroup();

    printf(
        "Buffer<%p:%s> RtGroup<%p> texture<%p:%s>\n",
        (void*)this,
        _basename.c_str(),
        (void*)mRtGroup,
        (void*)mrt->texture(),
        mrt->texture()->_debugName.c_str());
  }
  return mRtGroup;
}

///////////////////////////////////////////////////////////////////////////////
lev2::Texture* Buffer::OutputTexture() {
  return (mRtGroup != nullptr) ? mRtGroup->GetMrt(0)->texture() : nullptr;
}

Buffer32::Buffer32()
    : Buffer(lev2::EBufferFormat::RGBA8) {
} // EBufferFormat::RGBA8
Buffer64::Buffer64()
    : Buffer(lev2::EBufferFormat::RGBA16F) {
} // EBufferFormat::RGBA16F

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ork::lev2::Texture* Img32::GetTexture(ProcTex& ptex) const {
  Buffer& buf = GetBuffer(ptex);
  buf.GetRtGroup(ptex.GetTarget());
  return buf.OutputTexture();
}
Buffer& Img32::GetBuffer(ProcTex& ptex) const {
  static Buffer32 gnone;
  return (miBufferIndex >= 0) ? ptex.GetBuffer32(miBufferIndex) : gnone;
}
ork::lev2::Texture* Img64::GetTexture(ProcTex& ptex) const {
  Buffer& buf = GetBuffer(ptex);
  buf.GetRtGroup(ptex.GetTarget());
  return buf.OutputTexture();
}
Buffer& Img64::GetBuffer(ProcTex& ptex) const {
  static Buffer64 gnone;
  return (miBufferIndex >= 0) ? ptex.GetBuffer64(miBufferIndex) : gnone;
}

///////////////////////////////////////////////////////////////////////////////
static lev2::Texture* GetImgModuleIcon(ork::dataflow::dgmodule* pmod) {
  ImgModule* pimgmod = rtti::autocast(pmod);
  auto& buffer       = pimgmod->GetThumbBuffer();
  // printf("GetThumbIcon Buf<%p:%s>\n", &buffer, buffer._basename.c_str());
  return buffer.OutputTexture();
}

void ImgModule::describeX(class_t* clazz) {
  reflect::annotateClassForEditor<ImgModule>("dflowicon", &GetImgModuleIcon);

  auto opm = new ork::reflect::OpMap;

  opm->mLambdaMap["ExportPng"] = [=](Object* pobj) {
    Img32Module* as_module = rtti::autocast(pobj);

    printf("ExportPNG pobj<%p> as_mod<%p>\n", (void*)pobj, (void*)as_module);
    if (as_module) {
      as_module->mExport = true;
    }
  };

  reflect::annotateClassForEditor<Img32Module>("editor.object.ops", opm);
}
void Img32Module::describeX(class_t* clazz) {
  // RegisterObjOutPlug(Img32Module, ImgOut);
  // clazz
  //  ->directProperty("ImgOut", &Img32Module::OutAccessorImgOut) //
  //->annotate<bool>("editor.visible", false);
}
void Img64Module::describeX(class_t* clazz) {
  // RegisterObjOutPlug(Img64Module, ImgOut);
  // clazz
  //  ->directProperty("ImgOut", &Img64Module::OutAccessorImgOut) //
  //->annotate<bool>("editor.visible", false);
}
ImgModule::ImgModule()
    : mExport(false) {
}
Img32Module::Img32Module()
    : ConstructOutTypPlug(ImgOut, dataflow::EPR_UNIFORM, typeid(Img32)) {
  mThumbBuffer._basename = "Thumb32";
}
Img64Module::Img64Module()
    : ConstructOutTypPlug(ImgOut, dataflow::EPR_UNIFORM, typeid(Img64)) {
  mThumbBuffer._basename = "Thumb64";
}
Buffer& ImgModule::GetWriteBuffer(ProcTex& ptex) {
  ImgOutPlug* outplug = 0;
  GetTypedOutput<ImgBase>(0, outplug);
  const ImgBase& base = outplug->GetValue();
  // printf( "MOD<%p> WBI<%d>\n", this, base.miBufferIndex );
  return base.GetBuffer(ptex);
  // return ptex.GetBuffer(outplug->GetValue().miBufferIndex);
}
void ImgModule::Compute(dataflow::workunit* wu) {
  auto ptex                = wu->GetContextData().get<ProcTex*>();
  ProcTexContext* ptex_ctx = ptex->GetPTC();

  auto pTARG = ptex->GetTarget();
  auto fbi   = pTARG->FBI();

  const RenderContextFrameData* RCFD = pTARG->topRenderContextFrameData();
  auto CIMPL                         = RCFD->_cimpl;
  CompositingPassData CPD;
  CIMPL->pushCPD(CPD);

  const_cast<ImgModule*>(this)->compute(*ptex);

  auto& wrbuf   = GetWriteBuffer(*ptex);
  auto ptexture = wrbuf.OutputTexture();
  pTARG->TXI()->generateMipMaps(ptexture);
  ptexture->TexSamplingMode().PresetPointAndClamp();
  pTARG->TXI()->ApplySamplingMode(ptexture);

  if (mExport) {

    if (nullptr == ptexture)
      return;

    auto rtg = wrbuf.GetRtGroup(pTARG);

    auto fname = SaveFileRequester("Export ProcTexImage", "DDS (*.dds)");

    if (fname.length()) {

      // SetRecentSceneFile(FileName.toAscii().data(),SCENEFILE_DIR);
      if (ork::FileEnv::filespec_to_extension(fname.c_str()).length() == 0)
        fname += ".dds";
      auto buf0 = rtg->GetMrt(0);
      fbi->capture(buf0.get(), fname);
    }

    mExport = false;
  }
  CIMPL->popCPD();
}
///////////////////////////////////////////////////////////////////////////////
void ImgModule::UnitTexQuad(
    GfxMaterial* material,       //
    ork::lev2::Context* pTARG) { // fmtx4 mtxortho = pTARG->MTXI()->Ortho( -1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f );
  pTARG->MTXI()->PushPMatrix(fmtx4::Identity());
  pTARG->MTXI()->PushVMatrix(fmtx4::Identity());
  pTARG->MTXI()->PushMMatrix(fmtx4::Identity());
  pTARG->PushModColor(fvec3::White());
  { RenderQuad(material, pTARG, -1.0f, -1.0f, 1.0f, 1.0f); }
  pTARG->PopModColor();
  pTARG->MTXI()->PopPMatrix();
  pTARG->MTXI()->PopVMatrix();
  pTARG->MTXI()->PopMMatrix();
}
void ImgModule::MarkClean() {
  for (int i = 0; i < GetNumOutputs(); i++) {
    GetOutput(i)->SetDirty(false);
  }
}
///////////////////////////////////////////////////////////////////////////////
void RenderQuad(
    GfxMaterial* material, //
    ork::lev2::Context* pTARG,
    float fX1,
    float fY1,
    float fX2,
    float fY2,
    float fu1,
    float fv1,
    float fu2,
    float fv2) {
  U32 uColor = 0xffffffff;

  float maxuv = 1.0f;
  float minuv = 0.0f;

  int ivcount = 6;

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTARG, &GfxEnv::GetSharedDynamicVB(), ivcount);
  float fZ = 0.0f;

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ, fu1, fv1, uColor));
  vw.AddVertex(SVtxV12C4T16(fX2, fY1, fZ, fu2, fv1, uColor));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ, fu2, fv2, uColor));

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ, fu1, fv1, uColor));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ, fu2, fv2, uColor));
  vw.AddVertex(SVtxV12C4T16(fX1, fY2, fZ, fu1, fv2, uColor));

  vw.UnLock(pTARG);

  pTARG->GBI()->DrawPrimitive(material, vw, PrimitiveType::TRIANGLES, ivcount);
}
///////////////////////////////////////////////////////////////////////////////
void RenderQuadEML(
    ork::lev2::Context* pTARG,
    float fX1,
    float fY1,
    float fX2,
    float fY2,
    float fu1,
    float fv1,
    float fu2,
    float fv2) {
  U32 uColor = 0xffffffff;

  float maxuv = 1.0f;
  float minuv = 0.0f;

  int ivcount = 6;

  lev2::VtxWriter<SVtxV12C4T16> vw;
  vw.Lock(pTARG, &GfxEnv::GetSharedDynamicVB(), ivcount);
  float fZ = 0.0f;

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ, fu1, fv1, uColor));
  vw.AddVertex(SVtxV12C4T16(fX2, fY1, fZ, fu2, fv1, uColor));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ, fu2, fv2, uColor));

  vw.AddVertex(SVtxV12C4T16(fX1, fY1, fZ, fu1, fv1, uColor));
  vw.AddVertex(SVtxV12C4T16(fX2, fY2, fZ, fu2, fv2, uColor));
  vw.AddVertex(SVtxV12C4T16(fX1, fY2, fZ, fu1, fv2, uColor));

  vw.UnLock(pTARG);

  pTARG->GBI()->DrawPrimitiveEML(vw, PrimitiveType::TRIANGLES, ivcount);
}

///////////////////////////////////////////////////////////////////////////////
void ImgModule::UpdateThumb(ProcTex& ptex) {
  auto pTARG    = ptex.GetTarget();
  auto fbi      = pTARG->FBI();
  auto dwi      = pTARG->DWI();
  auto& wrbuf   = GetWriteBuffer(ptex);
  auto ptexture = wrbuf.OutputTexture();
  if (nullptr == ptexture) {
    return;
  }

  OrkAssert(_ptex != nullptr);

  const RenderContextFrameData* RCFD = pTARG->topRenderContextFrameData();
  auto CIMPL                         = RCFD->_cimpl;
  CompositingPassData CPD;
  CIMPL->pushCPD(CPD);

  pTARG->debugPushGroup("PtexUpdateThumb");

  auto& thumbmtl = _ptex->_thumbmtl;
  auto thumbrtg  = mThumbBuffer.GetRtGroup(pTARG);

  fbi->PushRtGroup(thumbrtg);
  auto tek    = thumbmtl.technique("ttex");
  auto parmvp = thumbmtl.param("MatMVP");
  auto partex = thumbmtl.param("ColorMap");
  thumbmtl._rasterstate.SetAlphaTest(ork::lev2::EALPHATEST_OFF);
  thumbmtl._rasterstate.SetCullTest(ork::lev2::ECullTest::OFF);
  thumbmtl._rasterstate.SetBlending(ork::lev2::Blending::OFF);
  thumbmtl._rasterstate.SetDepthTest(ork::lev2::EDepthTest::ALWAYS);
  // thumbmtl.SetUser0(fvec4(0.0f, 0.0f, 0.0f, float(wrbuf.miW)));
  thumbmtl.begin(tek, *RCFD);
  thumbmtl.bindParamCTex(partex, ptexture);
  thumbmtl.bindParamMatrix(parmvp, fmtx4::Identity());
  ////////////////////////////////////////////////////////////////
  // float ftexw = ptexture ? ptexture->_width : 1.0f;
  // pTARG->PushModColor(ork::fvec4(ftexw, ftexw, ftexw, ftexw));
  ////////////////////////////////////////////////////////////////
  {
    fvec4 xywh(-1, -1, 2, 2);
    fvec4 uv(0, 0, 1, 1);
    dwi->quad2DEML(xywh, uv, uv);
  }
  thumbmtl.end(*RCFD);
  MarkClean();
  fbi->PopRtGroup();
  fbi->rtGroupMipGen(thumbrtg);
  pTARG->debugPopGroup();
  CIMPL->popCPD();
}
///////////////////////////////////////////////////////////////////////////////
void ProcTex::describeX(class_t* clazz) { // ork::reflect::RegisterProperty( "Global", & ProcTex::GlobalAccessor );
  // ork::reflect::annotatePropertyForEditor< ProcTex >("Global", "editor.visible", "false" );
  // ork::reflect::annotatePropertyForEditor< ProcTex >("Modules", "editor.factorylistbase",
  // "proctex::Module" );
}

///////////////////////////////////////////////////////////////////////////////
ProcTex::ProcTex()
    : mpctx(0)
    , mbTexQuality(false)
    , mpResTex(0) {
  Global::GetClassStatic();
  RotSolid::GetClassStatic();
  Colorize::GetClassStatic();
  ImgOp2::GetClassStatic();
  ImgOp3::GetClassStatic();
  Transform::GetClassStatic();
  Octaves::GetClassStatic();
  Texture::GetClassStatic();
}
///////////////////////////////////////////////////////////////////////////////
bool ProcTex::CanConnect(const ork::dataflow::inplugbase* pin, const ork::dataflow::outplugbase* pout) const // virtual
{
  bool brval = false;
  brval |= (&pin->GetDataTypeId() == &typeid(ImgBase)) && (&pout->GetDataTypeId() == &typeid(Img64));
  brval |= (&pin->GetDataTypeId() == &typeid(ImgBase)) && (&pout->GetDataTypeId() == &typeid(Img32));
  brval |= (&pin->GetDataTypeId() == &typeid(float)) && (&pout->GetDataTypeId() == &typeid(float));
  return brval;
}
///////////////////////////////////////////////////////////////////////////////
// compute result to mBuffer
///////////////////////////////////////////////////////////////////////////////
void ProcTex::compute(ProcTexContext& ptctx) {
  if (false == IsComplete())
    return;
  mpctx = &ptctx;
  // printf( "ProcTex<%p>::compute ProcTexContext<%p>\n", this, mpctx );
  //////////////////////////////////
  // build the execution graph
  //////////////////////////////////
  Clear();
  RefreshTopology(ptctx.mdflowctx);

  //////////////////////////////////

  mpResTex = nullptr;

  auto pTARG = GetTarget();

  if (_dogpuinit) {
    _thumbmtl.gpuInit(pTARG, "orkshader://proctex");
    _dogpuinit = false;
  }
  pTARG->debugPushGroup(FormatString("ptx::compute"));

  //////////////////////////////////
  // execute df graph
  //////////////////////////////////

#if 1
  ImgModule* res_module                              = nullptr;
  const orklut<int, dataflow::dgmodule*>& TopoSorted = LockTopoSortedChildrenForRead(1);
  {
    for (orklut<int, dataflow::dgmodule*>::const_iterator it = TopoSorted.begin(); it != TopoSorted.end(); it++) {
      dataflow::dgmodule* dgmod = it->second;
      Group* pgroup             = rtti::autocast(dgmod);
      ImgModule* img_module     = ork::rtti::autocast(dgmod);
      ///////////////////////////////////
      ImgModule* img_module_updthumb = nullptr;
      ///////////////////////////////////
      if (img_module) {
        img_module->_ptex   = this;
        ImgOutPlug* outplug = 0;
        img_module->GetTypedOutput<ImgBase>(0, outplug);
        const ImgBase& base = outplug->GetValue();

        if (outplug->GetRegister()) {
          int ireg = outplug->GetRegister()->mIndex;
          // OrkAssert( ireg>=0 && ireg<ibufmax );
          outplug->GetValue().miBufferIndex = ireg;
          // printf( "pmod<%p> reg<%d>\n", img_module, ireg );
          auto tex = base.GetTexture(*this);
          if (tex != nullptr) {
            mpResTex            = tex;
            res_module          = img_module;
            img_module_updthumb = img_module;
          }
        } else {
          outplug->GetValue().miBufferIndex = -2;
        }

        /*if( mpProbeImage == pmodule )
        {
            outplug->GetValue().miBufferIndex = ibufmax-1;
        }*/
      }
      ///////////////////////////////////
      if (pgroup) {
      } else {
        bool bmoddirty = true; // pmod->IsDirty();
        if (bmoddirty) {
          dataflow::cluster mycluster;
          dataflow::workunit mywunit(dgmod, &mycluster, 0);
          mywunit.SetContextData(this);
          dgmod->Compute(&mywunit);
        }
      }
      ///////////////////////////////////
      if (img_module_updthumb) {
        img_module_updthumb->UpdateThumb(*this);
      }
    }
  }
  UnLockTopoSortedChildren();
#endif

  //////////////////////////////////

  if (ptctx.mWriteFrames && res_module) {
    auto& wrbuf   = res_module->GetWriteBuffer(*this);
    auto ptexture = wrbuf.OutputTexture();

    if (nullptr == ptexture)
      return;

    auto targ = ptctx.mTarget;
    auto fbi  = targ->FBI();

    auto rtg = wrbuf.GetRtGroup(targ);

    if (ptctx.mWritePath.length()) {
      file::DecomposedPath dpath;
      ptctx.mWritePath.decompose(dpath);
      fxstring<64> fidstr;
      fidstr.format("_%04d", ptctx.mWriteFrameIndex);
      dpath.mFile += fidstr.c_str();
      ork::file::Path indexed_path;
      indexed_path.compose(dpath);
      printf("indexed_path<%s>\n", indexed_path.c_str());
      auto buf0 = rtg->GetMrt(0);
      fbi->capture(buf0.get(), indexed_path);
    }
  }
  pTARG->debugPopGroup();

  //////////////////////////////////
  // mpctx = 0;
}
///////////////////////////////////////////////////////////////////////////////
void Module::describeX(class_t* clazz) {
}
Module::Module() {
}
///////////////////////////////////////////////////////////////////////////////
ork::lev2::Texture* ProcTex::ResultTexture() {
  return mpResTex;
}
Buffer& ProcTexContext::GetBuffer32(int edest) {
  if (edest < 0)
    return mTrashBuffer;
  OrkAssert(edest < k32buffers);
  return *mBuffer32[edest];
}
Buffer& ProcTexContext::GetBuffer64(int edest) {
  if (edest < 0)
    return mTrashBuffer;
  OrkAssert(edest < k64buffers);
  return *mBuffer64[edest];
}
ProcTexContext::ProcTexContext()
    : mdflowctx()
    , mFloatRegs("ptex_float", 4)
    , mImage32Regs("ptex_img32", k32buffers)
    , mImage64Regs("ptex_img64", k64buffers)
    , mWritePath("ptex_out.png") {

  mdflowctx.SetRegisters<float>(&mFloatRegs);
  mdflowctx.SetRegisters<Img32>(&mImage32Regs);
  mdflowctx.SetRegisters<Img64>(&mImage64Regs);

  for (int i = 0; i < k64buffers; i++)
    mBuffer64[i] = AllocBuffer64();

  for (int i = 0; i < k32buffers; i++)
    mBuffer32[i] = AllocBuffer64();
}
ProcTexContext::~ProcTexContext() {
  for (int i = 0; i < k64buffers; i++)
    ReturnBuffer(mBuffer64[i]);

  for (int i = 0; i < k32buffers; i++)
    ReturnBuffer(mBuffer32[i]);
}
void ProcTexContext::SetBufferDim(int idim) {
  mBufferDim = idim;
  for (int i = 0; i < k64buffers; i++)
    mBuffer64[i]->SetBufferSize(idim, idim);

  for (int i = 0; i < k32buffers; i++)
    mBuffer32[i]->SetBufferSize(idim, idim);
}
///////////////////////////////////////////////////////////////////////////////
ProcTex* ProcTex::Load(const ork::file::Path& pth) {
  /*ProcTex* rval        = 0;
  ork::file::Path path = pth;
  path.SetExtension("ptx");
  lev2::GfxEnv::GetRef().GetGlobalLock().Lock();
  stream::FileInputStream istream(path.c_str());
  reflect::serdes::JsonDeserializer iser(istream);
  rtti::ICastable* pcastable = 0;
  bool bOK                   = iser.deserializeObject(pcastable);
  if (bOK) {
    ork::asset::AssetManager<ork::lev2::TextureAsset>::AutoLoad();
    rval = rtti::safe_downcast<ProcTex*>(pcastable);
  }
  lev2::GfxEnv::GetRef().GetGlobalLock().UnLock();
  return rval;*/
  return nullptr;
}

Buffer* ProcTexContext::AllocBuffer32() {
  Buffer* rval = nullptr;
  if (false == gBuf32Q.try_pop(rval)) {
    rval = new Buffer32;
  }
  return rval;
}
Buffer* ProcTexContext::AllocBuffer64() {
  Buffer* rval = nullptr;
  if (false == gBuf64Q.try_pop(rval)) {
    rval = new Buffer64;
  }
  return rval;
}
void ProcTexContext::ReturnBuffer(Buffer* pbuf) {
  assert(pbuf != nullptr);
  if (pbuf->IsBuf32())
    gBuf32Q.push(pbuf);
  else
    gBuf64Q.push(pbuf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AA16Render::AA16Render(ProcTex& ptx, Buffer& bo)
    : mPTX(ptx)
    , bufout(bo)
    , downsamplemat(lev2::contextForCurrentThread(), "orkshader://proctex", "downsample16") {
  downsamplemat.SetColorMode(lev2::GfxMaterial3DSolid::EMODE_USER);
  downsamplemat._rasterstate.SetAlphaTest(ork::lev2::EALPHATEST_OFF);
  downsamplemat._rasterstate.SetCullTest(ork::lev2::ECullTest::OFF);
  downsamplemat._rasterstate.SetBlending(ork::lev2::Blending::ADDITIVE);
  downsamplemat._rasterstate.SetDepthTest(ork::lev2::EDepthTest::ALWAYS);
  downsamplemat.SetUser0(fvec4(0.0f, 0.0f, 0.0f, float(bo.miW)));
}

///////////////////////////////////////////////////////////////////////////////

struct quad {
  float fx0;
  float fy0;
  float fx1;
  float fy1;
};

void AA16Render::RenderAA() {
  auto target = mPTX.GetTarget();
  auto fbi    = target->FBI();
  auto mtxi   = target->MTXI();
  auto txi    = target->TXI();

  fmtx4 mtxortho;

  float boxx = mOrthoBoxXYWH.x;
  float boxy = mOrthoBoxXYWH.y;
  float boxw = mOrthoBoxXYWH.z;
  float boxh = mOrthoBoxXYWH.w;

  float xa = boxx + (boxw * 0.0f);
  float xb = boxx + (boxw * 0.25f);
  float xc = boxx + (boxw * 0.5f);
  float xd = boxx + (boxw * 0.75f);
  float xe = boxx + (boxw * 1.0f);

  float ya = boxy + (boxh * 0.0f);
  float yb = boxy + (boxh * 0.25f);
  float yc = boxy + (boxh * 0.5f);
  float yd = boxy + (boxh * 0.75f);
  float ye = boxy + (boxh * 1.0f);

  quad quads[16] = {
      {xa, ya, xb, yb},
      {xb, ya, xc, yb},
      {xc, ya, xd, yb},
      {xd, ya, xe, yb},
      {xa, yb, xb, yc},
      {xb, yb, xc, yc},
      {xc, yb, xd, yc},
      {xd, yb, xe, yc},
      {xa, yc, xb, yd},
      {xb, yc, xc, yd},
      {xc, yc, xd, yd},
      {xd, yc, xe, yd},
      {xa, yd, xb, ye},
      {xb, yd, xc, ye},
      {xc, yd, xd, ye},
      {xd, yd, xe, ye},
  };

  float ua = 0.0f;
  float ub = 0.25f;
  float uc = 0.5f;
  float ud = 0.75f;
  float ue = 1.0f;

  float va = 0.0f;
  float vb = 0.25f;
  float vc = 0.5f;
  float vd = 0.75f;
  float ve = 1.0f;

  quad quadsUV[16] = {
      {ua, va, ub, vb},
      {ub, va, uc, vb},
      {uc, va, ud, vb},
      {ud, va, ue, vb},
      {ua, vb, ub, vc},
      {ub, vb, uc, vc},
      {uc, vb, ud, vc},
      {ud, vb, ue, vc},
      {ua, vc, ub, vd},
      {ub, vc, uc, vd},
      {uc, vc, ud, vd},
      {ud, vc, ue, vd},
      {ua, vd, ub, ve},
      {ub, vd, uc, ve},
      {uc, vd, ud, ve},
      {ud, vd, ue, ve},
  };

  auto temp_buffer = bufout.IsBuf32() ? ProcTexContext::AllocBuffer32() : ProcTexContext::AllocBuffer64();

  for (int i = 0; i < 16; i++) {
    const quad& q  = quads[i];
    const quad& uq = quadsUV[i];
    float left     = q.fx0;
    float right    = q.fx1;
    float top      = q.fy0;
    float bottom   = q.fy1;

    //////////////////////////////////////////////////////
    // Render subsection to BufTA
    //////////////////////////////////////////////////////

    {
      temp_buffer->PtexBegin(target, true, false);
      fmtx4 mtxortho = mtxi->Ortho(left, right, top, bottom, 0.0f, 1.0f);
      mtxi->PushMMatrix(fmtx4::Identity());
      mtxi->PushVMatrix(fmtx4::Identity());
      mtxi->PushPMatrix(mtxortho);
      DoRender(left, right, top, bottom, *temp_buffer);
      mtxi->PopPMatrix();
      mtxi->PopVMatrix();
      mtxi->PopMMatrix();
      temp_buffer->PtexEnd(true);
    }

    //////////////////////////////////////////////////////
    // Resolve to output buffer
    //////////////////////////////////////////////////////

    bufout.PtexBegin(target, true, (i == 0));
    {
      float l = boxx;
      float r = boxx + boxw;
      float t = boxy;
      float b = boxy + boxh;

      auto tex = temp_buffer->OutputTexture();
      downsamplemat.SetTexture(tex);
      tex->TexSamplingMode().PresetPointAndClamp();
      txi->ApplySamplingMode(tex);

      fmtx4 mtxortho = mtxi->Ortho(l, r, t, b, 0.0f, 1.0f);
      mtxi->PushMMatrix(fmtx4::Identity());
      mtxi->PushVMatrix(fmtx4::Identity());
      mtxi->PushPMatrix(mtxortho);
      RenderQuad(&downsamplemat, target, q.fx0, q.fy1, q.fx1, q.fy0, 0.0f, 0.0f, 1.0f, 1.0f);
      mtxi->PopPMatrix();
      mtxi->PopVMatrix();
      mtxi->PopMMatrix();
    }
    bufout.PtexEnd(true);

    //////////////////////////////////////////////////////
  }
  ProcTexContext::ReturnBuffer(temp_buffer);
  fbi->SetAutoClear(true);
}

///////////////////////////////////////////////////////////////////////////////

void AA16Render::RenderNoAA() {
  auto target = mPTX.GetTarget();
  auto mtxi   = target->MTXI();
  auto fbi    = target->FBI();

  float x = mOrthoBoxXYWH.x;
  float y = mOrthoBoxXYWH.y;
  float w = mOrthoBoxXYWH.z;
  float h = mOrthoBoxXYWH.w;
  float l = x;
  float r = x + w;
  float t = y;
  float b = y + h;

  bufout.PtexBegin(target, true, true);
  {
    fmtx4 mtxortho = mtxi->Ortho(l, r, t, b, 0.0f, 1.0f);
    mtxi->PushMMatrix(fmtx4::Identity());
    mtxi->PushVMatrix(fmtx4::Identity());
    mtxi->PushPMatrix(mtxortho);
    DoRender(l, r, t, b, bufout);
    mtxi->PopPMatrix();
    mtxi->PopVMatrix();
    mtxi->PopMMatrix();
  }
  bufout.PtexEnd(true);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::proctex

ImplementTemplateReflectionX(ork::dataflow::outplug<ork::proctex::ImgBase>, "proctex::OutImgPlug");
ImplementTemplateReflectionX(ork::dataflow::inplug<ork::proctex::ImgBase>, "proctex::InImgPlug");
