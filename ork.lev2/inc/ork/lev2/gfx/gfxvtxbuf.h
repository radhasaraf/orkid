////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#pragma once

#include <ork/lev2/lev2_types.h>
#include <ork/lev2/gfx/gfxenv_enum.h>
#include <ork/math/cvector2.h>
#include <ork/math/cvector3.h>
#include <ork/math/cvector4.h>
#include <ork/util/endian.h>

namespace ork { namespace lev2 {

class GeometryBufferInterface;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class IndexBufferBase {
protected:
  int miNumIndices;
  mutable void* mhIndexBuf;
  void* mpIndices;
  bool mbLocked;

  void Release(void);

public:
  int GetNumIndices() const {
    return miNumIndices;
  }
  void SetNumIndices(int inum) {
    miNumIndices = inum;
  }

  IndexBufferBase();
  virtual ~IndexBufferBase();
  void* GetHandle(void) const {
    return (mhIndexBuf);
  }
  void SetHandle(void* ph) const {
    mhIndexBuf = ph;
  }

  virtual int GetIndexSize() const = 0;
  virtual bool IsStatic() const    = 0;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T> class IdxBuffer : public IndexBufferBase {
public:
  IdxBuffer()
      : IndexBufferBase() {
  }
  ~IdxBuffer();
  virtual int GetIndexSize() const {
    return sizeof(T);
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T> class StaticIndexBuffer : public IdxBuffer<T> {
public:
  StaticIndexBuffer(int inumidx = 0, const T* src = 0);
  ~StaticIndexBuffer();
  /*virtual*/ bool IsStatic() const {
    return true;
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T> class DynamicIndexBuffer : public IdxBuffer<T> {
public:
  DynamicIndexBuffer(int inumidx = 0);
  ~DynamicIndexBuffer();
  /*virtual*/ bool IsStatic() const {
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VertexBufferBase {
public:
  VertexBufferBase(int iMax, int iFlush, int iSize, PrimitiveType eType, EVtxStreamFormat eFmt);
  virtual ~VertexBufferBase();

  ///////////////////////////////////////////////////////////////

  virtual void EndianSwap() = 0;

  int GetMax(void) const {
    return int(miMaxVerts);
  }
  int GetNumVertices(void) const {
    return int(miNumVerts);
  }
  int GetVtxSize(void) const {
    return int(miVtxSize);
  }
  void SetHandle(void* hVB) {
    _IMPL = hVB;
  }
  void* GetHandle(void) const {
    return _IMPL;
  }

  void Reset(void) {
    miNumVerts = 0;
  }
  void SetNumVertices(int inum) {
    miNumVerts = inum;
  }

  EVtxStreamFormat GetStreamFormat(void) const {
    return EVtxStreamFormat(meStreamFormat);
  }
  PrimitiveType GetPrimType(void) const {
    return PrimitiveType(mePrimType);
  }

  bool IsLocked(void) const {
    return mbLocked;
  }
  void Lock() const {
    miLockWriteIndex = 0;
    SetLock(true);
  }
  void Unlock() const {
    // miLockWriteIndex=0;
    SetLock(false);
  }
  void SetRingLock(bool v) {
    mbRingLock = v;
  }
  bool GetRingLock() const {
    return mbRingLock;
  }

  int _ring_lock_index  = 0;

  ///////////////////////////////////////////////////////////////

  static vtxbufferbase_ptr_t CreateVertexBuffer(EVtxStreamFormat eformat, int inumverts, bool bstatic);

  virtual bool IsStatic() const = 0;

protected:
  void* _vertices = nullptr;
  void* _IMPL     = nullptr;

  int miNumVerts;
  int miMaxVerts;
  int miVtxSize;
  mutable int miLockWriteIndex;
  int miFlushSize;
  PrimitiveType mePrimType;
  EVtxStreamFormat meStreamFormat;
  mutable bool mbLocked;
  bool mbInited;
  bool mbRingLock;

private:
  void SetLock(bool bLock) const {
    mbLocked = bLock;
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T> class CVtxBuffer : public VertexBufferBase {
public:
  typedef T vertex_t;

  CVtxBuffer(int iMax, int iFlush, PrimitiveType eType)
      : VertexBufferBase(iMax, iFlush, sizeof(T), eType, T::meFormat) {
  }

  virtual void EndianSwap() {
    T* tbase = (T*) _vertices;

    for (int i = 0; i < miNumVerts; i++) {
      tbase[i].EndianSwap();
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

enum EVtxWriteUnlockFlags {
  EULFLG_NONE        = 0,
  EULFLG_ASSIGNVBLEN = 1,
  EULFLG_END,
};

struct VtxWriterBase {
  int miWriteBase;
  int miWriteCounter;
  int miWriteMax;
  char* mpBase;
  VertexBufferBase* mpVB;

  VtxWriterBase()
      : miWriteBase(0)
      , miWriteCounter(0)
      , miWriteMax(0)
      , mpBase(0)
      , mpVB(0) {
  }

  void Lock(Context* pT, VertexBufferBase* mpVB, int icount = 0);
  void UnLock(Context* pT, u32 UnLockFlags = EULFLG_NONE);
  void Lock(GeometryBufferInterface* GBI, VertexBufferBase* mpVB, int icount = 0);
  void UnLock(GeometryBufferInterface* GBI, u32 UnLockFlags = EULFLG_NONE);
};

template <typename T> struct VtxWriter : public VtxWriterBase {
  inline T& RefAndIncrement() {
    OrkAssert(mpBase != 0);
    OrkAssert(miWriteCounter < miWriteMax);
    T* pVtx = reinterpret_cast<T*>(mpBase);
    return pVtx[miWriteCounter++];
  }
  inline int AddVertex(T const& rT) {
    OrkAssert(mpBase != 0);
    OrkAssert(miWriteCounter < miWriteMax);
    T* pVtx    = reinterpret_cast<T*>(mpBase);
    int rval   = miWriteCounter++;
    pVtx[rval] = rT;
    return rval;
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T> class StaticVertexBuffer : public CVtxBuffer<T> {
  /*virtual*/ bool IsStatic() const {
    return true;
  }

public:
  StaticVertexBuffer(int iMax, int iFlush, PrimitiveType eType)
      : CVtxBuffer<T>(iMax, iFlush, eType) {
    // printf("StaticVertexBuffer max<%d> len<%zu>\n", iMax, iMax * sizeof(T));
  }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T> class DynamicVertexBuffer : public CVtxBuffer<T> {
  /*virtual*/ bool IsStatic() const {
    return false;
  }

public:
  DynamicVertexBuffer(int iMax, int iFlush, PrimitiveType eType)
      : CVtxBuffer<T>(iMax, iFlush, eType) {
    // printf("DynamicVertexBuffer max<%d> len<%zu>\n", iMax, iMax * sizeof(T));
  }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VtxV12 {
  F32 x, y, z; // 12

  VtxV12()
      : x(0.0f)
      , y(0.0f)
      , z(0.0f) {
  }

  VtxV12(F32 X, F32 Y, F32 Z)
      : x(X)
      , y(Y)
      , z(Z) {
  }

  void EndianSwap() {
    swapbytes_dynamic(x);
    swapbytes_dynamic(y);
    swapbytes_dynamic(z);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct _VtxV12T8 {
  float x, y, z;
  float u,v ;
};

struct VtxV12T8 {
  fvec3 pos; // 12
  fvec2 uv0; // 20

  VtxV12T8(F32 X, F32 Y, F32 Z)
      : pos(X,Y,Z){
  }
  VtxV12T8(F32 X, F32 Y, F32 Z, F32 U, F32 V)
      : pos(X,Y,Z)
      , uv0(U,V) {
  }

  void EndianSwap() {
    swapbytes_dynamic(pos.x);
    swapbytes_dynamic(pos.y);
    swapbytes_dynamic(pos.z);
    swapbytes_dynamic(uv0.x);
    swapbytes_dynamic(uv0.y);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12T8;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct _VtxV12C4 {
  float x, y, z; 
  uint32_t color;   
};

struct VtxV12C4 {
  F32 x, y, z; // 12
  U32 color;   // 20

  VtxV12C4()
      : x(0.0f)
      , y(0.0f)
      , z(0.0f)
      , color(0xffffffff) {
  }

  VtxV12C4(F32 X, F32 Y, F32 Z)
      : x(X)
      , y(Y)
      , z(Z)
      , color(0xffffffff) {
  }
  VtxV12C4(F32 X, F32 Y, F32 Z, U32 c)
      : x(X)
      , y(Y)
      , z(Z)
      , color(c) {
  }

  void EndianSwap() {
    swapbytes_dynamic(x);
    swapbytes_dynamic(y);
    swapbytes_dynamic(z);
    swapbytes_dynamic(color);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12C4;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N6I1T4 {
  F32 mX, mY, mZ;    // 12
  S16 mNX, mNY, mNZ; // 18
  U8 mBone;          // 19
  U8 mPad;           // 20
  S16 mU, mV;        // 24

  SVtxV12N6I1T4()
      : mX(0.0f)
      , mY(0.0f)
      , mZ(0.0f)
      , mNX(0)
      , mNY(0)
      , mNZ(0)
      , mBone(0)
      , mPad(0)
      , mU(0)
      , mV(0) {
  }

  SVtxV12N6I1T4(F32 X, F32 Y, F32 Z, S16 NX, S16 NY, S16 NZ, int ibone, S16 U, S16 V)
      : mX(X)
      , mY(Y)
      , mZ(Z)
      , mNX(NX)
      , mNY(NY)
      , mNZ(NZ)
      , mBone(U8(ibone))
      , mPad(0)
      , mU(U)
      , mV(V) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mX);
    swapbytes_dynamic(mY);
    swapbytes_dynamic(mZ);
    swapbytes_dynamic(mNZ);
    swapbytes_dynamic(mNX);
    swapbytes_dynamic(mNY);
    swapbytes_dynamic(mU);
    swapbytes_dynamic(mV);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N6I1T4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N6C2T4 // WII rigid format
{
  F32 mX, mY, mZ;    // 0  12
  S16 mNX, mNY, mNZ; // 12 18
  U16 mColor;        // 18 20
  S16 mU, mV;        // 20 24

  SVtxV12N6C2T4()
      : mX(0.0f)
      , mY(0.0f)
      , mZ(0.0f)
      , mNX(0)
      , mNY(0)
      , mNZ(0)
      , mColor(0)
      , mU(0)
      , mV(0) {
  }

  SVtxV12N6C2T4(F32 X, F32 Y, F32 Z, S16 NX, S16 NY, S16 NZ, U16 color, S16 U, S16 V)
      : mX(X)
      , mY(Y)
      , mZ(Z)
      , mNX(NX)
      , mNY(NY)
      , mNZ(NZ)
      , mColor(color)
      , mU(U)
      , mV(V) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mX);
    swapbytes_dynamic(mY);
    swapbytes_dynamic(mZ);
    swapbytes_dynamic(mNZ);
    swapbytes_dynamic(mNX);
    swapbytes_dynamic(mNY);
    swapbytes_dynamic(mColor);
    swapbytes_dynamic(mU);
    swapbytes_dynamic(mV);
    // orkprintf( "wii: SVtxV12N6C2T4 <u %08x> <v %08x>\n", int(mU), int(mV) );
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N6C2T4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12I4N12T8 {
  fvec3 mPosition;
  U32 mIDX;
  fvec3 mNormal;
  fvec2 mUV0;

  SVtxV12I4N12T8(const fvec3& pos = fvec3(), const fvec3& nrm = fvec3(), const fvec2& uv = fvec2(), U32 idx = 0)
      : mPosition(pos)
      , mIDX(idx)
      , mNormal(nrm)
      , mUV0(uv) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12I4N12T8;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N12B12T8 {
  fvec3 mPosition;
  fvec3 mNormal;
  fvec3 mBiNormal;
  fvec2 mUV0;

  SVtxV12N12B12T8(const fvec3& pos = fvec3(), const fvec3& nrm = fvec3(), const fvec3& binrm = fvec3(), const fvec2& uv = fvec2())
      : mPosition(pos)
      , mNormal(nrm)
      , mBiNormal(binrm)
      , mUV0(uv) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N12B12T8;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N12B12T8C4 {
  fvec3 mPosition;
  fvec3 mNormal;
  fvec3 mBiNormal;
  fvec2 mUV0;
  U32 mColor;

  SVtxV12N12B12T8C4(
      const fvec3& pos   = fvec3(),
      const fvec3& nrm   = fvec3(),
      const fvec3& binrm = fvec3(),
      const fvec2& uv    = fvec2(),
      const U32 clr      = 0xffffffff)
      : mPosition(pos)
      , mNormal(nrm)
      , mBiNormal(binrm)
      , mUV0(uv)
      , mColor(clr) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mPosition[0]);
    swapbytes_dynamic(mPosition[1]);
    swapbytes_dynamic(mPosition[2]);

    swapbytes_dynamic(mNormal[0]);
    swapbytes_dynamic(mNormal[1]);
    swapbytes_dynamic(mNormal[2]);

    swapbytes_dynamic(mBiNormal[0]);
    swapbytes_dynamic(mBiNormal[1]);
    swapbytes_dynamic(mBiNormal[2]);

    swapbytes_dynamic(mUV0[0]);
    swapbytes_dynamic(mUV0[1]);

    swapbytes_dynamic(mColor);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N12B12T8C4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N12B12T16 {
  fvec3 mPosition;
  fvec3 mNormal;
  fvec3 mBiNormal;
  fvec2 mUV0;
  fvec2 mUV1;

  SVtxV12N12B12T16(
      const fvec3& pos   = fvec3(),
      const fvec3& nrm   = fvec3(),
      const fvec3& binrm = fvec3(),
      const fvec2& uv0   = fvec2(),
      const fvec2& uv1   = fvec2())
      : mPosition(pos)
      , mNormal(nrm)
      , mBiNormal(binrm)
      , mUV0(uv0)
      , mUV1(uv1) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mPosition[0]);
    swapbytes_dynamic(mPosition[1]);
    swapbytes_dynamic(mPosition[2]);

    swapbytes_dynamic(mNormal[0]);
    swapbytes_dynamic(mNormal[1]);
    swapbytes_dynamic(mNormal[2]);

    swapbytes_dynamic(mBiNormal[0]);
    swapbytes_dynamic(mBiNormal[1]);
    swapbytes_dynamic(mBiNormal[2]);

    swapbytes_dynamic(mUV0[0]);
    swapbytes_dynamic(mUV0[1]);

    swapbytes_dynamic(mUV1[0]);
    swapbytes_dynamic(mUV1[1]);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N12B12T16;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N12T16C4 {
  fvec3 mPosition;
  fvec3 mNormal;
  fvec2 mUV0;
  fvec2 mUV1;
  U32 mColor;

  SVtxV12N12T16C4(
      const fvec3& pos = fvec3(),
      const fvec3& nrm = fvec3(),
      const fvec2& uv0 = fvec2(),
      const fvec2& uv1 = fvec2(),
      const U32 clr    = 0xffffffff)
      : mPosition(pos)
      , mNormal(nrm)
      , mUV0(uv0)
      , mUV1(uv1)
      , mColor(clr) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mPosition[0]);
    swapbytes_dynamic(mPosition[1]);
    swapbytes_dynamic(mPosition[2]);

    swapbytes_dynamic(mNormal[0]);
    swapbytes_dynamic(mNormal[1]);
    swapbytes_dynamic(mNormal[2]);

    swapbytes_dynamic(mUV0[0]);
    swapbytes_dynamic(mUV0[1]);

    swapbytes_dynamic(mUV1[0]);
    swapbytes_dynamic(mUV1[1]);

    swapbytes_dynamic(mColor);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N12T16C4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N12T8I4W4 {
  fvec3 mPosition;
  fvec3 mNormal;
  fvec2 mUV0;
  U32 mBoneIndices;
  U32 mBoneWeights;

  SVtxV12N12T8I4W4(
      const fvec3& pos = fvec3(),
      const fvec3& nrm = fvec3(),
      const fvec2& uv  = fvec2(),
      U32 BoneIndices  = 0,
      U32 BoneWeights  = 0)
      : mPosition(pos)
      , mNormal(nrm)
      , mUV0(uv)
      , mBoneIndices(BoneIndices)
      , mBoneWeights(BoneWeights) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mPosition[0]);
    swapbytes_dynamic(mPosition[1]);
    swapbytes_dynamic(mPosition[2]);

    swapbytes_dynamic(mNormal[0]);
    swapbytes_dynamic(mNormal[1]);
    swapbytes_dynamic(mNormal[2]);

    swapbytes_dynamic(mBoneIndices);
    swapbytes_dynamic(mBoneWeights);

    swapbytes_dynamic(mUV0[0]);
    swapbytes_dynamic(mUV0[1]);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N12T8I4W4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12N12B12T8I4W4 {
  fvec3 mPosition;
  fvec3 mNormal;
  fvec3 mBiNormal;
  fvec2 mUV0;
  U32 mBoneIndices;
  U32 mBoneWeights;

  SVtxV12N12B12T8I4W4(
      const fvec3& pos   = fvec3(),
      const fvec3& nrm   = fvec3(),
      const fvec3& binrm = fvec3(),
      const fvec2& uv    = fvec2(),
      U32 BoneIndices    = 0,
      U32 BoneWeights    = 0)
      : mPosition(pos)
      , mNormal(nrm)
      , mBiNormal(binrm)
      , mUV0(uv)
      , mBoneIndices(BoneIndices)
      , mBoneWeights(BoneWeights) {
  }

  void EndianSwap() {
    swapbytes_dynamic(mPosition[0]);
    swapbytes_dynamic(mPosition[1]);
    swapbytes_dynamic(mPosition[2]);

    swapbytes_dynamic(mNormal[0]);
    swapbytes_dynamic(mNormal[1]);
    swapbytes_dynamic(mNormal[2]);

    swapbytes_dynamic(mBiNormal[0]);
    swapbytes_dynamic(mBiNormal[1]);
    swapbytes_dynamic(mBiNormal[2]);

    swapbytes_dynamic(mBoneIndices);
    swapbytes_dynamic(mBoneWeights);

    swapbytes_dynamic(mUV0[0]);
    swapbytes_dynamic(mUV0[1]);
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12N12B12T8I4W4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12C4N6I2T8 {
  F32 mX, mY, mZ;
  U32 mColor;
  S16 mNX, mNY, mNZ;
  U16 mIDX;
  F32 mU, mV;

  SVtxV12C4N6I2T8(F32 X, F32 Y, F32 Z, U16 idx, U32 color, S16 NX, S16 NY, S16 NZ, F32 U, F32 V)
      : mX(X)
      , mY(Y)
      , mZ(Z)
      , mColor(color)
      , mNX(NX)
      , mNY(NY)
      , mNZ(NZ)
      , mIDX(idx)
      , mU(U)
      , mV(V) {
  }

  SVtxV12C4N6I2T8()
      : mX(0.0f)
      , mY(0.0f)
      , mZ(0.0f)
      , mColor(0xffffffff)
      , mNX(0)
      , mNY(0)
      , mNZ(0)
      , mIDX(0)
      , mU(0.0f)
      , mV(0.0f) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12C4N6I2T8;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV6I2C4N3T2 {
  S16 mX, mY, mZ;
  U16 mIDX;
  U32 mColor;
  S8 mNX, mNY, mNZ;
  U8 mU, mV;

  SVtxV6I2C4N3T2(S16 X, S16 Y, S16 Z, U16 idx, U32 color, S8 NX, S8 NY, S8 NZ, U8 U, U8 V)
      : mX(X)
      , mY(Y)
      , mZ(Z)
      , mIDX(idx)
      , mColor(color)
      , mNX(NX)
      , mNY(NY)
      , mNZ(NZ)
      , mU(U)
      , mV(V) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V6I2C4N3T2;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV12C4T16 // 32 BPV
{
  SVtxV12C4T16(F32 iX = 0.0f, F32 iY = 0.0f, F32 iZ = 0.0f, F32 fU = 0.0f, F32 fV = 0.0f, U32 uColor = 0xffffffff)
      : _position(iX, iY, iZ)
      , _color(uColor)
      , _uv0(fU, fV) {
  }

  SVtxV12C4T16(F32 iX, F32 iY, F32 iZ, F32 fU, F32 fV, F32 fU2, F32 fV2, U32 uColor = 0xffffffff)
      : _position(iX, iY, iZ)
      , _color(uColor)
      , _uv0(fU, fV)
      , _uv1(fU2, fV2) {
  }

  SVtxV12C4T16(const fvec3& pos, const fvec2& uv, U32 uColor = 0xffffffff)
      : _position(pos)
      , _color(uColor)
      , _uv0(uv)
      , _uv1(uv) {
  }

  SVtxV12C4T16(const fvec3& pos, const fvec2& uv, const fvec2& uv2, U32 uColor = 0xffffffff)
      : _position(pos)
      , _color(uColor)
      , _uv0(uv)
      , _uv1(uv2) {
  }

  void EndianSwap() {
  }

  fvec3 _position; // 12
  U32 _color;      // 16
  fvec2 _uv0;      // 24
  fvec2 _uv1;      // 32

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V12C4T16;
}; // namespace lev2

///////////////////////////////////////////////////////////////////////////////
// UI Vertex Format for lines/quads (non Textured, colored)

struct SVtxV4C4 // 8 BPV
{
  S16 miX;     // 2
  S16 miY;     // 4
  U32 muColor; // 8

  SVtxV4C4(S16 iX, S16 iY, U32 uColor)
      : miX(iX)
      , miY(iY)
      , muColor(uColor) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V4C4;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxV4T4 // 8 BPV	PreXF 2D (all on top)
{
  S16 miX; // 2
  S16 miY; // 4
  S16 miU; // 6
  S16 miV; // 8

  SVtxV4T4(S16 iX, S16 iY, S16 iU, S16 iV)
      : miX(iX)
      , miY(iY)
      , miU(iU)
      , miV(iV) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V4T4;
};

struct SVtxV4T4C4 // 8 BPV	PreXF 2D (all on top)
{
  S16 miX; // 2
  S16 miY; // 4
  S16 miU; // 6
  S16 miV; // 8
  U32 muColor;

  SVtxV4T4C4(S16 iX, S16 iY, S16 iU, S16 iV, U32 uColor)
      : miX(iX)
      , miY(iY)
      , miU(iU)
      , miV(iV)
      , muColor(uColor) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V4T4C4;
};

///////////////////////////////////////////////////////////////////////////////
// fat testing format

struct SVtxV16T16C16 // 48 BPV
{
  float miPX;
  float miPY;
  float miPZ;
  float miPW;
  float miTU;
  float miTV;
  float miTW;
  float miTQ;
  float miCR;
  float miCG;
  float miCB;
  float miCA;

  explicit SVtxV16T16C16(
      float px,
      float py,
      float pz,
      float pw, //
      float tu,
      float tv,
      float tw,
      float tq, //
      float cr,
      float cg,
      float cb,
      float ca) //
      : miPX(px)
      , miPY(py)
      , miPZ(pz)
      , miPW(pw) //
      , miTU(tu)
      , miTV(tv)
      , miTW(tw)
      , miTQ(tq) //
      , miCR(cr)
      , miCG(cg)
      , miCB(cb)
      , miCA(ca) {
  }

  explicit SVtxV16T16C16(
      fvec4 pos,  //
      fvec4 uvwq, //
      fvec4 rgba) //
      : miPX(pos.x)
      , miPY(pos.y)
      , miPZ(pos.z)
      , miPW(pos.w) //
      , miTU(uvwq.x)
      , miTV(uvwq.y)
      , miTW(uvwq.z)
      , miTQ(uvwq.w) //
      , miCR(rgba.x)
      , miCG(rgba.y)
      , miCB(rgba.z)
      , miCA(rgba.w) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::V16T16C16;
};

///////////////////////////////////////////////////////////////////////////////

struct SVtxMODELERRIGID {
  F32 mX, mY, mZ;        // 12
  U32 mObjectID;         // 16
  U32 mFaceID;           // 20
  U32 mVertexID;         // 24
  U32 mColor;            // 28
  S16 mNX, mNY, mNZ, mS; // 36
  S16 mBX, mBY, mBZ, mT; // 40
  S16 mU0, mV0;          // 48
  S16 mU1, mV1;          // 52
  S16 mU2, mV2;          // 56
  S16 mU3, mV3;          // 60
  S16 mU4, mV4;          // 64
  f32 fU, fV;

  SVtxMODELERRIGID() {
  }

  SVtxMODELERRIGID(F32 X, F32 Y, F32 Z, U32 obj, U32 cmp, U32 clr, S16 NX, S16 NY, S16 BX, S16 BY, S16 u0, S16 v0, S16 u1, S16 v1)
      : mX(X)
      , mY(Y)
      , mZ(Z)
      , mObjectID(obj)
      , mFaceID(0)
      , mVertexID(0)
      , mColor(clr)
      , mNX(NX)
      , mNY(NY)
      , mNZ(0)
      , mS(0)
      , mBX(BX)
      , mBY(BY)
      , mBZ(0)
      , mT(0)
      , mU0(u0)
      , mV0(v0)
      , mU1(u1)
      , mV1(v1)
      , mU2(0)
      , mV2(0)
      , mU3(0)
      , mV3(0)
      , mU4(0)
      , mV4(0) {
  }

  SVtxMODELERRIGID(const fvec4& Pos, U32 obj, U32 cmp, U32 clr, S16 NX, S16 NY, S16 BX, S16 BY, S16 u0, S16 v0, S16 u1, S16 v1)
      : mObjectID(obj)
      , mFaceID(0)
      , mVertexID(0)
      , mColor(clr)
      , mNX(NX)
      , mNY(NY)
      , mNZ(0)
      , mS(0)
      , mBX(BX)
      , mBY(BY)
      , mBZ(0)
      , mT(0)
      , mU0(u0)
      , mV0(v0)
      , mU1(u1)
      , mV1(v1)
      , mU2(0)
      , mV2(0)
      , mU3(0)
      , mV3(0)
      , mU4(0)
      , mV4(0) {
  }

  void EndianSwap() {
  }

  constexpr static EVtxStreamFormat meFormat = EVtxStreamFormat::MODELERRIGID;
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::lev2
