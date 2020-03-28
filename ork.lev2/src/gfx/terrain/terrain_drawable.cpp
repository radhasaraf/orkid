///////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////

#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/gfxprimitives.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/util/crc64.h>
#include <ork/math/polar.h>
#include <ork/pch.h>
#include <ork/rtti/downcast.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/kernel/msgrouter.inl>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/pickbuffer.h>
#include <ork/lev2/gfx/renderer/renderer.h>
#include <ork/lev2/gfx/material_freestyle.inl>
#include <ork/lev2/gfx/terrain/terrain_drawable.h>
#include <ork/util/endian.h>
///////////////////////////////////////////////////////////////////////////////
#include <ork/reflect/AccessorObjectPropertyType.hpp>
#include <ork/reflect/DirectObjectPropertyType.hpp>
#include <ork/kernel/datacache.inl>
///////////////////////////////////////////////////////////////////////////////
using namespace ork::lev2;
ImplementReflectionX(ork::lev2::TerrainDrawableData, "TerrainDrawableData");
///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2 {
///////////////////////////////////////////////////////////////////////////////

typedef SVtxV12C4T16 vertex_type;

enum PatchType {
  PT_A = 0,
  PT_BT,
  PT_BB,
  PT_BL,
  PT_BR,
  PT_C,
};

///////////////////////////////////////////////////////////////////////////////

struct Patch {
  PatchType _type;
  int _lod;
  int _x, _z;
};

///////////////////////////////////////////////////////////////////////////////

struct SectorLodInfo {
  std::vector<Patch> _patches;
  size_t triangle_count = 0;
  size_t vertex_count   = 0;
  VtxWriter<vertex_type> vwriter;
  TerVtxBuffersType* _vtxbuflist;
  bool _islod0 = false;

  size_t countTriangles() {
    triangle_count = 0;
    for (auto p : _patches) {

      // printf("p<%d %d> t<%d>\n", p._x, p._z, p._type);
      switch (p._type) {
        case PT_A: //
          triangle_count += 8;
          break;
        case PT_BT:
        case PT_BL:
        case PT_BB:
        case PT_BR:
          triangle_count += 5;
          break;
        case PT_C:
          triangle_count += 4;
          break;
      }
    }
    return triangle_count;
  }
  void createVB() {
    vertex_count = _patches.size() * 24;
    _vtxbuflist  = new TerVtxBuffersType;
    auto vbuf    = new StaticVertexBuffer<vertex_type>(vertex_count, 0, EPRIM_POINTS);
    vbuf->Reset();
    _vtxbuflist->push_back(vbuf);

    // printf("sector<%p> triangle_count<%zu>\n", this, triangle_count);
    // printf("sector<%p> vertex_count<%zu>\n", this, vertex_count);
  }
};

///////////////////////////////////////////////////////////////////////////////

struct SectorInfo {
  SectorLodInfo _lod0;
  SectorLodInfo _lodX;
};

///////////////////////////////////////////////////////////////////////////////

struct TerrainRenderImpl {

  TerrainRenderImpl(TerrainDrawableInst* hfdrw);
  ~TerrainRenderImpl();
  void gpuUpdate(Context* ptarg);
  void render(const RenderContextInstData& RCID);

  datablockptr_t recomputeTextures(Context* ptarg);
  void reloadCachedTextures(Context* ptarg, datablockptr_t dblock);

  TerrainDrawableInst* _hfinstance;
  hfptr_t _heightfield;

  bool _gpuDataDirty                          = true;
  FreestyleMaterial* _terrainMaterial         = nullptr;
  const FxShaderTechnique* _tekBasic          = nullptr;
  const FxShaderTechnique* _tekDefGbuf1       = nullptr;
  const FxShaderTechnique* _tekStereo         = nullptr;
  const FxShaderTechnique* _tekDefGbuf1Stereo = nullptr;
  const FxShaderTechnique* _tekPick           = nullptr;

  const FxShaderParam* _parMatVPL       = nullptr;
  const FxShaderParam* _parMatVPC       = nullptr;
  const FxShaderParam* _parMatVPR       = nullptr;
  const FxShaderParam* _parCamPos       = nullptr;
  const FxShaderParam* _parTexA         = nullptr;
  const FxShaderParam* _parTexB         = nullptr;
  const FxShaderParam* _parTexEnv       = nullptr;
  const FxShaderParam* _parModColor     = nullptr;
  const FxShaderParam* _parTime         = nullptr;
  const FxShaderParam* _parTestXXX      = nullptr;
  const FxShaderParam* _parFogColor     = nullptr;
  const FxShaderParam* _parGrass        = nullptr;
  const FxShaderParam* _parSnow         = nullptr;
  const FxShaderParam* _parRock1        = nullptr;
  const FxShaderParam* _parRock2        = nullptr;
  const FxShaderParam* _parGblendYscale = nullptr;
  const FxShaderParam* _parGblendYbias  = nullptr;
  const FxShaderParam* _parGblendStepLo = nullptr;
  const FxShaderParam* _parGblendStepHi = nullptr;
  Texture* _heightmapTextureA           = nullptr;
  Texture* _heightmapTextureB           = nullptr;
  fvec3 _aabbmin;
  fvec3 _aabbmax;
  SectorInfo _sector[8];
  lev2::textureassetptr_t _sphericalenvmap = nullptr;
};

///////////////////////////////////////////////////////////////////////////////

TerrainRenderImpl::TerrainRenderImpl(TerrainDrawableInst* hfinst) {
  _hfinstance  = hfinst;
  _heightfield = std::make_shared<HeightMap>(0, 0);
}

///////////////////////////////////////////////////////////////////////////////

TerrainRenderImpl::~TerrainRenderImpl() {
  if (_heightmapTextureA) {
    delete _heightmapTextureA;
  }
  if (_heightmapTextureB) {
    delete _heightmapTextureB;
  }
}

