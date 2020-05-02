////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2020, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#pragma once

#include "gfxenv.h"

///////////////////////////////////////////////////////////////////////////////

namespace ork { namespace lev2 {

class ContextDummy;

///////////////////////////////////////////////////////////////////////////////

struct DummyDrawingInterface : public DrawingInterface {
  DummyDrawingInterface(ContextDummy& ctx);
};

class DummyFxInterface : public FxInterface {
public:
  void _doBeginFrame() final {
  }

  int BeginBlock(const FxShaderTechnique* tek, const RenderContextInstData& data) final {
    return 0;
  }
  bool BindPass(int ipass) final {
    return false;
  }
  void EndPass() final {
  }
  void EndBlock() final {
  }
  void CommitParams(void) final {
  }

  const FxShaderTechnique* technique(FxShader* hfx, const std::string& name) final {
    return nullptr;
  }
  const FxShaderParam* parameter(FxShader* hfx, const std::string& name) final {
    return nullptr;
  }
  const FxShaderParamBlock* parameterBlock(FxShader* hfx, const std::string& name) final {
    return nullptr;
  }
#if defined(ENABLE_SHADER_STORAGE)
  const FxShaderStorageBlock* storageBlock(FxShader* hfx, const std::string& name) final {
    return nullptr;
  }
#endif
#if defined(ENABLE_COMPUTE_SHADERS)
  const FxComputeShader* computeShader(FxShader* hfx, const std::string& name) final {
    return nullptr;
  }
#endif

  void BindParamBool(FxShader* hfx, const FxShaderParam* hpar, const bool bval) final {
  }
  void BindParamInt(FxShader* hfx, const FxShaderParam* hpar, const int ival) final {
  }
  void BindParamVect2(FxShader* hfx, const FxShaderParam* hpar, const fvec2& Vec) final {
  }
  void BindParamVect3(FxShader* hfx, const FxShaderParam* hpar, const fvec3& Vec) final {
  }
  void BindParamVect4(FxShader* hfx, const FxShaderParam* hpar, const fvec4& Vec) final {
  }
  void BindParamVect4Array(FxShader* hfx, const FxShaderParam* hpar, const fvec4* Vec, const int icount) final {
  }
  void BindParamFloatArray(FxShader* hfx, const FxShaderParam* hpar, const float* pfA, const int icnt) final {
  }
  void BindParamFloat(FxShader* hfx, const FxShaderParam* hpar, float fA) final {
  }
  void BindParamMatrix(FxShader* hfx, const FxShaderParam* hpar, const fmtx4& Mat) final {
  }
  void BindParamMatrix(FxShader* hfx, const FxShaderParam* hpar, const fmtx3& Mat) final {
  }
  void BindParamMatrixArray(FxShader* hfx, const FxShaderParam* hpar, const fmtx4* MatArray, int iCount) final {
  }
  void BindParamU32(FxShader* hfx, const FxShaderParam* hpar, U32 uval) final {
  }
  void BindParamCTex(FxShader* hfx, const FxShaderParam* hpar, const Texture* pTex) final {
  }

  bool LoadFxShader(const AssetPath& pth, FxShader* ptex) final;

  DummyFxInterface() {
  }
};

///////////////////////////////////////////////////////////////////////////////

struct DuRasterStateInterface : public RasterStateInterface {
  DuRasterStateInterface(Context& target);
  void BindRasterState(const SRasterState& rState, bool bForce = false) override {
  }
  void SetZWriteMask(bool bv) override {
  }
  void SetRGBAWriteMask(bool rgb, bool a) override {
  }
  void SetBlending(EBlending eVal) override {
  }
  void SetDepthTest(EDepthTest eVal) override {
  }
  void SetCullTest(ECullTest eVal) override {
  }
  void setScissorTest(EScissorTest eVal) override {
  }

public:
};

///////////////////////////////////////////////////////////////////////////////
#if defined(ENABLE_COMPUTE_SHADERS)
struct DuComputeInterface : public ComputeInterface {};
#endif
///////////////////////////////////////////////////////////////////////////////

class DuMatrixStackInterface : public MatrixStackInterface {
  virtual fmtx4 Ortho(float left, float right, float top, float bottom, float fnear, float ffar);

public:
  DuMatrixStackInterface(Context& target)
      : MatrixStackInterface(target) {
  }
};

///////////////////////////////////////////////////////////////////////////////

class DuGeometryBufferInterface final : public GeometryBufferInterface {

  ///////////////////////////////////////////////////////////////////////
  // VtxBuf Interface

  void* LockVB(VertexBufferBase& VBuf, int ivbase, int icount) override;
  void UnLockVB(VertexBufferBase& VBuf) override;

  const void* LockVB(const VertexBufferBase& VBuf, int ivbase = 0, int icount = 0) override;
  void UnLockVB(const VertexBufferBase& VBuf) override;

