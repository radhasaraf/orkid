////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/application/application.h>
#include <ork/file/chunkfile.h>
#include <ork/file/chunkfile.inl>
#include <ork/kernel/prop.h>
#include <ork/kernel/string/StringBlock.h>
#include <ork/kernel/string/string.h>
#include <ork/lev2/gfx/gfxenv.h>
#include <ork/lev2/gfx/gfxmaterial_test.h>
#include <ork/lev2/gfx/gfxmodel.h>
#include <ork/lev2/gfx/texman.h>
#include <ork/lev2/lev2_asset.h>
#include <ork/pch.h>
#include <ork/rtti/downcast.h>
#include <boost/filesystem.hpp>
#include <ork/kernel/datacache.h>
#include <ork/util/logger.h>
#include <ork/kernel/memcpy.inl>

namespace bfs = boost::filesystem;
namespace ork::meshutil {
datablock_ptr_t assimpToXgm(datablock_ptr_t inp_datablock);
}

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace lev2 {
static logchannel_ptr_t logchan_mioR = logger()->createChannel("gfxmodelIOREAD",fvec3(0.8,0.8,0.4));
static logchannel_ptr_t logchan_mioW = logger()->createChannel("gfxmodelIOWRITE",fvec3(0.8,0.7,0.4));
///////////////////////////////////////////////////////////////////////////////
bool SaveXGM(const AssetPath& Filename, const lev2::XgmModel* mdl) {
  //logchan_mioR->log("Writing Xgm<%p> to path<%s>d, (void*) mdl, Filename.c_str());
  auto datablock = writeXgmToDatablock(mdl);
  if (datablock) {
    ork::File outputfile(Filename, ork::EFM_WRITE);
    outputfile.Write(datablock->data(), datablock->length());
    return true;
  }
  return false;
}
///////////////////////////////////////////////////////////////////////////////

bool XgmModel::LoadUnManaged(XgmModel* mdl, const AssetPath& Filename, const asset::vars_t& vars) {
  bool rval        = false;
  auto ActualPath  = Filename.ToAbsolute();
  mdl->msModelName = AddPooledString(Filename.c_str());
  /////////////////////
  // merge in asset vars
  /////////////////////
  mdl->_varmap.mergeVars(vars);
  /////////////////////
  if (auto datablock = datablockFromFileAtPath(ActualPath)) {
    ///////////////////////////////////
    // annotate the datablock with some info
    //  that might be useful in loading the file
    ///////////////////////////////////
    auto actual_as_bfs = ActualPath.toBFS();
    auto base_dir      = actual_as_bfs.parent_path();
    OrkAssert(bfs::exists(actual_as_bfs));
    OrkAssert(bfs::is_regular_file(actual_as_bfs));
    OrkAssert(bfs::exists(base_dir));
    OrkAssert(bfs::is_directory(base_dir));
    datablock->_vars->makeValueForKey<std::string>("file-extension") = ActualPath.GetExtension().c_str();
    datablock->_vars->makeValueForKey<bfs::path>("base-directory")   = base_dir;
    /////////////////////
    // merge in asset vars
    /////////////////////
    datablock->_vars->mergeVars(vars);
    ///////////////////////////////////
    rval = _loaderSelect(mdl, datablock);
  }
  return rval;
}

////////////////////////////////////////////////////////////

bool XgmModel::_loaderSelect(XgmModel* mdl, datablock_ptr_t datablock) {
  DataBlockInputStream datablockstream(datablock);
  Char4 check_magic(datablockstream.getItem<uint32_t>());
  if (check_magic == Char4("chkf")) // its a chunkfile
    return _loadXGM(mdl, datablock);
  if (check_magic == Char4("glTF")) // its a glb (binary)
    return _loadAssimp(mdl, datablock);
  auto extension = datablock->_vars->typedValueForKey<std::string>("file-extension").value();
  if (extension == "gltf" or //
      extension == "dae" or  //
      extension == "fbx" or  //
      extension == "obj")
    return _loadAssimp(mdl, datablock);
  OrkAssert(false);
  return false;
}

////////////////////////////////////////////////////////////

bool XgmModel::_loadAssimp(XgmModel* mdl, datablock_ptr_t inp_datablock) {
  auto basehasher = DataBlock::createHasher();
  basehasher->accumulateString("assimp2xgm");

  auto str   = FormatString("version-x%05d",rand()%10000);
  basehasher->accumulateString(str); 
  inp_datablock->accumlateHash(basehasher);
  /////////////////////////////////////
  // include asset vars as hash mutator 
  //  because they may influence the loading mechanism 
  /////////////////////////////////////
  for( auto item : mdl->_varmap._themap ){
    const std::string& k = item.first;
    const rendervar_t& v = item.second;
    basehasher->accumulateString(k);
    if(auto as_str = v.tryAs<std::string>() ){
      basehasher->accumulateString(as_str.value());
      logchan_mioR->log( "LOADASSIMP VAR<%s> <%s>", k.c_str(), as_str.value().c_str());
    }
    else if(auto as_bool = v.tryAs<bool>() ){
      basehasher->accumulateItem<bool>(as_bool.value());
    }
    else if(auto as_dbl = v.tryAs<double>() ){
      basehasher->accumulateItem<double>(as_dbl.value());
    }
    else{
      OrkAssert(false);
    }

  }
  /////////////////////////////////////
  basehasher->finish();
  uint64_t hashkey   = basehasher->result();
  auto xgm_datablock = DataBlockCache::findDataBlock(hashkey);
  if (not xgm_datablock) {
    xgm_datablock = meshutil::assimpToXgm(inp_datablock);
    DataBlockCache::setDataBlock(hashkey, xgm_datablock);
  }
  return _loadXGM(mdl, xgm_datablock);
}
////////////////////////////////////////////////////////////

bool XgmModel::_loadXGM(XgmModel* mdl, datablock_ptr_t datablock) {
  constexpr int kVERSIONTAG = 0x01234567;
  bool rval                 = false;
  /////////////////////////////////////////////////////////////
  auto context = lev2::contextForCurrentThread();
  /////////////////////////////////////////////////////////////
  OrkHeapCheck();
  chunkfile::DefaultLoadAllocator allocator;
  chunkfile::Reader chunkreader(datablock, allocator);
  OrkHeapCheck();
  /////////////////////////////////////////////////////////////
  if (chunkreader.IsOk()) {
    chunkfile::InputStream* HeaderStream    = chunkreader.GetStream("header");
    chunkfile::InputStream* ModelDataStream = chunkreader.GetStream("modeldata");
    chunkfile::InputStream* EmbTexStream    = chunkreader.GetStream("embtexmap");

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    mdl->mbSkinned = false;
    /////////////////////////////////////////////////////////
    PropTypeString ptstring;
    /////////////////////////////////////////////////////////
    int inumjoints = 0;
    HeaderStream->GetItem(inumjoints);
    /////////////////////////////////////////////////////////
    // test for version tag
    /////////////////////////////////////////////////////////
    if (inumjoints == kVERSIONTAG) {
      int XGMVERSIONCODE = 0;
      HeaderStream->GetItem(XGMVERSIONCODE);
      HeaderStream->GetItem(inumjoints);
    }
    /////////////////////////////////////////////////////////
    if (inumjoints) {
      mdl->mSkeleton.resize(inumjoints);
      for (int ib = 0; ib < inumjoints; ib++) {
        int iskelindex = 0;
        int iparentindex = 0;
        int ijointname = 0;
        int ijointmatrix = 0;
        int iinvrestmatrix = 0;
        int inodematrix = 0;
        HeaderStream->GetItem(iskelindex);
        OrkAssert(ib == iskelindex);
        HeaderStream->GetItem(iparentindex);
        HeaderStream->GetItem(ijointname);
        HeaderStream->GetItem(inodematrix);
        HeaderStream->GetItem(ijointmatrix);
        HeaderStream->GetItem(iinvrestmatrix);
        const char* pjntname = chunkreader.GetString(ijointname);

        fmtx4 scalematrix;
        //scalematrix.compose(fvec3(0,0,0),fquat(),0.01f);

        fxstring<256> jnamp(pjntname);
        mdl->mSkeleton.AddJoint(iskelindex, iparentindex, jnamp.c_str());
        ptstring.set(chunkreader.GetString(inodematrix));
        mdl->mSkeleton.RefNodeMatrix(iskelindex) = scalematrix*PropType<fmtx4>::FromString(ptstring);
        ptstring.set(chunkreader.GetString(ijointmatrix));
        mdl->mSkeleton.RefJointMatrix(iskelindex) = scalematrix*PropType<fmtx4>::FromString(ptstring);
        ptstring.set(chunkreader.GetString(iinvrestmatrix));
        auto bind_matrix = scalematrix*PropType<fmtx4>::FromString(ptstring);
        mdl->mSkeleton._bindMatrices[iskelindex] = bind_matrix;
        mdl->mSkeleton._inverseBindMatrices[iskelindex] = bind_matrix.inverse();
      }
    }
    ///////////////////////////////////
    // read flattened bones
    ///////////////////////////////////
    int inumbones = 0;
    HeaderStream->GetItem(inumbones);
    for (int ib = 0; ib < inumbones; ib++) {
      int iib = 0;
      lev2::XgmBone Bone;
      HeaderStream->GetItem(iib);
      OrkAssert(iib == ib);
      HeaderStream->GetItem(Bone._parentIndex);
      HeaderStream->GetItem(Bone._childIndex);
      mdl->mSkeleton.addBone(Bone);
    }
    if (inumbones) {
      mdl->mSkeleton.miRootNode = (inumbones > 0) ? mdl->mSkeleton.bone(0)._parentIndex : -1;
    }
    // mdl->mSkeleton.dump();
    ///////////////////////////////////
    HeaderStream->GetItem(mdl->mBoundingCenter);
    HeaderStream->GetItem(mdl->mAABoundXYZ);
    HeaderStream->GetItem(mdl->mAABoundWHD);
    HeaderStream->GetItem(mdl->mBoundingRadius);
    // END HACK
    ///////////////////////////////////
    int inummeshes = 0, inummats = 0;
    HeaderStream->GetItem(mdl->miBonesPerCluster);
    HeaderStream->GetItem(inummeshes);
    HeaderStream->GetItem(inummats);
    ///////////////////////////////////
    mdl->mMeshes.reserve(inummeshes);
    ///////////////////////////////////
    // embedded textures
    ///////////////////////////////////
    auto& embtexmap = mdl->_varmap.makeValueForKey<embtexmap_t>("embtexmap");
    if (EmbTexStream) {
      size_t numembtex = 0;
      EmbTexStream->GetItem(numembtex);
      int itexname = 0;
      for (size_t i = 0; i < numembtex; i++) {
        EmbTexStream->GetItem(itexname);
        auto texname    = chunkreader.GetString(itexname);
        size_t datasize = 0;
        EmbTexStream->GetItem(datasize);
        auto texturedata = EmbTexStream->GetCurrent();
        auto texdatcopy  = malloc(datasize);
        memcpy_fast(texdatcopy, texturedata, datasize);
        EmbTexStream->advance(datasize);
        auto embtex         = new EmbeddedTexture;
        embtex->_srcdata    = texdatcopy;
        embtex->_srcdatalen = datasize;
        embtexmap[texname]  = embtex;
        // logchan_mioR->log("embtex<%zu:%s> datasiz<%zu> dataptr<%p>d, i, texname, datasize, texturedata);
      }
    }

    ///////////////////////////////////
    chunkfile::XgmMaterialReaderContext materialread_ctx(chunkreader);
    materialread_ctx._varmap->mergeVars(mdl->_varmap);
    materialread_ctx._varmap->makeValueForKey<Context*>("gfxtarget")    = context;
    materialread_ctx._inputStream = HeaderStream;

    ///////////////////////////////////
    bool use_normalviz = false;
    if( auto try_override = mdl->_varmap.typedValueForKey<std::string>("override.shader.gbuf") ){
      const auto& override_sh = try_override.value();
      if(override_sh=="normalviz"){
        use_normalviz = true;
      }
    }
    ///////////////////////////////////
    for (int imat = 0; imat < inummats; imat++) {
      int iimat = 0, imatname = 0, imatclass = 0;
      HeaderStream->GetItem(iimat);
      OrkAssert(iimat == imat);
      HeaderStream->GetItem(imatname);
      HeaderStream->GetItem(imatclass);
      const char* pmatname                = chunkreader.GetString(imatname);
      const char* pmatclassname           = chunkreader.GetString(imatclass);

      logchan_mioR->log( "pmatname<%s>", pmatname );
      logchan_mioR->log( "pmatclassname<%s>", pmatclassname );
      ork::object::ObjectClass* pmatclass = rtti::autocast(rtti::Class::FindClass(pmatclassname));

      static const int kdefaulttranssortpass = 100;

      ///////////////////////////////////////////////////////////
      // check xgm reader annotation
      ///////////////////////////////////////////////////////////

      auto anno = pmatclass->annotation("xgm.reader");
      if (auto as_reader = anno.tryAs<chunkfile::materialreader_t>()) {
        auto pmat = as_reader.value()(materialread_ctx);
        pmat->SetName(AddPooledString(pmatname));
        mdl->AddMaterial(pmat);
        pmat->gpuInit(context);
      }

      ///////////////////////////////////////////////////////////
      // material class not supported in XGM
      ///////////////////////////////////////////////////////////
      else {
        OrkAssert(false);
      }
    }
    ///////////////////////////////////
    for (int imesh = 0; imesh < inummeshes; imesh++) {
      XgmMesh* Mesh = new XgmMesh;

      int itestmeshindex    = -1;
      int itestmeshname     = -1;
      int imeshnummats      = -1;
      int imeshnumsubmeshes = -1;

      HeaderStream->GetItem(itestmeshindex);
      OrkAssert(itestmeshindex == imesh);

      HeaderStream->GetItem(itestmeshname);
      const char* MeshName  = chunkreader.GetString(itestmeshname);
      PoolString MeshNamePS = AddPooledString(MeshName);
      Mesh->SetMeshName(MeshNamePS);
      mdl->mMeshes.AddSorted(MeshNamePS, Mesh);

      HeaderStream->GetItem(imeshnumsubmeshes);

      Mesh->ReserveSubMeshes(imeshnumsubmeshes);

      for (int ics = 0; ics < imeshnumsubmeshes; ics++) {
        int itestclussetindex = -1, imatname = -1;
        HeaderStream->GetItem(itestclussetindex);
        OrkAssert(ics == itestclussetindex);

        XgmSubMesh* submesh = new XgmSubMesh;
        Mesh->AddSubMesh(submesh);
        XgmSubMesh& xgm_sub_mesh = *submesh;

        int numclusters = 0;
        HeaderStream->GetItem(numclusters);

        HeaderStream->GetItem(imatname);
        const char* matname = chunkreader.GetString(imatname);

        //////////////////////////////

        for (int imat = 0; imat < mdl->miNumMaterials; imat++) {
          auto pmat = mdl->GetMaterial(imat);
          if (strcmp(pmat->GetName().c_str(), matname) == 0) {
            xgm_sub_mesh._material = pmat;
          }
        }

        for (int ic = 0; ic < numclusters; ic++) {
          auto cluster = std::make_shared<XgmCluster>();
          xgm_sub_mesh._clusters.push_back(cluster);
          int iclusindex = -1;
          int inumbb     = -1;
          int ivboffset  = -1;
          int ivbnum     = -1;
          int ivbsize    = -1;
          fvec3 boxmin, boxmax;
          EVtxStreamFormat efmt;

          ////////////////////////////////////////////////////////////////////////
          HeaderStream->GetItem(iclusindex);
          OrkAssert(ic == iclusindex);
          int numprimgroups = 0;
          HeaderStream->GetItem(numprimgroups);
          HeaderStream->GetItem(inumbb);
          HeaderStream->GetItem<EVtxStreamFormat>(efmt);
          HeaderStream->GetItem(ivboffset);
          HeaderStream->GetItem(ivbnum);
          HeaderStream->GetItem(ivbsize);
          HeaderStream->GetItem(boxmin);
          HeaderStream->GetItem(boxmax);
          ////////////////////////////////////////////////////////////////////////
          cluster->mBoundingBox.SetMinMax(boxmin, boxmax);
          cluster->mBoundingSphere = Sphere(boxmin, boxmax);
          ////////////////////////////////////////////////////////////////////////
          // logchan_mioR->log( "XGMLOAD vbfmt<%s> efmt<%d>d, vbfmt, int(efmt) );
          ////////////////////////////////////////////////////////////////////////
          cluster->_vertexBuffer = VertexBufferBase::CreateVertexBuffer(efmt, ivbnum, true);
          void* pverts           = (void*)(ModelDataStream->GetDataAt(ivboffset));
          int ivblen             = ivbnum * ivbsize;
          // logchan_mioR->log("ReadVB NumVerts<%d> VtxSize<%d>d, ivbnum, pvb->GetVtxSize());
          void* poutverts = context->GBI()->LockVB(*cluster->_vertexBuffer.get(), 0, ivbnum); // ivblen );
          {
            memcpy_fast(poutverts, pverts, ivblen);
            cluster->_vertexBuffer->SetNumVertices(ivbnum);
            if (efmt == EVtxStreamFormat::V12N12B12T8I4W4) {
              auto pv = (const SVtxV12N12B12T8I4W4*)pverts;
              for (int iv = 0; iv < ivbnum; iv++) {
                auto& v = pv[iv];
                auto& p = v.mPosition;
                auto& n = v.mNormal;
                OrkAssert(n.length() > 0.95);
                // logchan_mioR->log( " iv<%d> pos<%f %f %f> bi<%08x> bw<%08x>d, iv, p.x, p.y, p.z, v.mBoneIndices,
                // v.mBoneWeights );
              }
            }
          }
          context->GBI()->UnLockVB(*cluster->_vertexBuffer.get());
          ////////////////////////////////////////////////////////////////////////
          for (int32_t ipg = 0; ipg < numprimgroups; ipg++) {
            auto newprimgroup = std::make_shared<XgmPrimGroup>();
            cluster->_primgroups.push_back(newprimgroup);
            int32_t ipgindex    = -1;
            int32_t ipgprimtype = -1;
            HeaderStream->GetItem<int32_t>(ipgindex);
            OrkAssert(ipgindex == ipg);

            HeaderStream->GetItem<PrimitiveType>(newprimgroup->mePrimType);
            HeaderStream->GetItem<int32_t>(newprimgroup->miNumIndices);

            int32_t idxdataoffset = -1;
            HeaderStream->GetItem<int32_t>(idxdataoffset);

            U16* pidx = (U16*)ModelDataStream->GetDataAt(idxdataoffset);

            auto pidxbuf = new StaticIndexBuffer<U16>(newprimgroup->miNumIndices);

            void* poutidx = (void*)context->GBI()->LockIB(*pidxbuf);
            {
              // TODO: Make 16-bit indices a policy
              if (newprimgroup->miNumIndices > 0xFFFF)
                orkerrorlog(
                    "WARNING: <%s> Wii cannot have num indices larger than 65535: MeshName=%s, MatName=%s",
                    mdl->msModelName.c_str(),
                    MeshName,
                    matname);

              memcpy_fast(poutidx, pidx, newprimgroup->miNumIndices * sizeof(U16));
            }
            context->GBI()->UnLockIB(*pidxbuf);

            newprimgroup->mpIndices = pidxbuf;
          }
          ////////////////////////////////////////////////////////////////////////
          cluster->mJoints.resize(inumbb);
          cluster->mJointSkelIndices.resize(inumbb);
          for (int ib = 0; ib < inumbb; ib++) {
            int ibindingindex = -1;
            int ibindingname  = -1;

            HeaderStream->GetItem(ibindingindex);
            HeaderStream->GetItem(ibindingname);

            const char* jointname = chunkreader.GetString(ibindingname);
            auto itfind = mdl->mSkeleton.mmJointNameMap.find(jointname);

            if(itfind == mdl->mSkeleton.mmJointNameMap.end()){
              logerrchannel()->log( "\n\ncannot find joint<%s> in:", jointname );
              for( auto it: mdl->mSkeleton.mmJointNameMap ){
                logerrchannel()->log( "  %s", it.first.c_str() );
              }
              OrkAssert(false);
            }
            int iskelindex                 = (*itfind).second;
            cluster->mJoints[ib]           = jointname;
            cluster->mJointSkelIndices[ib] = iskelindex;
          }

          mdl->mbSkinned |= (inumbb > 0);

          //logchan_mioR->log("mdl<%p> mbSkinned<%d>d, (void*) mdl, int(mdl->mbSkinned));
          ////////////////////////////////////////////////////////////////////////
        }
      }
    }
    rval = true;
  } // if( chunkreader.IsOk() )
  else {
    OrkAssert(false);
  }
  // rval->mSkeleton.dump();
  // mdl->dump();
  OrkHeapCheck();
  return rval;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

datablock_ptr_t writeXgmToDatablock(const lev2::XgmModel* mdl) {
  datablock_ptr_t out_datablock = std::make_shared<DataBlock>();

  lev2::ContextDummy DummyTarget;

  ///////////////////////////////////
  chunkfile::Writer chunkwriter("xgm");

  ///////////////////////////////////
  chunkfile::OutputStream* HeaderStream    = chunkwriter.AddStream("header");
  chunkfile::OutputStream* ModelDataStream = chunkwriter.AddStream("modeldata");

  ///////////////////////////////////
  // write out new VERSION code
  int32_t iVERSIONTAG = 0x01234567;
  int32_t iVERSION    = 1;
  HeaderStream->AddItem(iVERSIONTAG);
  HeaderStream->AddItem(iVERSION);
  logchan_mioW->log("WriteXgm: VERSION<%d>", iVERSION);

  ///////////////////////////////////
  // write out joints
  ///////////////////////////////////

  const lev2::XgmSkeleton& skel = mdl->skeleton();

  int32_t inumjoints = skel.numJoints();

  HeaderStream->AddItem(inumjoints);

  int32_t istring;
  logchan_mioW->log("WriteXgm: numjoints<%d>", inumjoints);

  for (int32_t ib = 0; ib < inumjoints; ib++) {
    const std::string& JointName = skel.GetJointName(ib);

    int32_t JointParentIndex   = skel.GetJointParent(ib);
    const fmtx4& bind_matrix = skel._bindMatrices[ib];
    const fmtx4& JointMatrix   = skel.RefJointMatrix(ib);
    const fmtx4& NodeMatrix    = skel.RefNodeMatrix(ib);

    HeaderStream->AddItem(ib);
    HeaderStream->AddItem(JointParentIndex);
    istring = chunkwriter.stringIndex(JointName.c_str());
    HeaderStream->AddItem(istring);

    PropTypeString tstr;
    PropType<fmtx4>::ToString(NodeMatrix, tstr);
    istring = chunkwriter.stringIndex(tstr.c_str());
    HeaderStream->AddItem(istring);

    PropType<fmtx4>::ToString(JointMatrix, tstr);
    istring = chunkwriter.stringIndex(tstr.c_str());
    HeaderStream->AddItem(istring);

    PropType<fmtx4>::ToString(bind_matrix, tstr);
    istring = chunkwriter.stringIndex(tstr.c_str());
    HeaderStream->AddItem(istring);
  }

  ///////////////////////////////////
  // write out flattened bones
  ///////////////////////////////////

  int32_t inumbones = skel.numBones();

  HeaderStream->AddItem(inumbones);

  logchan_mioW->log("WriteXgm: numbones<%d>", inumbones);
  for (int32_t ib = 0; ib < inumbones; ib++) {
    const lev2::XgmBone& Bone = skel.bone(ib);

    HeaderStream->AddItem(ib);
    HeaderStream->AddItem(Bone._parentIndex);
    HeaderStream->AddItem(Bone._childIndex);
  }

  ///////////////////////////////////
  // basic model data
  ///////////////////////////////////

  int32_t inummeshes = mdl->numMeshes();
  int32_t inummats   = mdl->GetNumMaterials();

  logchan_mioW->log("WriteXgm: nummeshes<%d>", inummeshes);
  logchan_mioW->log("WriteXgm: nummtls<%d>", inummats);

  const fvec3& bc    = mdl->boundingCenter();
  float br           = mdl->GetBoundingRadius();
  const fvec3& bbxyz = mdl->GetBoundingAA_XYZ();
  const fvec3& bbwhd = mdl->boundingAA_WHD();

  HeaderStream->AddItem(bc.x);
  HeaderStream->AddItem(bc.y);
  HeaderStream->AddItem(bc.z);
  HeaderStream->AddItem(bbxyz.x);
  HeaderStream->AddItem(bbxyz.y);
  HeaderStream->AddItem(bbxyz.z);
  HeaderStream->AddItem(bbwhd.x);
  HeaderStream->AddItem(bbwhd.y);
  HeaderStream->AddItem(bbwhd.z);
  HeaderStream->AddItem(br);

  HeaderStream->AddItem(mdl->GetBonesPerCluster());
  HeaderStream->AddItem(inummeshes);
  HeaderStream->AddItem(inummats);

  std::set<std::string> ParameterIgnoreSet;
  ParameterIgnoreSet.insert("binMembership");
  ParameterIgnoreSet.insert("colorSource");
  ParameterIgnoreSet.insert("texCoordSource");
  ParameterIgnoreSet.insert("uniformParameters");
  ParameterIgnoreSet.insert("varyingParameters");

  ///////////////////////////////////////////////////////////////////////////////////////////
  // embedded textures chunk
  ///////////////////////////////////////////////////////////////////////////////////////////

  if (auto as_embtexmap = mdl->_varmap.typedValueForKey<embtexmap_t>("embtexmap")) {
    auto& embtexmap    = as_embtexmap.value();
    auto textureStream = chunkwriter.AddStream("embtexmap");
    textureStream->AddItem<size_t>(embtexmap.size());
    for (auto item : embtexmap) {
      std::string texname   = item.first;
      EmbeddedTexture* ptex = item.second;
      int istring           = chunkwriter.stringIndex(texname.c_str());
      textureStream->AddItem<int>(istring);
      auto ddsblock    = ptex->_ddsdestdatablock;
      size_t blocksize = ddsblock->length();
      textureStream->AddItem<size_t>(blocksize);
      auto data = (const void*)ddsblock->data();
      textureStream->AddData(data, blocksize);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////
  // materials
  ///////////////////////////////////////////////////////////////////////////////////////////

  for (int32_t imat = 0; imat < inummats; imat++) {
    auto pmat        = mdl->GetMaterial(imat);
    auto matclass    = pmat->GetClass();
    auto& writeranno = matclass->annotation("xgm.writer");

    HeaderStream->AddItem(imat);
    istring = chunkwriter.stringIndex(pmat->GetName().c_str());
    HeaderStream->AddItem(istring);

    rtti::Class* pclass         = pmat->GetClass();
    const PoolString& classname = pclass->Name();
    const char* pclassname      = classname.c_str();

    logchan_mioW->log("WriteXgm: material<%d> class<%s> name<%s>", imat, pclassname, pmat->GetName().c_str());
    istring = chunkwriter.stringIndex(classname.c_str());
    HeaderStream->AddItem(istring);

    logchan_mioW->log("Material Name<%s> Class<%s>", pmat->GetName().c_str(), classname.c_str());

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // new style material writer
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    if (auto as_writer = writeranno.tryAs<chunkfile::materialwriter_t>()) {
      chunkfile::XgmMaterialWriterContext mtlwctx(chunkwriter);
      mtlwctx._material     = pmat;
      mtlwctx._outputStream = HeaderStream;
      auto& writer          = as_writer.value();
      writer(mtlwctx);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    // material class not supported for XGM
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    else {
      OrkAssert(false);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////
  // meshes
  ///////////////////////////////////////////////////////////////////////////////////////////

  for (int32_t imesh = 0; imesh < inummeshes; imesh++) {
    const lev2::XgmMesh& Mesh = *mdl->mesh(imesh);

    int32_t inumsubmeshes = Mesh.numSubMeshes();

    HeaderStream->AddItem(imesh);
    istring = chunkwriter.stringIndex(Mesh.meshName().c_str());
    HeaderStream->AddItem(istring);
    HeaderStream->AddItem(inumsubmeshes);

    logchan_mioW->log("WriteXgm: mesh<%d:%s> numsubmeshes<%d>", imesh, Mesh.meshName().c_str(), inumsubmeshes);
    for (int32_t ics = 0; ics < inumsubmeshes; ics++) {
      const lev2::XgmSubMesh& xgm_sub_mesh = *Mesh.subMesh(ics);
      auto pmat                            = xgm_sub_mesh.GetMaterial();

      int32_t inumclus = xgm_sub_mesh.GetNumClusters();

      int32_t inumenabledclus = 0;

      for (int ic = 0; ic < inumclus; ic++) {
        auto cluster = xgm_sub_mesh.cluster(ic);
        auto VB      = cluster->_vertexBuffer;

        if (!VB)
          return nullptr;

        if (VB->GetNumVertices() > 0) {
          inumenabledclus++;
        } else {
          logchan_mioW->log("WARNING: material<%s> cluster<%d> has a zero length vertex buffer, skipping", pmat->GetName().c_str(), ic);
        }
      }

      HeaderStream->AddItem(ics);
      HeaderStream->AddItem(inumenabledclus);

      logchan_mioW->log("WriteXgm:  submesh<%d> numenaclus<%d>", ics, inumenabledclus);
      ////////////////////////////////////////////////////////////
      istring = chunkwriter.stringIndex(pmat ? pmat->GetName().c_str() : "None");
      HeaderStream->AddItem(istring);
      ////////////////////////////////////////////////////////////
      for (int32_t ic = 0; ic < inumclus; ic++) {
        auto cluster              = xgm_sub_mesh.cluster(ic);
        auto VB                   = cluster->_vertexBuffer;
        const Sphere& clus_sphere = cluster->mBoundingSphere;
        const AABox& clus_box     = cluster->mBoundingBox;

        if (VB->GetNumVertices() == 0)
          continue;

        int32_t inumpg = cluster->numPrimGroups();
        int32_t inumjb = (int)cluster->GetNumJointBindings();

        logchan_mioW->log("VB<%p> NumVerts<%d>", (void*) VB.get(), VB->GetNumVertices());
        logchan_mioW->log("clus<%d> numjb<%d>", ic, inumjb);

        int32_t ivbufoffset = ModelDataStream->GetSize();
        const u8* VBdata    = (const u8*)DummyTarget.GBI()->LockVB(*VB);
        OrkAssert(VBdata != 0);
        {

          int VBlen = VB->GetNumVertices() * VB->GetVtxSize();

          logchan_mioW->log("WriteVB VB<%p> NumVerts<%d> VtxSize<%d>", (void*) VB.get(), VB->GetNumVertices(), VB->GetVtxSize());

          HeaderStream->AddItem(ic);
          HeaderStream->AddItem(inumpg);
          HeaderStream->AddItem(inumjb);
          HeaderStream->AddItem<lev2::EVtxStreamFormat>(VB->GetStreamFormat());
          HeaderStream->AddItem(ivbufoffset);
          HeaderStream->AddItem(VB->GetNumVertices());
          HeaderStream->AddItem(VB->GetVtxSize());

          HeaderStream->AddItem(clus_box.Min());
          HeaderStream->AddItem(clus_box.Max());

          // VBNC->EndianSwap();

          ModelDataStream->Write(VBdata, VBlen);
        }
        DummyTarget.GBI()->UnLockVB(*VB);

        for (int32_t ipg = 0; ipg < inumpg; ipg++) {
          auto PG = cluster->primgroup(ipg);

          int32_t inumidx = PG->GetNumIndices();

          logchan_mioW->log("WritePG<%d> NumIndices<%d>", ipg, inumidx);

          HeaderStream->AddItem(ipg);
          HeaderStream->AddItem<PrimitiveType>(PG->GetPrimType());
          HeaderStream->AddItem<int32_t>(inumidx);
          HeaderStream->AddItem<int32_t>(ModelDataStream->GetSize());

          //////////////////////////////////////////////////
          U16* pidx = (U16*)DummyTarget.GBI()->LockIB(*PG->GetIndexBuffer()); //->GetDataPointer();
          OrkAssert(pidx != 0);
          for (int32_t ii = 0; ii < inumidx; ii++) {
            int32_t iv = int32_t(pidx[ii]);
            if (iv >= VB->GetNumVertices()) {
              logchan_mioW->log("index id<%d> val<%d> is > vertex count<%d>", ii, iv, VB->GetNumVertices());
            }
            OrkAssert(iv < VB->GetNumVertices());

            // swapbytes_dynamic<U16>( pidx[ii] );
          }
          DummyTarget.GBI()->UnLockIB(*PG->GetIndexBuffer());
          //////////////////////////////////////////////////

          ModelDataStream->Write((const unsigned char*)pidx, inumidx * sizeof(U16));
        }
        // write cluster bindings
        for (int32_t ij = 0; ij < inumjb; ij++) {
          const std::string& bound = cluster->GetJointBinding(ij);
          OrkAssert(bound!="");
          HeaderStream->AddItem(ij);
          istring = chunkwriter.stringIndex(bound.c_str());
          logchan_mioW->log( "bound<%s> istring<%d>", bound.c_str(), istring );
          HeaderStream->AddItem(istring);
        }
      }
    }
  }
  chunkwriter.writeToDataBlock(out_datablock);
  //OrkAssert(false);
  return out_datablock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
}} // namespace ork::lev2