///////////////////////////////////////////////////////////////////////////////
datablockptr_t TerrainRenderImpl::recomputeTextures(Context* ptarg) {

  ork::Timer timer;
  timer.Start();

  datablockptr_t dblock = std::make_shared<DataBlock>();

  int MIPW = _heightfield->GetGridSizeX();
  int MIPH = _heightfield->GetGridSizeZ();

  dblock->addItem<int>(MIPW);
  dblock->addItem<int>(MIPH);

  auto chainA = new MipChain(MIPW, MIPH, EBufferFormat::RGBA32F, ETEXTYPE_2D);
  auto chainB = new MipChain(MIPW, MIPH, EBufferFormat::RGBA32F, ETEXTYPE_2D);

  auto mipA0      = chainA->_levels[0];
  auto mipB0      = chainB->_levels[0];
  auto pfloattexA = (float*)mipA0->_data;
  auto pfloattexB = (float*)mipB0->_data;
  assert(pfloattexA != nullptr);
  assert(pfloattexB != nullptr);

  fvec2 origin(0, 0);

  auto heightdata = (float*)_heightfield->GetHeightData();

  const bool debugmip = false;

  fvec3 mipdebugcolors[12] = {

      fvec3(0, 0, 0), // 0
      fvec3(0, 0, 1), // 1
      fvec3(0, 1, 0), // 2
      fvec3(0, 1, 1), // 3
      fvec3(1, 0, 0), // 4
      fvec3(1, 0, 1), // 5
      fvec3(1, 1, 0), // 6
      fvec3(1, 1, 1), // 7
      fvec3(0, 0, 0), // 0
      fvec3(0, 0, 1), // 1
      fvec3(0, 1, 0), // 2
      fvec3(0, 1, 1), // 3
  };

  size_t miplen = sizeof(fvec4) * MIPW * MIPH;

  dblock->reserve(miplen * 2);

  for (ssize_t z = 0; z < MIPH; z++) {
    ssize_t zz = z - (MIPH >> 1);
    float fzz  = float(zz) / float(MIPH >> 1); // -1 .. 1
    for (ssize_t x = 0; x < MIPW; x++) {
      ssize_t xx = x - (MIPW >> 1);
      float fxx  = float(xx) / float(MIPW >> 1);
      fvec2 pos2d(fxx, fzz);
      float d                   = (pos2d - origin).Mag();
      float dpow                = powf(d, 3);
      size_t index              = z * MIPW + x;
      float h                   = heightdata[index];
      size_t pixelbase          = index * 4;
      pfloattexA[pixelbase + 0] = float(xx);
      pfloattexA[pixelbase + 1] = float(h);
      pfloattexA[pixelbase + 2] = float(zz);
      pfloattexB[pixelbase + 0] = float(h);
      ///////////////////
      // compute normal
      ///////////////////
      if (x > 0 and z > 0) {
        size_t xxm1       = x - 1;
        float fxxm1       = float(xxm1);
        size_t zzm1       = z - 1;
        float fzzm1       = float(zzm1);
        size_t index_dxm1 = (z * MIPW) + xxm1;
        size_t index_dzm1 = (zzm1 * MIPW) + x;
        float hd0         = heightdata[index] * 1500;
        float hdx         = heightdata[index_dxm1] * 1500;
        float hdz         = heightdata[index_dzm1] * 1500;
        float fzz         = float(zz);
        fvec3 pos3d(x * 2, hd0, z * 2);
        fvec3 pos3d_dx(xxm1 * 2, hdx, z * 2);
        fvec3 pos3d_dz(x * 2, hdz, zzm1 * 2);

        fvec3 e01                 = (pos3d_dx - pos3d).Normal();
        fvec3 e02                 = (pos3d_dz - pos3d).Normal();
        auto n                    = e02.Cross(e01).Normal();
        pfloattexB[pixelbase + 1] = debugmip ? 0.0f : float(n.x); // r x
        pfloattexB[pixelbase + 2] = debugmip ? 0.0f : float(n.y); // g y
        pfloattexB[pixelbase + 3] = debugmip ? 0.0f : float(n.z); // b z

      } // if(x>0 and z>0){
    }   // for( size_t x=0; x<MIPW; x++ ){
  }     // for( size_t z=0; z<MIPH; z++ ){

  dblock->addData(pfloattexA, miplen);
  dblock->addData(pfloattexB, miplen);

  /////////////////////////////
  // compute mips
  /////////////////////////////

  int levindex = 0;
  MIPW >>= 1;
  MIPH >>= 1;
  while (MIPW >= 2 and MIPH >= 2) {
    size_t miplen = sizeof(fvec4) * MIPW * MIPH;
    assert((levindex + 1) < chainA->_levels.size());
    auto prevlevA = chainA->_levels[levindex];
    auto nextlevA = chainA->_levels[levindex + 1];
    auto prevlevB = chainB->_levels[levindex];
    auto nextlevB = chainB->_levels[levindex + 1];
    // printf("levindex<%d> prevlev<%p> nextlev<%p>\n", levindex, prevlevA.get(), nextlevA.get());
    auto prevbasA = (float*)prevlevA->_data;
    auto nextbasA = (float*)nextlevA->_data;
    auto prevbasB = (float*)prevlevB->_data;
    auto nextbasB = (float*)nextlevB->_data;
    ////////////////////////////////////////////////
    constexpr float kdiv9 = 1.0f / 9.0f;
    int MAXW              = (MIPW * 2 - 1);
    int MAXH              = (MIPH * 2 - 1);
    for (int y = 0; y < MIPH; y++) {
      for (int x = 0; x < MIPW; x++) {
        ///////////////////////////////////////////
        int plx = x * 2;
        int ply = y * 2;
        ///////////////////////////////////////////
        int plxm1 = std::clamp(plx - 1, 0, MAXW);
        int plxp1 = std::clamp(plx + 1, 0, MAXW);
        int plyp1 = std::clamp(ply + 1, 0, MAXH);
        int plym1 = std::clamp(ply - 1, 0, MAXH);
        ///////////////////////////////////////////
        auto& dest_sampleA = nextlevA->sample<fvec4>(x, y);
        auto& dest_sampleB = nextlevB->sample<fvec4>(x, y);
        ///////////////////////////////////////////
        dest_sampleA = prevlevA->sample<fvec4>(plxm1, plym1);
        dest_sampleA += prevlevA->sample<fvec4>(plx, plym1);
        dest_sampleA += prevlevA->sample<fvec4>(plxp1, plym1);
        dest_sampleA += prevlevA->sample<fvec4>(plxm1, ply);
        dest_sampleA += prevlevA->sample<fvec4>(plx, ply);
        dest_sampleA += prevlevA->sample<fvec4>(plxp1, ply);
        dest_sampleA += prevlevA->sample<fvec4>(plxm1, plyp1);
        dest_sampleA += prevlevA->sample<fvec4>(plx, plyp1);
        dest_sampleA += prevlevA->sample<fvec4>(plxp1, plyp1);
        ///////////////////////////////////////////
        dest_sampleB = prevlevB->sample<fvec4>(plxm1, plym1);
        dest_sampleB += prevlevB->sample<fvec4>(plx, plym1);
        dest_sampleB += prevlevB->sample<fvec4>(plxp1, plym1);
        dest_sampleB += prevlevB->sample<fvec4>(plxm1, ply);
        dest_sampleB += prevlevB->sample<fvec4>(plx, ply);
        dest_sampleB += prevlevB->sample<fvec4>(plxp1, ply);
        dest_sampleB += prevlevB->sample<fvec4>(plxm1, plyp1);
        dest_sampleB += prevlevB->sample<fvec4>(plx, plyp1);
        dest_sampleB += prevlevB->sample<fvec4>(plxp1, plyp1);
        ///////////////////////////////////////////
        dest_sampleA.x *= kdiv9;
        dest_sampleA.y *= kdiv9;
        dest_sampleA.z *= kdiv9;
        dest_sampleA.w *= kdiv9;
        //
        dest_sampleB.x *= kdiv9;
        dest_sampleB.y *= kdiv9;
        dest_sampleB.z *= kdiv9;
        dest_sampleB.w *= kdiv9;
        ///////////////////////////////////////////
        if (debugmip) {
          auto& dm       = mipdebugcolors[levindex];
          dest_sampleB.y = dm.x;
          dest_sampleB.z = dm.y;
          dest_sampleB.w = dm.z;
        }
      }
    }
    ////////////////////////////////////////////////
    dblock->addData(nextbasA, miplen);
    dblock->addData(nextbasB, miplen);
    ////////////////////////////////////////////////
    MIPW >>= 1;
    MIPH >>= 1;
    levindex++;
  }

  ////////////////////////////////////////////////////////////////

  _heightmapTextureA = ptarg->TXI()->createFromMipChain(chainA);
  _heightmapTextureB = ptarg->TXI()->createFromMipChain(chainB);

  delete chainA;
  delete chainB;

  float runtime = timer.SecsSinceStart();
  // printf( "recomputeTextures runtime<%g>\n", runtime );
  // printf( "recomputeTextures dblocklen<%zu>\n", dblock->_data.GetSize() );

  return dblock;
}