  void ReleaseVB(VertexBufferBase& VBuf) override;

  //

  void* LockIB(IndexBufferBase& VBuf, int ivbase, int icount) override;
  void UnLockIB(IndexBufferBase& VBuf) override;

  const void* LockIB(const IndexBufferBase& VBuf, int ibase = 0, int icount = 0) override;
  void UnLockIB(const IndexBufferBase& VBuf) override;

  void ReleaseIB(IndexBufferBase& VBuf) override;

  //

  void DrawPrimitiveEML(
      const VertexBufferBase& VBuf, //
      EPrimitiveType eType,
      int ivbase,
      int ivcount) override;

  void DrawIndexedPrimitiveEML(
      const VertexBufferBase& VBuf,
      const IndexBufferBase& IdxBuf,
      EPrimitiveType eType,
      int ivbase,
      int ivcount) override;

  void DrawInstancedIndexedPrimitiveEML(
      const VertexBufferBase& VBuf,
      const IndexBufferBase& IdxBuf,
      EPrimitiveType eType,
      size_t instance_count) override;

  //////////////////////////////////////////////

public:
  DuGeometryBufferInterface(ContextDummy& ctx);
  ContextDummy& _ducontext;
};

///////////////////////////////////////////////////////////////////////////////

class DuFrameBufferInterface : public FrameBufferInterface {
public:
  DuFrameBufferInterface(Context& target);
  ~DuFrameBufferInterface();

  virtual void SetRtGroup(RtGroup* Base) {
  }

  ///////////////////////////////////////////////////////

  virtual void _setViewport(int iX, int iY, int iW, int iH) {
  }
  virtual void _setScissor(int iX, int iY, int iW, int iH) {
  }
  virtual void Clear(const fcolor4& rCol, float fdepth) {
  }
  virtual void clearDepth(float fdepth) {
  }

  virtual void GetPixel(const fvec4& rAt, PixelFetchContext& ctx) {
  }

  //////////////////////////////////////////////

  virtual void _doBeginFrame(void) {
  }
  virtual void _doEndFrame(void) {
  }

protected:
};

///////////////////////////////////////////////////////////////////////////////

class DuTextureInterface : public TextureInterface {
public:
  void TexManInit(void) final {
  }

  bool DestroyTexture(Texture* ptex) final {
    return false;
  }
  bool LoadTexture(const AssetPath& fname, Texture* ptex) final;
  bool LoadTexture(Texture* ptex, datablockptr_t inpdata) final {
    return false;
  }
  void SaveTexture(const ork::AssetPath& fname, Texture* ptex) final {
  }
  void generateMipMaps(Texture* ptex) final {
  }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class ContextDummy final : public Context {
  RttiDeclareConcrete(ContextDummy, Context);

  friend class GfxEnv;

  ///////////////////////////////////////////////////////////////////////

public:
  ContextDummy();
  ~ContextDummy();

  ///////////////////////////////////////////////////////////////////////
  // VtxBuf Interface

  bool SetDisplayMode(DisplayMode* mode) final;

  //////////////////////////////////////////////
  // FX Interface

  FxInterface* FXI() final {
    return &mFxI;
  }
  RasterStateInterface* RSI() final {
    return &mRsI;
  }
  MatrixStackInterface* MTXI() final {
    return &mMtxI;
  }
  GeometryBufferInterface* GBI() final {
    return &mGbI;
  }
  TextureInterface* TXI() final {
    return &mTxI;
  }
  FrameBufferInterface* FBI() final {
    return &mFbI;
  }
  DrawingInterface* DWI() final {
    return &mDWI;
  }

#if defined(ENABLE_COMPUTE_SHADERS)
  ComputeInterface* CI() final {
    return &mCI;
  }
#endif

  //////////////////////////////////////////////

private:
  //////////////////////////////////////////////
  // GfxHWContext Concrete Interface
  //////////////////////////////////////////////

  void _doBeginFrame(void) final {
  }
  void _doEndFrame(void) final {
  }
  void initializeWindowContext(Window* pWin, CTXBASE* pctxbase) final; // make a window
  void initializeOffscreenContext(OffscreenBuffer* pBuf) final;        // make a pbuffer
  void initializeLoaderContext() final;
  void _doResizeMainSurface(int iW, int iH) final;

  ///////////////////////////////////////////////////////////////////////

private:
  DummyFxInterface mFxI;
  DuMatrixStackInterface mMtxI;
  DuRasterStateInterface mRsI;
  DuGeometryBufferInterface mGbI;
  DuTextureInterface mTxI;
  DuFrameBufferInterface mFbI;
  DummyDrawingInterface mDWI;

#if defined(ENABLE_COMPUTE_SHADERS)
  DuComputeInterface mCI;
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2