///////////////////////////////////////////////////////////////////////////////

void TerrainRenderImpl::reloadCachedTextures(Context* ptarg, datablockptr_t dblock) {
  chunkfile::InputStream istr(dblock->data(), dblock->length());
  int MIPW, MIPH;
  istr.GetItem<int>(MIPW);
  istr.GetItem<int>(MIPH);
  assert(MIPW == _heightfield->GetGridSizeX());
  assert(MIPH == _heightfield->GetGridSizeZ());
  auto chainA = new MipChain(MIPW, MIPH, EBufferFormat::RGBA32F, ETEXTYPE_2D);
  auto chainB = new MipChain(MIPW, MIPH, EBufferFormat::RGBA32F, ETEXTYPE_2D);
  // printf( "reloadCachedTextures ostr.len<%zu> nmips<%zu>\n", ostr.GetSize(), chainA->_levels.size() );
  int levindex = 0;
  while (MIPW >= 2 and MIPH >= 2) {
    int CHECKMIPW, CHECKMIPH;
    size_t levlen = sizeof(fvec4) * MIPW * MIPH;
    auto pa       = (const float*)istr.GetCurrent();
    auto leva     = chainA->_levels[levindex];
    auto levb     = chainB->_levels[levindex];
    memcpy(leva->_data, pa, levlen);
    istr.advance(levlen);
    auto pb = (const float*)istr.GetCurrent();
    memcpy(levb->_data, pb, levlen);
    istr.advance(levlen);
    // printf( "reloadmip lev<%d> w<%d> h<%d> pa<%p> pb<%p>\n", levindex, MIPW, MIPH, pa, pb );
    MIPW >>= 1;
    MIPH >>= 1;
    levindex++;
  }
  assert(istr.midx == istr.GetLength());
  _heightmapTextureA = ptarg->TXI()->createFromMipChain(chainA);
  _heightmapTextureB = ptarg->TXI()->createFromMipChain(chainB);
  delete chainA;
  delete chainB;
}

///////////////////////////////////////////////////////////////////////////////

void TerrainRenderImpl::gpuUpdate(Context* ptarg) {
  if (false == _gpuDataDirty)
    return;

  if (nullptr == _terrainMaterial) {
    _terrainMaterial = new FreestyleMaterial;
    _terrainMaterial->gpuInit(ptarg, "orkshader://terrain");
    _tekBasic          = _terrainMaterial->technique("terrain");
    _tekStereo         = _terrainMaterial->technique("terrain_stereo");
    _tekPick           = _terrainMaterial->technique("pick");
    _tekDefGbuf1       = _terrainMaterial->technique("terrain_gbuf1");
    _tekDefGbuf1Stereo = _terrainMaterial->technique("terrain_gbuf1_stereo");

    _parMatVPL   = _terrainMaterial->param("MatMVPL");
    _parMatVPC   = _terrainMaterial->param("MatMVPC");
    _parMatVPR   = _terrainMaterial->param("MatMVPR");
    _parCamPos   = _terrainMaterial->param("CamPos");
    _parTexA     = _terrainMaterial->param("HFAMap");
    _parTexB     = _terrainMaterial->param("HFBMap");
    _parTexEnv   = _terrainMaterial->param("EnvMap");
    _parModColor = _terrainMaterial->param("ModColor");
    _parTime     = _terrainMaterial->param("Time");
    _parTestXXX  = _terrainMaterial->param("testxxx");

    _parFogColor     = _terrainMaterial->param("FogColor");
    _parGrass        = _terrainMaterial->param("GrassColor");
    _parSnow         = _terrainMaterial->param("SnowColor");
    _parRock1        = _terrainMaterial->param("Rock1Color");
    _parRock2        = _terrainMaterial->param("Rock2Color");
    _parGblendYscale = _terrainMaterial->param("GBlendYScale");
    _parGblendYbias  = _terrainMaterial->param("GBlendYBias");
    _parGblendStepLo = _terrainMaterial->param("GBlendStepLo");
    _parGblendStepHi = _terrainMaterial->param("GBlendStepHi");
  }

  auto hmap    = _heightfield;
  bool _loadok = hmap->Load(_hfinstance->hfpath());
  hmap->SetWorldSize(_hfinstance->_worldSizeXZ, _hfinstance->_worldSizeXZ);
  hmap->SetWorldHeight(_hfinstance->_worldHeight);

  boost::Crc64 basehasher;
  basehasher.accumulateItem<uint64_t>(hmap->_hash);
  basehasher.accumulateItem<float>(_hfinstance->_worldSizeXZ);
  basehasher.accumulateItem<float>(_hfinstance->_worldHeight);
  basehasher.finish();
  uint64_t hashkey = basehasher.result();

  ork::Timer timer;
  timer.Start();

  // orkprintf("ComputingGeometry hashkey<0x%llx>\n", hashkey );

  ////////////////////////////////////////////////////////////////
  // create and fill in gpu texture
  ////////////////////////////////////////////////////////////////

  const int iglX           = hmap->GetGridSizeX();
  const int iglZ           = hmap->GetGridSizeZ();
  const int terrain_ngrids = iglX * iglZ;

  if (0 == iglX)
    return;

  auto dblock = DataBlockCache::findDataBlock(hashkey);

  if (dblock) {
    reloadCachedTextures(ptarg, dblock);
  } else {
    dblock = recomputeTextures(ptarg);
    DataBlockCache::setDataBlock(hashkey, dblock);
  }

  ////////////////////////////////////////////////////////////////

  auto bbctr = (_aabbmin + _aabbmax) * 0.5f;
  auto bbdim = (_aabbmax - _aabbmin);

  // printf("IGLX<%d> IGLZ<%d> kworldsizeXZ<%f %f>\n", iglX, iglZ);
  // printf("bbmin<%f %f %f>\n", _aabbmin.GetX(), _aabbmin.GetY(), _aabbmin.GetZ());
  // printf("bbmax<%f %f %f>\n", _aabbmax.GetX(), _aabbmax.GetY(), _aabbmax.GetZ());
  // printf("bbctr<%f %f %f>\n", bbctr.GetX(), bbctr.GetY(), bbctr.GetZ());
  // printf("bbdim<%f %f %f>\n", bbdim.GetX(), bbdim.GetY(), bbdim.GetZ());

  auto sectorID = [&](int x, int z) -> int {
    int rval = 0;
    if (x >= 0) {
      if (z >= 0) {
        if (x >= z) {
          // A
          rval = 0;
        } else {
          // B
          rval = 1;
        }
      } else {
        if (x >= (-z)) {
          // C
          rval = 7;
        } else {
          // D
          rval = 6;
        }
      }
    } else { // x<0
      if (z >= 0) {
        if ((-x) > z) {
          // E
          rval = 3;
        } else {
          // F
          rval = 2;
        }
      } else {
        if ((-x) > (-z)) {
          // G
          rval = 4;
        } else {
          // H
          rval = 5;
        }
      }
    }
    return rval;
  };

  ////////////////////////////////////////////

  auto patch_row = [&](PatchType t, int lod, int x1, int x2, int z) {
    int step = 1 << lod;
    for (int x = x1; x < x2; x += step) {
      Patch p;
      p._type = t;
      p._x    = x;
      p._z    = z;
      p._lod  = lod;
      if (lod == 0)
        _sector[sectorID(x, z)]._lod0._patches.push_back(p);
      else
        _sector[sectorID(x, z)]._lodX._patches.push_back(p);
    }
  };

  ////////////////////////////////////////////

  auto patch_column = [&](PatchType t, int lod, int x, int z1, int z2) {
    int step = 1 << lod;
    for (int z = z1; z < z2; z += step) {
      Patch p;
      p._type = t;
      p._x    = x;
      p._z    = z;
      p._lod  = lod;
      if (lod == 0)
        _sector[sectorID(x, z)]._lod0._patches.push_back(p);
      else
        _sector[sectorID(x, z)]._lodX._patches.push_back(p);
    }
  };

  ////////////////////////////////////////////

  auto patch_block = [&](PatchType t, int lod, int x1, int z1, int x2, int z2) {
    if (x1 == x2)
      x2++;
    if (z1 == z2)
      z2++;

    int step = 1 << lod;

    for (int x = x1; x < x2; x += step) {
      for (int z = z1; z < z2; z += step) {
        Patch p;
        p._type = t;
        p._x    = x;
        p._z    = z;
        p._lod  = lod;
        if (lod == 0)
          _sector[sectorID(x, z)]._lod0._patches.push_back(p);
        else
          _sector[sectorID(x, z)]._lodX._patches.push_back(p);
      }
    }
  };

  ////////////////////////////////////////////

  auto single_patch = [&](PatchType t, int lod, int x, int z) {
    Patch p;
    p._type = t;
    p._x    = x;
    p._z    = z;
    p._lod  = lod;
    if (lod == 0)
      _sector[sectorID(x, z)]._lod0._patches.push_back(p);
    else
      _sector[sectorID(x, z)]._lodX._patches.push_back(p);
  };

  ////////////////////////////////////////////

  struct Iter {
    int _lod    = -1;
    int _acount = 0;
  };

  std::vector<Iter> iters;

  iters.push_back(Iter{0, 256}); // 256*2 = 512m
  iters.push_back(Iter{1, 128}); // 128*4 = 512m   tot(1024m)
  iters.push_back(Iter{2, 64});  // 64*8 = 512  tot(1536)
  iters.push_back(Iter{3, 32});  // 32*16 = 512m tot(2048m) - 2.56mi
  iters.push_back(Iter{4, 16});  // 16*32 = 512m tot(2560m) - 2.56mi
  iters.push_back(Iter{5, 8});   // 8*64 = 512m tot(3072m) - 2.56mi
  // iters.push_back(Iter{4, 128});
  // iters.push_back(Iter{5, 128});

  // printf("Generating Patches..\n");

  int iprevouterd2 = 0;

  for (auto iter : iters) {

    assert(iter._acount >= 4);       // at least one inner
    assert((iter._acount & 1) == 0); // must also be even

    int lod  = iter._lod;
    int step = 1 << lod;

    int newouterd2 = iprevouterd2 + (iter._acount * step / 2);

    int sectdim   = step;
    int sectdimp1 = sectdim + step;
    int a_start   = -(iter._acount << lod) / 2;
    int a_end     = a_start + (iter._acount - 1) * step;
    int a_z       = a_start;

    if (0 == lod) {
      patch_block(
          PT_A,
          lod, // Full Sector
          -newouterd2 + step,
          -newouterd2 + step,
          +newouterd2 - step,
          +newouterd2 - step);
    } else {
      patch_block(
          PT_A,
          lod, // Top Sector
          -newouterd2 + step,
          -newouterd2 + step,
          +newouterd2 - step,
          -iprevouterd2);

      patch_block(
          PT_A,
          lod, // Left Sector
          -newouterd2 + step,
          -iprevouterd2,
          -iprevouterd2,
          +iprevouterd2);

      patch_block(
          PT_A,
          lod, // Right Sector
          iprevouterd2,
          -iprevouterd2,
          newouterd2 - step,
          +iprevouterd2);

      patch_block(
          PT_A,
          lod, // Bottom Sector
          -newouterd2 + step,
          iprevouterd2,
          +newouterd2 - step,
          newouterd2 - step);
    }

    int bx0 = -newouterd2;
    int bx1 = newouterd2 - step;
    patch_row(PT_BT, lod, bx0 + step, bx1, -newouterd2);
    patch_row(PT_BB, lod, bx0 + step, bx1, +newouterd2 - step);
    patch_column(PT_BL, lod, -newouterd2, bx0, bx1);
    patch_column(PT_BR, lod, +newouterd2 - step, bx0, bx1);

    single_patch(PT_C, lod, bx0, bx0);
    single_patch(PT_C, lod, bx1, bx0);
    single_patch(PT_C, lod, bx1, bx1);
    single_patch(PT_C, lod, bx0, bx1);

    iprevouterd2 = newouterd2;
  }

  ////////////////////////////////////////////////////////////////
  // create 1 vertex buffer per 45 degree arc
  ////////////////////////////////////////////////////////////////

  for (int i = 0; i < 8; i++) {
    auto& sector = _sector[i];
    sector._lod0.countTriangles();
    sector._lodX.countTriangles();
    sector._lod0.createVB();
    sector._lodX.createVB();
    sector._lod0._islod0 = true;
    sector._lodX._islod0 = false;
  }

  ////////////////////////////////////////////
  // find/create vertexbuffers
  ////////////////////////////////////////////

  AABox aab;
  aab.BeginGrow();

  ////////////////////////////////////////////

  auto onlod = [&](SectorInfo& info, bool do_lod0) {
    auto& linfo = do_lod0 ? info._lod0 : info._lodX;
    auto vbuf   = (*linfo._vtxbuflist)[0];
    linfo.vwriter.Lock(ptarg, vbuf, linfo.vertex_count);
    ////////////////////////////////////////////
    linfo.triangle_count = 0;
    auto& sector_patches = linfo._patches;
    for (auto p : sector_patches) {
      int x   = p._x;
      int z   = p._z;
      int lod = p._lod;

      if (lod != 0 and do_lod0)
        continue;
      if (lod == 0 and (false == do_lod0))
        continue;

      int step = 1 << lod;

      fvec3 p0(x, lod, z);
      fvec3 p1(x + step, lod, z);
      fvec3 p2(x + step, lod, z + step);
      fvec3 p3(x, lod, z + step);

      aab.Grow(p0);
      aab.Grow(p1);
      aab.Grow(p2);
      aab.Grow(p3);

      uint32_t c0 = 0xff000000;
      uint32_t c1 = 0xff0000ff;
      uint32_t c2 = 0xff00ffff;
      uint32_t c3 = 0xff00ff00;

      // here we should have the patches,
      //  just need to create the prims

      switch (p._type) {
        case PT_A: //
          c0 = 0xff0000ff;
          break;
        case PT_BT:
        case PT_BB:
          c0 = 0xff800080;
          break;
        case PT_BL:
        case PT_BR:
          c0 = 0xff00ff00;
          break;
          break;
        case PT_C:
          c0 = 0xff808080;
          break;
      }

      p0.y = lod;
      p1.y = lod;
      p2.y = lod;
      p3.y = lod;

      auto v0   = vertex_type(p0, fvec2(), c0);
      auto v1   = vertex_type(p1, fvec2(), c0);
      auto v2   = vertex_type(p2, fvec2(), c0);
      auto v3   = vertex_type(p3, fvec2(), c0);
      auto vc   = vertex_type((p0 + p1 + p2 + p3) * 0.25, fvec2(), c0);
      auto vc01 = vertex_type((p0 + p1) * 0.5, fvec2(), c0);
      auto vc12 = vertex_type((p1 + p2) * 0.5, fvec2(), c0);
      auto vc23 = vertex_type((p2 + p3) * 0.5, fvec2(), c0);
      auto vc30 = vertex_type((p3 + p0) * 0.5, fvec2(), c0);

      switch (p._type) {
        case PT_A: //
          linfo.triangle_count += 8;
          linfo.vwriter.AddVertex(v0);
          linfo.vwriter.AddVertex(vc01);
          linfo.vwriter.AddVertex(vc);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc01);
          linfo.vwriter.AddVertex(v1);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v1);
          linfo.vwriter.AddVertex(vc12);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc12);
          linfo.vwriter.AddVertex(v2);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v2);
          linfo.vwriter.AddVertex(vc23);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc23);
          linfo.vwriter.AddVertex(v3);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v3);
          linfo.vwriter.AddVertex(vc30);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc30);
          linfo.vwriter.AddVertex(v0);

          break;
        case PT_BT:
          linfo.triangle_count += 5;
          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v0);
          linfo.vwriter.AddVertex(v1);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v1);
          linfo.vwriter.AddVertex(v2);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v2);
          linfo.vwriter.AddVertex(vc23);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc23);
          linfo.vwriter.AddVertex(v3);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v3);
          linfo.vwriter.AddVertex(v0);
          break;
        case PT_BB:
          linfo.triangle_count += 5;
          linfo.vwriter.AddVertex(v0);
          linfo.vwriter.AddVertex(vc01);
          linfo.vwriter.AddVertex(vc);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc01);
          linfo.vwriter.AddVertex(v1);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v1);
          linfo.vwriter.AddVertex(v2);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v2);
          linfo.vwriter.AddVertex(v3);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v3);
          linfo.vwriter.AddVertex(v0);
          break;
        case PT_BR:
          linfo.triangle_count += 5;
          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v0);
          linfo.vwriter.AddVertex(v1);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v1);
          linfo.vwriter.AddVertex(vc12);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc12);
          linfo.vwriter.AddVertex(v2);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v2);
          linfo.vwriter.AddVertex(v3);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v3);
          linfo.vwriter.AddVertex(v0);
          break;
        case PT_BL:
          linfo.triangle_count += 5;
          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v0);
          linfo.vwriter.AddVertex(v1);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v1);
          linfo.vwriter.AddVertex(v2);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v2);
          linfo.vwriter.AddVertex(v3);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v3);
          linfo.vwriter.AddVertex(vc30);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(vc30);
          linfo.vwriter.AddVertex(v0);
          break;
        case PT_C:
          linfo.triangle_count += 4;
          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v0);
          linfo.vwriter.AddVertex(v1);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v1);
          linfo.vwriter.AddVertex(v2);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v2);
          linfo.vwriter.AddVertex(v3);

          linfo.vwriter.AddVertex(vc);
          linfo.vwriter.AddVertex(v3);
          linfo.vwriter.AddVertex(v0);
          break;
      }
    }
    ////////////////////////////////////////////
    linfo.vwriter.UnLock(ptarg);
  };

  ////////////////////////////////////////////
  for (int i = 0; i < 8; i++) {
    auto& sector = _sector[i];
    onlod(sector, true);
    onlod(sector, false);
  } // for each sector
  aab.EndGrow();
  auto geomin = aab.Min();
  auto geomax = aab.Max();
  auto geosiz = aab.GetSize();

  _aabbmin = geomin;
  _aabbmax = geomax;

  // printf("geomin<%f %f %f>\n", geomin.GetX(), geomin.GetY(), geomin.GetZ());
  // printf("geomax<%f %f %f>\n", geomax.GetX(), geomax.GetY(), geomax.GetZ());
  // printf("geosiz<%f %f %f>\n", geosiz.GetX(), geosiz.GetY(), geosiz.GetZ());

  float runtime = timer.SecsSinceStart();
  // printf( "runtime<%g>\n", runtime );

  _gpuDataDirty = false;
}
///////////////////////////////////////////////////////////////////////////////

void TerrainRenderImpl::render(const RenderContextInstData& RCID) {

  auto raw_drawable         = _hfinstance->_rawdrawable;
  const IRenderer* renderer = RCID.GetRenderer();
  Context* targ             = renderer->GetTarget();
  auto RCFD                 = targ->topRenderContextFrameData();
  const auto& CPD           = RCFD->topCPD();
  bool stereo1pass          = CPD.isStereoOnePass();
  bool bpick                = CPD.isPicking();
  auto mtxi                 = targ->MTXI();
  auto fxi                  = targ->FXI();
  auto gbi                  = targ->GBI();
  ///////////////////////////////////////////////////////////////////
  if (bpick)
    return;
  assert(raw_drawable != nullptr);
  ///////////////////////////////////////////////////////////////////
  // update
  ///////////////////////////////////////////////////////////////////
  gpuUpdate(targ);
  const int iglX           = _heightfield->GetGridSizeX();
  const int iglZ           = _heightfield->GetGridSizeZ();
  const int terrain_ngrids = iglX * iglZ;
  if (terrain_ngrids < 1024)
    return;
  ///////////////////////////////////////////////////////////////////
  // render
  ///////////////////////////////////////////////////////////////////
  //////////////////////////
  fmtx4 viz_offset;
  viz_offset.SetTranslation(_hfinstance->_visualOffset);
  //////////////////////////
  // color
  //////////////////////////
  fvec4 color = fcolor4::White();
  if (bpick) {
    auto pickbuf    = targ->FBI()->currentPickBuffer();
    Object* pickobj = nullptr;
    uint64_t pickid = pickbuf->AssignPickId(pickobj);
    color.SetRGBAU64(pickid);
  } else if (false) { // is_sel ){
    color = fcolor4::Red();
  }
  //////////////////////////
  // env texture
  //////////////////////////
  Texture* ColorTex = nullptr;
  if (_sphericalenvmap && _sphericalenvmap->GetTexture())
    ColorTex = _sphericalenvmap->GetTexture();
  //////////////////////////
  //////////////////////////
  fmtx4 MVPL, MVPC, MVPR;
  //////////////////////////

  fvec3 campos_mono = CPD.monoCamPos(viz_offset);
  fvec3 znormal     = CPD.monoCamZnormal();

  if (stereo1pass) {
    auto stcams = CPD._stereoCameraMatrices;
    MVPL        = stcams->MVPL(viz_offset);
    MVPR        = stcams->MVPR(viz_offset);
    MVPC        = stcams->MVPMONO(viz_offset);
  } else {
    auto mcams             = CPD._cameraMatrices;
    const fmtx4& PMTX_mono = mcams->_pmatrix;
    const fmtx4& VMTX_mono = mcams->_vmatrix;
    auto MV_mono           = (viz_offset * VMTX_mono);
    auto MVP               = MV_mono * PMTX_mono;
    MVPL                   = MVP;
    MVPC                   = MVP;
    MVPR                   = MVP;
  }

  ///////////////////////////////////////////////////////////////////
  // render
  ///////////////////////////////////////////////////////////////////

  auto instance    = _hfinstance;
  const auto& HFDD = instance->_data;

  // auto range = _aabbmax - _aabbmin;

  targ->PushMaterial(_terrainMaterial);
  _terrainMaterial->bindTechnique(stereo1pass ? _tekDefGbuf1Stereo : _tekDefGbuf1);
  _terrainMaterial->begin(*RCFD);
  _terrainMaterial->bindParamMatrix(_parMatVPL, MVPL);
  _terrainMaterial->bindParamMatrix(_parMatVPC, MVPC);
  _terrainMaterial->bindParamMatrix(_parMatVPR, MVPR);
  _terrainMaterial->bindParamCTex(_parTexA, _heightmapTextureA);
  _terrainMaterial->bindParamCTex(_parTexB, _heightmapTextureB);
  _terrainMaterial->bindParamVec3(_parCamPos, campos_mono);
  _terrainMaterial->bindParamVec4(_parModColor, color);
  _terrainMaterial->bindParamFloat(_parTime, 0.0f);

  //_terrainMaterial->bindParamTex(_parTexEnv, HFDD._sphericalenvmap);
  _terrainMaterial->bindParamFloat(_parTestXXX, HFDD._testxxx);

  _terrainMaterial->bindParamVec3(_parFogColor, fvec3(0, 0, 0));
  _terrainMaterial->bindParamVec3(_parGrass, HFDD._grass);
  _terrainMaterial->bindParamVec3(_parSnow, HFDD._snow);
  _terrainMaterial->bindParamVec3(_parRock1, HFDD._rock1);
  _terrainMaterial->bindParamVec3(_parRock2, HFDD._rock2);

  _terrainMaterial->bindParamFloat(_parGblendYscale, HFDD._gblend_yscale);
  _terrainMaterial->bindParamFloat(_parGblendYbias, HFDD._gblend_ybias);
  _terrainMaterial->bindParamFloat(_parGblendStepLo, HFDD._gblend_steplo);
  _terrainMaterial->bindParamFloat(_parGblendStepHi, HFDD._gblend_stephi);

  if (true) { // abs(znormal.y) > 0.8 ){ // looking up or down
    ////////////////////////////////
    // render L0
    ////////////////////////////////
    for (int isector = 0; isector < 8; isector++) {
      auto& sector = _sector[isector];
      auto L0      = sector._lod0;
      auto vbufs   = L0._vtxbuflist;
      int inumvb   = vbufs->size();
      for (int ivb = 0; ivb < inumvb; ivb++) {
        auto vertex_buf = (*vbufs)[ivb];
        gbi->DrawPrimitiveEML(*vertex_buf, EPRIM_TRIANGLES);
      }
    }
    ////////////////////////////////
    // render LX
    ////////////////////////////////
    for (int isector = 0; isector < 8; isector++) {
      auto& sector = _sector[isector];
      auto LX      = sector._lodX;
      auto vbufs   = LX._vtxbuflist;
      int inumvb   = vbufs->size();
      for (int ivb = 0; ivb < inumvb; ivb++) {
        auto vertex_buf = (*vbufs)[ivb];
        gbi->DrawPrimitiveEML(*vertex_buf, EPRIM_TRIANGLES);
      }
    }
  } else { // sector based culling (WIP)

    // lod0 - inner sectors - draw all
    for (int sectID = 0; sectID < 8; sectID++) {
      auto& sector = _sector[sectID];
      auto L0      = sector._lod0;
      auto vbufs   = L0._vtxbuflist;
      int inumvb   = vbufs->size();
      for (int ivb = 0; ivb < inumvb; ivb++) {
        auto vertex_buf = (*vbufs)[ivb];
        gbi->DrawPrimitiveEML(*vertex_buf, EPRIM_TRIANGLES);
      }
    }

    // lodXouter - inner sectors - draw visible
    auto zn_xz  = znormal.GetXZ().Normal();
    float angle = 8.0 * ((PI * 0.5) + rect2pol_ang(zn_xz.x, zn_xz.y)) / (PI * 2.0);
    // printf( "znormal<%g %g> angle<%g>\n", zn_xz.x, zn_xz.y, angle );
    int basesector = int(floor(angle)) + 2;
    for (int soff = 6; soff < 10; soff++) {
      int sectID   = (basesector + soff) & 7;
      auto& sector = _sector[sectID];
      auto LX      = sector._lodX;
      auto vbufs   = LX._vtxbuflist;
      int inumvb   = vbufs->size();
      for (int ivb = 0; ivb < inumvb; ivb++) {
        auto vertex_buf = (*vbufs)[ivb];
        gbi->DrawPrimitiveEML(*vertex_buf, EPRIM_TRIANGLES);
      }
    }
    _terrainMaterial->end(*RCFD);
  }

  targ->PopMaterial();
}

///////////////////////////////////////////////////////////////////////////////

static void _RenderHeightfield(RenderContextInstData& rcid, Context* targ, const CallbackRenderable* pren) {
  pren->GetDrawableDataA().getShared<TerrainRenderImpl>()->render(rcid);
}

///////////////////////////////////////////////////////////////////////////////

void TerrainDrawableData::describeX(class_t* c) {
  c->memberProperty("Offset", &TerrainDrawableData::_visualOffset);
  c->memberProperty("FogColor", &TerrainDrawableData::_fogcolor);
  c->memberProperty("GrassColor", &TerrainDrawableData::_grass);
  c->memberProperty("SnowColor", &TerrainDrawableData::_snow);
  c->memberProperty("rock1Color", &TerrainDrawableData::_rock1);
  c->memberProperty("Rock2Color", &TerrainDrawableData::_rock2);
  ////////////////////////////////////////////////////////////////////////
  c->accessorProperty("HeightMap", &TerrainDrawableData::_readHmapPath, &TerrainDrawableData::_writeHmapPath)
      ->annotate<ConstString>("editor.class", "ged.factory.assetlist")
      ->annotate<ConstString>("editor.filetype", "png");
  ////////////////////////////////////////////////////////////////////////
  c->accessorProperty("SphericalEnvMap", &TerrainDrawableData::_readEnvMap, &TerrainDrawableData::_writeEnvMap)
      ->annotate<ConstString>("editor.class", "ged.factory.assetlist")
      ->annotate<ConstString>("editor.assettype", "lev2tex")
      ->annotate<ConstString>("editor.assetclass", "lev2tex");
  ////////////////////////////////////////////////////////////////////////
  c->floatProperty("TestXXX", float_range{-100, 100}, &TerrainDrawableData::_testxxx);
  c->floatProperty("GBlendYScale", float_range{-1, 1}, &TerrainDrawableData::_gblend_yscale);
  c->floatProperty("GBlendYBias", float_range{-5000, 5000}, &TerrainDrawableData::_gblend_ybias);
  c->floatProperty("GBlendStepLo", float_range{0, 1}, &TerrainDrawableData::_gblend_steplo);
  c->floatProperty("GBlendStepHi", float_range{0, 1}, &TerrainDrawableData::_gblend_stephi);
  ////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

hfdrawableinstptr_t TerrainDrawableData::createInstance() const {
  auto drw           = std::make_shared<TerrainDrawableInst>(*this);
  drw->_visualOffset = _visualOffset;
  return drw;
}

TerrainDrawableData::TerrainDrawableData()
    : _hfpath("none")
    , _testxxx(0) {
}
TerrainDrawableData::~TerrainDrawableData() {
}

///////////////////////////////////////////////////////////////////////////////
void TerrainDrawableData::_writeEnvMap(ork::rtti::ICastable* const& tex) {
  _sphericalenvmap = tex ? ork::rtti::autocast(tex) : nullptr;
}
void TerrainDrawableData::_readEnvMap(ork::rtti::ICastable*& tex) const {
  tex = _sphericalenvmap;
}
static int count = 0;
void TerrainDrawableData::_writeHmapPath(file::Path const& hmap) {
  _hfpath = hmap;
  printf("set _hfpath<%s> count<%d>\n", _hfpath.c_str(), count++);
  if (_hfpath == "none") {
  }
}
void TerrainDrawableData::_readHmapPath(file::Path& hmap) const {
  hmap = _hfpath;
}

///////////////////////////////////////////////////////////////////////////////

TerrainDrawableInst::TerrainDrawableInst(const TerrainDrawableData& data)
    : _data(data) {
}

file::Path TerrainDrawableInst::hfpath() const {
  return _data._hfpath;
}

///////////////////////////////////////////////////////////////////////////////

CallbackDrawable* TerrainDrawableInst::createCallbackDrawable() {

  auto impl    = _impl.makeShared<TerrainRenderImpl>(this);
  _rawdrawable = new CallbackDrawable(nullptr);
  _rawdrawable->SetRenderCallback(_RenderHeightfield);
  _rawdrawable->SetUserDataA(impl);
  _rawdrawable->SetSortKey(1000);
  return _rawdrawable;
}

///////////////////////////////////////////////////////////////////////////////

TerrainDrawableInst::~TerrainDrawableInst() {
  _rawdrawable->SetRenderCallback(nullptr);
  _rawdrawable->SetUserDataA(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2
///////////////////////////////////////////////////////////////////////////////
