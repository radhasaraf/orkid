///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2020, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

#include "assimp_util.inl"
#include<ork/util/logger.h>

namespace bfs = boost::filesystem;
///////////////////////////////////////////////////////////////////////////////
namespace ork::meshutil {
static logchannel_ptr_t logchan_meshutilassimp = logger()->createChannel("meshutil.assimp",fvec3(1,.9,.9));
///////////////////////////////////////////////////////////////////////////////
typedef std::set<std::string> bonemarkset_t;
///////////////////////////////////////////////////////////////////////////////
void Mesh::readFromAssimp(const file::Path& BasePath) {

  ork::file::Path GlbPath = BasePath;
  auto base_dir           = BasePath.toBFS().parent_path();
  OrkAssert(bfs::exists(GlbPath.toBFS()));
  OrkAssert(bfs::is_regular_file(GlbPath.toBFS()));
  OrkAssert(bfs::exists(base_dir));
  OrkAssert(bfs::is_directory(base_dir));
  auto dblock                                                  = datablockFromFileAtPath(GlbPath);
  dblock->_vars->makeValueForKey<std::string>("file-extension") = GlbPath.getExtension().c_str();
  dblock->_vars->makeValueForKey<bfs::path>("base-directory")   = base_dir;
  //logchan_meshutilassimp->log("BEGIN: importing<%s> via Assimp\n", GlbPath.c_str());
  readFromAssimp(dblock);
}
///////////////////////////////////////////////////////////////////////////////
void Mesh::readFromAssimp(datablock_ptr_t datablock) {
  auto& extension = datablock->_vars->typedValueForKey<std::string>("file-extension").value();
  //logchan_meshutilassimp->log("BEGIN: importing scene from datablock length<%zu> extension<%s>\n", datablock->length(), extension.c_str());
  auto scene = aiImportFileFromMemory((const char*)datablock->data(), datablock->length(), assimpImportFlags(), extension.c_str());
  //logchan_meshutilassimp->log("END: importing scene<%p>\n", (void*) scene);
  if (scene) {
    auto& embtexmap = _varmap->makeValueForKey<lev2::embtexmap_t>("embtexmap");
    aiVector3D scene_min, scene_max, scene_center;
    aiMatrix4x4 identity;
    aiIdentityMatrix4(&identity);
    scene_min.x = scene_min.y = scene_min.z = 1e10f;
    scene_max.x = scene_max.y = scene_max.z = -1e10f;
    get_bounding_box_for_node(scene, scene->mRootNode, scene_min, scene_max, identity);

    scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
    scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
    scene_center.z = (scene_min.z + scene_max.z) / 2.0f;

    _vertexExtents.BeginGrow();
    _skeletonExtents.BeginGrow();

    //////////////////////////////////////////////
    // parse embedded textures
    //////////////////////////////////////////////

    logchan_meshutilassimp->log("NumTex<%d>", scene->mNumTextures);

    for (int i = 0; i < scene->mNumTextures; i++) {
      auto texture        = scene->mTextures[i];
      std::string fmt     = (const char*)texture->achFormatHint;
      std::string texname = (const char*)texture->mFilename.data;
      bool embedded       = false;

      const void* dataptr = nullptr;
      size_t datalen      = 0;

      auto embtex = new lev2::EmbeddedTexture;

      embtex->_varmap["src.filename"].set<std::string>(texname);
      embtex->_varmap["src.format"].set<std::string>(texname);

      embtex->_format = fmt;



      embedded            = true;
      embtex->_srcdatalen = texture->mWidth;
      embtex->_srcdata    = (const void*)texture->pcData;
      embtex->_origname   = texname;
      texname             = FormatString("*%d", i);
      embtex->_name       = texname;

      if (fmt == "png" or fmt == "jpg") {
        embtexmap[texname] = embtex;
      } else if (fmt == "rgba8888" or fmt == "argb8888") {
        int w                 = texture->mWidth;
        int h                 = texture->mHeight;
        const aiTexel* texels = texture->pcData;
      } else {
        assert(false);
      }

      logchan_meshutilassimp->log("embtex: name<%s> fmt<%s> texpath<%s> texlen<%d>", //
                                  embtex->_name.c_str(), //
                                  fmt.c_str(), //
                                  texname.c_str(), //
                                  int(texture->mWidth));

      //logchan_meshutilassimp->log("embtex: texpath<%s>\n", texname.c_str());

    }

    //////////////////////////////////////////////
    // parse materials
    //////////////////////////////////////////////

    auto find_texture = [&](const std::string texname, lev2::ETextureUsage usage) -> lev2::EmbeddedTexture* {
      lev2::EmbeddedTexture* rval = nullptr;
      auto it                     = embtexmap.find(texname);
      if (it != embtexmap.end()) {
        rval = it->second;
        if (rval->_compressionPending) {
          rval->_usage = usage;
          rval->fetchDDSdata();
        }
      } else {
        // find by path
        auto base_dir = datablock->_vars->typedValueForKey<bfs::path>("base-directory").value();
        logchan_meshutilassimp->log("findtex: base_dir<%s> texname<%s>", base_dir.c_str(), texname.c_str());
        auto tex_path = base_dir / texname;
        auto tex_ext  = std::string(tex_path.extension().c_str());
        logchan_meshutilassimp->log("findtex: findtex: texpath<%s> tex_ext<%s>", tex_path.c_str(), tex_ext.c_str());
        if (boost::filesystem::exists(tex_path) and boost::filesystem::is_regular_file(tex_path)) {
          ork::file::Path as_ork_path;
          as_ork_path.fromBFS(tex_path);
          ork::File texfile;
          texfile.OpenFile(as_ork_path, ork::EFM_READ);
          size_t length = 0;
          texfile.GetLength(length);
          //logchan_meshutilassimp->log("texlen<%zu>\n", length);

          if (tex_ext == ".jpg" or tex_ext == ".jpeg" or tex_ext == ".png" or tex_ext == ".tga") {

            auto embtex     = new ork::lev2::EmbeddedTexture;
            embtex->_format = tex_ext.substr(1);
            embtex->_usage  = usage;

            void* dataptr = malloc(length);
            texfile.Read(dataptr, length);
            texfile.Close();

            embtex->_srcdatalen = length;
            embtex->_srcdata    = (const void*)dataptr;
            std::string texid   = FormatString("*%d", int(embtexmap.size()));
            embtex->_name       = texname;

            embtex->fetchDDSdata();
            embtexmap[texname] = embtex;
            rval               = embtex;
          } else {
            logerrchannel()->log("findtex: file extension not supported <%s> texname<%s>", tex_path.c_str(), texname.c_str());
          }
        } // if (boost::filesystem::exists
        else{
          logerrchannel()->log("findtex: file not found<%s> texname<%s>", tex_path.c_str(), texname.c_str());
        }
      }   // else

      ////////////////////////////////
      // catchall
      ////////////////////////////////

      if(rval==nullptr){
          logerrchannel()->log("findtex: texname<%s> substituting", texname.c_str());

          std::string texid   = FormatString("*%d", int(embtexmap.size()));

          auto embtex     = new ork::lev2::EmbeddedTexture;
          embtex->_name       = texname;
          embtex->_format = "bin.nodata";
          embtex->_usage  = usage;
          embtexmap[texname] = embtex;
          rval               = embtex;

          embtex->fetchDDSdata();

      }


      return rval;
    };

    logchan_meshutilassimp->log("/////////////////////////////////////////////////////////////////\n");

    //////////////////////////////////////////////

    gltfmaterialmap_t materialmap;

    for (int i = 0; i < scene->mNumMaterials; i++) {
      auto material = scene->mMaterials[i];

      auto outmtl    = new GltfMaterial;
      materialmap[i] = outmtl;

      std::string material_name;

      aiColor4D color;
      aiString string;
      float f;
      if (AI_SUCCESS == aiGetMaterialString(material, AI_MATKEY_NAME, &string)) {
        material_name = (const char*)string.data;
        outmtl->_name = material_name;
        logchan_meshutilassimp->log("//////////////////////////////");
        logchan_meshutilassimp->log("has name<%s>", material_name.c_str());
      }
      if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color)) {
        outmtl->_baseColor = fvec4(color.r, color.g, color.b, color.a);
        logchan_meshutilassimp->log("has_uniform_diffuse<%f %f %f %f>", color.r, color.g, color.b, color.a);
      }
      if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &color)) {
        logchan_meshutilassimp->log("has_uniform_specular<%f %f %f %f>", color.r, color.g, color.b, color.a);
      }
      if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &color)) {
        logchan_meshutilassimp->log("has_uniform_ambient");
      }
      if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &color)) {
        logchan_meshutilassimp->log("has_uniform_emissive");
      }
      if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &f)) {
        outmtl->_metallicFactor = f;
        logchan_meshutilassimp->log("has_pbr_MetallicFactor<%g>", f);
      }
      if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &f)) {
        outmtl->_roughnessFactor = f;
        logchan_meshutilassimp->log("has_pbr_RoughnessFactor<%g>", f);
      }
      if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &string, NULL, NULL, NULL, NULL, NULL)) {
        outmtl->_colormap = (const char*)string.data;
        auto tex          = find_texture(outmtl->_colormap, lev2::ETEXUSAGE_COLOR);
        logchan_meshutilassimp->log("has_pbr_colormap<%s> tex<%p>", outmtl->_colormap.c_str(), (void*) tex);
      }
      if (AI_SUCCESS == aiGetMaterialTexture(material, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &string)) {
        outmtl->_metallicAndRoughnessMap = (const char*)string.data;
        auto tex                         = find_texture(outmtl->_metallicAndRoughnessMap, lev2::ETEXUSAGE_COLOR);
        logchan_meshutilassimp->log("has_pbr_MetallicAndRoughnessMap<%s> tex<%p>", outmtl->_metallicAndRoughnessMap.c_str(), (void*) tex);
      }

      if (AI_SUCCESS == material->GetTexture(aiTextureType_NORMALS, 0, &string, NULL, NULL, NULL, NULL, NULL)) {
        outmtl->_normalmap = (const char*)string.data;
        auto tex           = find_texture(outmtl->_normalmap, lev2::ETEXUSAGE_NORMAL);
        logchan_meshutilassimp->log("has_pbr_normalmap<%s> tex<%p>", outmtl->_normalmap.c_str(), (void*) tex);
      }
      if (AI_SUCCESS == material->GetTexture(aiTextureType_AMBIENT, 0, &string, NULL, NULL, NULL, NULL, NULL)) {
        outmtl->_amboccmap = (const char*)string.data;
        auto tex           = find_texture(outmtl->_amboccmap, lev2::ETEXUSAGE_GREYSCALE);
        logchan_meshutilassimp->log("has_pbr_amboccmap<%s> tex<%p>", outmtl->_amboccmap.c_str(), (void*) tex);
      }
      if (AI_SUCCESS == material->GetTexture(aiTextureType_EMISSIVE, 0, &string, NULL, NULL, NULL, NULL, NULL)) {
        outmtl->_emissivemap = (const char*)string.data;
        auto tex             = find_texture(outmtl->_emissivemap, lev2::ETEXUSAGE_GREYSCALE);
        logchan_meshutilassimp->log("has_pbr_emissivemap<%s> tex<%p>", outmtl->_emissivemap.c_str(), (void*) tex);
      }
      //logchan_meshutilassimp->log("");
    }

    logchan_meshutilassimp->log("/////////////////////////////////////////////////////////////////\n");

    //////////////////////////////////////////////

    auto& bonemarkset = (*_varmap)["bonemarkset"].make<bonemarkset_t>();

    std::queue<aiNode*> nodestack;

    auto parsedskel = parseSkeleton(scene);

    (*_varmap)["parsedskel"].make<parsedskeletonptr_t>(parsedskel);
    bool is_skinned    = parsedskel->_isSkinned;
    auto& xgmskelnodes = parsedskel->_xgmskelmap;

    //////////////////////////////////////////////
    // count, visit dagnodes
    //////////////////////////////////////////////

    //logchan_meshutilassimp->log("/////////////////////////////////////////////////////////////////\n");

    //////////////////////////////////////////////
    // visit meshes, marking dagnodes as bones and fetching joint matrices
    //////////////////////////////////////////////

    struct XgmAssimpVertexWeightItem {
      std::string _bonename;
      float _weight = 0.0f;
    };
    struct XgmAssimpVertexWeights {
      std::vector<XgmAssimpVertexWeightItem> _items;
    };
    std::map<int, XgmAssimpVertexWeights*> assimpweightlut;

    nodestack = std::queue<aiNode*>();
    nodestack.push(scene->mRootNode);
    while (not nodestack.empty()) {
      auto n = nodestack.front();
      nodestack.pop();
      for (int m = 0; m < n->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[n->mMeshes[m]];
        for (int b = 0; b < mesh->mNumBones; b++) {
          auto bone     = mesh->mBones[b];
          auto bonename = remapSkelName(bone->mName.data);
          auto itb      = xgmskelnodes.find(bonename);
          bonemarkset.insert(bonename);
          /////////////////////////////////////////////
          // yuk -- assimp is not like gltf, or collada...
          // https://github.com/assimp/assimp/blob/master/code/glTF2/glTF2Importer.cpp#L904
          /////////////////////////////////////////////
          int numvertsaffected = bone->mNumWeights;
          ////////////////////////////////////////////
          if (itb != xgmskelnodes.end()) {
            auto xgmnode = itb->second;
            //////////////////////////////////
            // mark skel node as actual mesh referenced bone
            //////////////////////////////////
            if (false == xgmnode->_varmap["visited_weights"].isA<bool>()) {
              xgmnode->_varmap["visited_weights"].set<bool>(true);
              int index = xgmnode->miSkelIndex;
              /////////////////////////////
              // xgmjointmatrix.dump(bonename.c_str());
              // remember effected verts
              /////////////////////////////
              xgmnode->_numBoundVertices += numvertsaffected;
              for (int v = 0; v < numvertsaffected; v++) {
                const aiVertexWeight& vw                   = bone->mWeights[v];
                int vertexID                               = vw.mVertexId;
                float weight                               = vw.mWeight;
                XgmAssimpVertexWeights* weights_for_vertex = nullptr;
                auto itw                                   = assimpweightlut.find(vertexID);
                if (itw != assimpweightlut.end()) {
                  weights_for_vertex = itw->second;
                } else {
                  weights_for_vertex        = new XgmAssimpVertexWeights;
                  assimpweightlut[vertexID] = weights_for_vertex;
                }
                auto xgmw = XgmAssimpVertexWeightItem{bonename, weight};
                weights_for_vertex->_items.push_back(xgmw);
              }
            }
          }
        }
      }
      for (int i = 0; i < n->mNumChildren; ++i) {
        nodestack.push(n->mChildren[i]);
      }
    } // while (not nodestack.empty()) {

    nodestack = std::queue<aiNode*>();
    nodestack.push(scene->mRootNode);

    //////////////////////////////////////////////

    // std::deque<fmtx4> ork_mtxstack;
    // ork_mtxstack.push_front(convertMatrix44(scene->mRootNode->mTransformation));

    auto it_root_skelnode                 = xgmskelnodes.find(remapSkelName(scene->mRootNode->mName.data));
    ork::lev2::xgmskelnode_ptr_t root_skelnode = (it_root_skelnode != xgmskelnodes.end()) ? it_root_skelnode->second : nullptr;

    //////////////////////////////////////////////
    // parse nodes
    //////////////////////////////////////////////

    //logchan_meshutilassimp->log("parsing nodes for meshdata\n");

    while (not nodestack.empty()) {

      auto n = nodestack.front();

      // aiMatrix4x4 mtx = n->mTransformation;

      nodestack.pop();

      auto nren = remapSkelName(n->mName.data);

      auto it_nod_skelnode                 = xgmskelnodes.find(nren);
      ork::lev2::xgmskelnode_ptr_t nod_skelnode = (it_nod_skelnode != xgmskelnodes.end()) ? it_nod_skelnode->second : nullptr;

      //logchan_meshutilassimp->log("xgmnode<%p:%s>\n", (void*) nod_skelnode, nod_skelnode->_name.c_str());
      // auto ppar_skelnode = nod_skelnode->_parent;
      std::string name = nod_skelnode->_name;
      auto nodematrix  = nod_skelnode->_nodeMatrix;
      fmtx4 local      = nod_skelnode->_jointMatrix;
      fmtx4 invbind    = nod_skelnode->_bindMatrixInverse;
      fmtx4 bind       = nod_skelnode->_bindMatrix;

      fvec3 trans = bind.translation();

      _skeletonExtents.Grow(trans);

      //////////////////////////////////////////////
      // for rigid meshes, preapply transforms
      //  TODO : make optional
      //////////////////////////////////////////////

      fmtx4 ork_model_mtx;
      fmtx3 ork_normal_mtx;
      if (false == is_skinned) {
        //deco::logchan_meshutilassimp->log(fvec3(1, 0, 0), "/////////////////////////////\n");
        std::deque<aiNode*> nodehier;
        bool done = false;
        auto walk = n;
        while (not done) {
          nodehier.push_back(n);
          fmtx4 test = convertMatrix44(walk->mTransformation);
          //auto test_str = test.dump4x3cn();
          //logchan_meshutilassimp->log("NODE<%s> : %s\n", walk->mName.data, test_str.c_str() );
          walk = walk->mParent;
          done = (walk == nullptr);
        }
        //for (auto item : nodehier) {
          //ork_model_mtx = convertMatrix44(item->mTransformation) * ork_model_mtx;
        //}

        ork_model_mtx = convertMatrix44(n->mTransformation);
        //auto test_str = ork_model_mtx.dump4x3cn();
        //logchan_meshutilassimp->log("NODE<%s> : %s\n", n->mName.data, test_str.c_str() );
        // ork_model_mtx  = ork_model_mtx.dump(n->mName.data);
        ork_normal_mtx = ork_model_mtx.rotMatrix33();
        //deco::logchan_meshutilassimp->log(fvec3(1, 0, 0), "/////////////////////////////\n");
      }

      //////////////////////////////////////////////
      // visit node
      //////////////////////////////////////////////

      for (int i = 0; i < n->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[n->mMeshes[i]];
        /////////////////////////////////////////////
        // query which input data is available
        /////////////////////////////////////////////
        bool has_normals    = mesh->mNormals != nullptr;
        bool has_tangents   = mesh->mTangents != nullptr;
        bool has_bitangents = mesh->mBitangents != nullptr;
        bool has_colors     = mesh->mColors[0] != nullptr;
        bool has_uvs        = mesh->mTextureCoords[0] != nullptr;
        bool has_bones      = mesh->mNumBones != 0;
        OrkAssert(has_normals);
        // OrkAssert(has_tangents);
        // OrkAssert(has_bitangents);
        /////////////////////////////////////////////
        const char* name = mesh->mName.data;
        /////////////////////////////////////////////
        GltfMaterial* outmtl = materialmap[mesh->mMaterialIndex];
        /////////////////////////////////////////////
        // merge geometry
        /////////////////////////////////////////////
        auto& out_submesh = MergeSubMesh(name);
        auto& mtlset      = out_submesh.typedAnnotation<std::set<int>>("materialset");
        mtlset.insert(mesh->mMaterialIndex);
        auto& mtlref = out_submesh.typedAnnotation<GltfMaterial*>("gltfmaterial");
        mtlref       = outmtl;
        ork::meshutil::vertex muverts[4];
        logchan_meshutilassimp->log("processing numfaces<%d> %s", mesh->mNumFaces, outmtl->_name.c_str() );
        for (int t = 0; t < mesh->mNumFaces; ++t) {
          const aiFace* face = &mesh->mFaces[t];
          bool is_triangle   = (face->mNumIndices == 3);

          if (is_triangle) {
            for (int facevert_index = 0; facevert_index < 3; facevert_index++) {
              int index = face->mIndices[facevert_index];
              /////////////////////////////////////////////
              const auto& v  = mesh->mVertices[index];
              const auto& n  = mesh->mNormals[index];
              const auto& uv = (mesh->mTextureCoords[0])[index];
              const auto& b  = (mesh->mBitangents)[index];
              auto& muvtx    = muverts[facevert_index];
              muvtx.mPos     = fvec3(v.x, v.y, v.z).transform(ork_model_mtx).xyz();
              muvtx.mNrm     = fvec3(n.x, n.y, n.z).transform(ork_normal_mtx);

              _vertexExtents.Grow(muvtx.mPos);

              // logchan_meshutilassimp->log("v<%g %g %g>\n", v.x, v.y, v.z);
              // logchan_meshutilassimp->log("norm<%g %g %g>\n", muvtx.mNrm.x, muvtx.mNrm.y, muvtx.mNrm.z);
              if (has_colors)
                muvtx.mCol[0] = fvec4(1, 1, 1, 1);
              if (has_uvs) {
                muvtx.mUV[0].mMapTexCoord = fvec2(uv.x, uv.y);
                muvtx.mUV[0].mMapBiNormal = fvec3(b.x, b.y, b.z).transform(ork_normal_mtx);
              }
              /////////////////////////////////////////////
              // yuk -- assimp is not like gltf, or collada...
              // https://github.com/assimp/assimp/blob/master/code/glTF2/glTF2Importer.cpp#L904
              /////////////////////////////////////////////
              if (is_skinned) {
                auto itw = assimpweightlut.find(index);
                // OrkAssert(itw != assimpweightlut.end());
                if (itw != assimpweightlut.end()) {
                  auto influences = itw->second;
                  int numinf      = influences->_items.size();
                  OrkAssert(numinf > 0);
                  // logchan_meshutilassimp->log("vertex<%d> raw_numweights<%d>\n", index, numinf);
                  ///////////////////////////////////////////////////
                  // prune to no more than 4 weights
                  ///////////////////////////////////////////////////
                  std::multimap<float, XgmAssimpVertexWeightItem> largestWeightMap;
                  std::multimap<float, std::string> prunedWeightMap;
                  std::map<std::string, float> rawweightMap;
                  for (int inf = 0; inf < numinf; inf++) {
                    auto infl = influences->_items[inf];
                    float fw  = infl._weight;
                    if (fw < 0.001)
                      fw = 0.001;
                    auto remapped          = remapSkelName(infl._bonename);
                    rawweightMap[remapped] = fw;
                    /*if (fw != 0.0f)*/ {
                      auto xgminfl = XgmAssimpVertexWeightItem(infl);
                      auto pr      = std::make_pair(1.0f - fw, xgminfl);
                      largestWeightMap.insert(pr);
                    }
                    // logchan_meshutilassimp->log(" inf<%d> bone<%s> weight<%g>\n", inf, remapped.c_str(), fw);
                  }
                  int icount      = 0;
                  float totweight = 0.0f;
                  for (auto it : largestWeightMap) {
                    if (icount < 4) {
                      totweight += (1.0f - it.first);
                      icount++;
                    }
                  }
                  if (totweight == 0.0f)
                    totweight = 1.0f;
                  icount             = 0;
                  float newtotweight = 0.0f;
                  for (auto item : largestWeightMap) {
                    if (icount < 4) {
                      // normalize pruned weights
                      float w            = 1.0f - item.first;
                      float fjointweight = w / totweight;
                      newtotweight += fjointweight;
                      std::string name = item.second._bonename;
                      prunedWeightMap.insert(std::make_pair(fjointweight, remapSkelName(name)));
                      ++icount;
                    }
                  }
                  // logchan_meshutilassimp->log("newtotweight<%f>\n", newtotweight);
                  float fwtest = fabs(1.0f - newtotweight);
                  if (fwtest >= 0.001f) // ensure within tolerable error limit
                  {
                    logchan_meshutilassimp->log(
                        "WARNING weight pruning tolerance: vertex<%d> fwtest<%f> icount<%d> prunedWeightMapSize<%zu>",
                        index,
                        fwtest,
                        icount,
                        prunedWeightMap.size());
                    OrkAssert(false);
                    // orkerrorlog( "ERROR: <%s> vertex<%d> fwtest<%f> numpairs<%d> largestWeightMap<%d>\n",
                    // policy->mColladaOutName.c_str(), im, fwtest, inumpairs, largestWeightMap.size() ); orkerrorlog( "ERROR:
                    // <%s> cannot prune weight, out of tolerance. You must prune it manually\n", policy->mColladaOutName.c_str()
                    // ); return false;
                  }
                  ///////////////////////////////////////////////////
                  muvtx.miNumWeights = prunedWeightMap.size();
                  OrkAssert(muvtx.miNumWeights >= 0);
                  OrkAssert(muvtx.miNumWeights <= 4);
                  int windex = 0;
                  /////////////////////////////////
                  // init vertex with no influences
                  /////////////////////////////////
                  for (int iw = 0; iw < 4; iw++) {
                    muvtx.mJointNames[iw]   = root_skelnode->_name;
                    muvtx.mJointWeights[iw] = 0.0f;
                  }
                  /////////////////////////////////
                  float totw = 0.0f;
                  if (newtotweight == 0.0f)
                    newtotweight = 1.0f;
                  for (auto it = prunedWeightMap.rbegin(); it != prunedWeightMap.rend(); it++) {
                    float w = it->first / newtotweight;
                    OrkAssert(w >= 0.0f);
                    OrkAssert(w <= 1.0f);
                    muvtx.mJointNames[windex]   = it->second;
                    muvtx.mJointWeights[windex] = w;
                    // logchan_meshutilassimp->log("inf<%s:%g> ", it->second.c_str(), w);
                    totw += w;
                    windex++;
                  }
                  // logchan_meshutilassimp->log("numw<%d> totw<%g>\n", muvtx.miNumWeights, totw);
                  fwtest = fabs(1.0f - totw);
                  if (fwtest >= 0.01f) { // ensure within tolerable error limit
                    OrkAssert(false);
                    // logchan_meshutilassimp->log( "\n");
                  }
                }
              }
            }
            ork::meshutil::vertex_ptr_t outvtx[3] = {nullptr, nullptr, nullptr};
            outvtx[0]                             = out_submesh.newMergeVertex(muverts[0]);
            outvtx[1]                             = out_submesh.newMergeVertex(muverts[1]);
            outvtx[2]                             = out_submesh.newMergeVertex(muverts[2]);
            ork::meshutil::poly ply(outvtx, 3);
            out_submesh.MergePoly(ply);
          } else {
            logchan_meshutilassimp->log("non triangle");
          }
        }
        // logchan_meshutilassimp->log("  done processing numfaces<%d> ..\n", mesh->mNumFaces);
        /////////////////////////////////////////////
        // stats
        /////////////////////////////////////////////
        // int meshout_numtris = out_submesh.GetNumPolys(3);
        // int meshout_numquads = out_submesh.GetNumPolys(4);
        // int meshout_numverts = out_submesh.RefVertexPool().GetNumVertices();
        // logchan_meshutilassimp->log( "meshout_numtris<%d>\n", meshout_numtris );
        // logchan_meshutilassimp->log( "meshout_numquads<%d>\n", meshout_numquads );
        // logchan_meshutilassimp->log( "meshout_numverts<%d>\n", meshout_numverts );
        /////////////////////////////////////////////
      }

      //////////////////////////////////////////////
      // enqueue children
      //////////////////////////////////////////////

      for (int i = 0; i < n->mNumChildren; ++i) {
        auto child     = n->mChildren[i];
        auto child_mtx = child->mTransformation;

        // fmtx4 orkmtx = convertMatrix44(child_mtx);
        // ork_mtxstack.push_back(orkmtx);

        nodestack.push(child);
      }
    }
    // logchan_meshutilassimp->log("done parsing nodes for meshdata\n");
    //logchan_meshutilassimp->log("/////////////////////////////////////////////////////////////////\n");

    // is_skinned = false; // not yet
    (*_varmap)["is_skinned"].set<bool>(is_skinned);

    //////////////////////////////////////////////
    // build xgm skeleton
    //////////////////////////////////////////////

    if (is_skinned) {
    }

    _vertexExtents.EndGrow();
    _skeletonExtents.EndGrow();

    //////////////////////////////////////////////
  } // if(scene)

  //logchan_meshutilassimp->log("DONE: readFromAssimp\n");
}

///////////////////////////////////////////////////////////////////////////////

void configureXgmSkeleton(const ork::meshutil::Mesh& input, lev2::XgmModel& xgmmdlout) {

  auto parsedskel = input._varmap->valueForKey("parsedskel").get<parsedskeletonptr_t>();

  const auto& xgmskelnodes = parsedskel->_xgmskelmap;

  //logchan_meshutilassimp->log("NumSkelNodes<%d>\n", int(xgmskelnodes.size()));
  xgmmdlout.SetSkinned(true);
  auto& xgmskel = xgmmdlout.skeleton();
  xgmskel.resize(xgmskelnodes.size());
  for (auto& item : xgmskelnodes) {
    const std::string& JointName = item.first;
    auto skelnode                = item.second;
    auto parskelnode             = skelnode->_parent;
    int idx                      = skelnode->miSkelIndex;
    int pidx                     = parskelnode ? parskelnode->miSkelIndex : -1;
    //logchan_meshutilassimp->log("JointName<%s> skelnode<%p> parskelnode<%p> idx<%d> pidx<%d>\n", JointName.c_str(), (void*) skelnode, (void*) parskelnode, idx, pidx);

    xgmskel.AddJoint(idx, pidx, JointName);
    xgmskel._bindMatrices[idx] = skelnode ? skelnode->_bindMatrix : fmtx4();
    xgmskel._inverseBindMatrices[idx] = skelnode ? skelnode->_bindMatrixInverse : fmtx4();
    xgmskel.RefJointMatrix(idx)       = skelnode ? skelnode->_jointMatrix : fmtx4();
    xgmskel.RefNodeMatrix(idx)        = skelnode ? skelnode->_nodeMatrix : fmtx4();
  }
  /////////////////////////////////////
  // flatten the skeleton (WIP)
  /////////////////////////////////////

  //logchan_meshutilassimp->log("Flatten Skeleton\n");
  const auto& bonemarkset = (*input._varmap)["bonemarkset"].get<bonemarkset_t>();

  auto root          = parsedskel->rootXgmSkelNode();
  xgmskel.miRootNode = root ? root->miSkelIndex : -1;
  if (root) {
    lev2::XgmSkelNode::visitHierarchy(root,[&xgmskel](lev2::xgmskelnode_ptr_t node) {
      auto parent = node->_parent;
      if (parent) {
        bool ignore = (parent->_numBoundVertices == 0);
        ignore      = ignore and (node->_numBoundVertices == 0);
        if (ignore){
          //logchan_meshutilassimp->log("IGNORE<%s>\n", node->_name.c_str());
        }
        else {
          //logchan_meshutilassimp->log("ADD<%s>\n", node->_name.c_str());
          int iparentindex   = parent->miSkelIndex;
          int ichildindex    = node->miSkelIndex;
          lev2::XgmBone Bone = {iparentindex, ichildindex};
          xgmskel.addBone(Bone);
        }
      }
    });
    // xgmskel.dump();
  }

  //logchan_meshutilassimp->log("skeleton configuration complete..\n");
}

///////////////////////////////////////////////////////////////////////////////

template <typename ClusterizerType>
void clusterizeToolMeshToXgmMesh(const ork::meshutil::Mesh& inp_model, ork::lev2::XgmModel& out_model) {

  // logchan_meshutilassimp->log("BEGIN: clusterizing model\n");
  bool is_skinned = false;
  if (auto as_bool = inp_model._varmap->valueForKey("is_skinned").tryAs<bool>()) {
    is_skinned = as_bool.value();
  }
  out_model._varmap.mergeVars(*inp_model._varmap);
  //auto& inp_embtexmap = inp_model._varmap->typedValueForKey<lev2::embtexmap_t>("embtexmap").value();
  //auto& out_embtexmap = out_model._varmap.makeValueForKey<lev2::embtexmap_t>("embtexmap") = inp_embtexmap;

  out_model.ReserveMeshes(inp_model.RefSubMeshLut().size());
  ork::lev2::XgmMesh* out_mesh = new ork::lev2::XgmMesh;
  out_mesh->SetMeshName("Mesh1"_pool);
  out_model.AddMesh("Mesh1"_pool, out_mesh);

  auto VertexFormat = is_skinned //
                          ? ork::lev2::EVtxStreamFormat::V12N12B12T8I4W4
                          : ork::lev2::EVtxStreamFormat::V12N12B12T16;
  struct SubRec {
    ork::meshutil::submesh_ptr_t _toolsub;
    ork::meshutil::MaterialGroup* _toolmgrp     = nullptr;
    ork::meshutil::XgmClusterizer* _clusterizer = nullptr;
    ork::lev2::pbrmaterial_ptr_t _pbrmaterial;
  };

  typedef std::vector<SubRec> xgmsubvect_t;
  typedef std::map<GltfMaterial*, xgmsubvect_t> mtl2submap_t;
  typedef std::map<GltfMaterial*, ork::meshutil::MaterialGroup*> mtl2mtlmap_t;

  mtl2submap_t mtlsubmap;
  mtl2mtlmap_t mtlmtlmap;

  int subindex = 0;
  for (auto item : inp_model.RefSubMeshLut()) {
    // logchan_meshutilassimp->log("BEGIN: clusterizing submesh<%d>\n", subindex);
    subindex++;
    auto inp_submesh = item.second;
    auto& mtlset     = inp_submesh->typedAnnotation<std::set<int>>("materialset");
    auto gltfmtl     = inp_submesh->typedAnnotation<GltfMaterial*>("gltfmaterial");
    assert(mtlset.size() == 1); // assimp does 1 material per submesh

    auto mtlout = std::make_shared<ork::lev2::PBRMaterial>();
    mtlout->setTextureBaseName(FormatString("material%d", subindex));
    mtlout->SetName(AddPooledString(gltfmtl->_name.c_str()));
    mtlout->_colorMapName    = gltfmtl->_colormap;
    mtlout->_normalMapName   = gltfmtl->_normalmap;
    mtlout->_amboccMapName   = gltfmtl->_amboccmap;
    mtlout->_emissiveMapName   = gltfmtl->_emissivemap;
    mtlout->_mtlRufMapName   = gltfmtl->_metallicAndRoughnessMap;
    mtlout->_metallicFactor  = gltfmtl->_metallicFactor;
    mtlout->_roughnessFactor = gltfmtl->_roughnessFactor;
    mtlout->_baseColor       = gltfmtl->_baseColor;
    out_model.AddMaterial(mtlout);

    auto clusterizer                            = new ClusterizerType;
    clusterizer->_policy._skinned               = is_skinned;
    ork::meshutil::MaterialGroup* materialGroup = nullptr;
    auto it                                     = mtlmtlmap.find(gltfmtl);
    if (it == mtlmtlmap.end()) {
      materialGroup                  = new ork::meshutil::MaterialGroup;
      materialGroup->meMaterialClass = ork::meshutil::MaterialGroup::EMATCLASS_PBR;
      materialGroup->SetClusterizer(clusterizer);
      materialGroup->mMeshConfigurationFlags._skinned = is_skinned;
      materialGroup->meVtxFormat                      = VertexFormat;
      mtlmtlmap[gltfmtl]                              = materialGroup;
    } else
      materialGroup = it->second;

    ork::meshutil::XgmClusterTri clustertri;
    clusterizer->Begin();

    const auto& vertexpool = inp_submesh->RefVertexPool();
    const auto& polys      = inp_submesh->RefPolys();
    for (const auto& poly : polys) {
      assert(poly->GetNumSides() == 3);
      for (int i = 0; i < 3; i++)
        clustertri._vertex[i] = vertexpool.GetVertex(poly->GetVertexID(i));
      clusterizer->addTriangle(clustertri, materialGroup->mMeshConfigurationFlags);
    }

    clusterizer->End();

    ///////////////////////////////////////

    SubRec srec;
    srec._pbrmaterial = mtlout;
    srec._toolsub     = inp_submesh;
    srec._toolmgrp    = materialGroup;
    srec._clusterizer = clusterizer;
    mtlsubmap[gltfmtl].push_back(srec);

    ///////////////////////////////////////
    // logchan_meshutilassimp->log("END: clusterizing submesh<%d>\n", subindex);
  }

  //////////////////////////////////////////////////////////////////
  size_t count_subs = 0;
  for (auto item : mtlsubmap) {
    GltfMaterial* gltfm = item.first;
    auto& subvect       = item.second;
    for (auto& subrec : subvect) {
      count_subs++;
    }
  }
  //////////////////////////////////////////////////////////////////

  out_mesh->ReserveSubMeshes(count_subs);
  subindex = 0;

  //logchan_meshutilassimp->log("generating %d submeshes\n", (int)count_subs);

  for (auto item : mtlsubmap) {
    GltfMaterial* gltfm = item.first;
    auto& subvect       = item.second;
    for (auto& subrec : subvect) {
      auto pbr_material = subrec._pbrmaterial;
      auto clusterizer  = subrec._clusterizer;

      auto xgm_submesh       = new ork::lev2::XgmSubMesh;
      xgm_submesh->_material = pbr_material;
      out_mesh->AddSubMesh(xgm_submesh);
      subindex++;

      int inumclus = clusterizer->GetNumClusters();
      for (int icluster = 0; icluster < inumclus; icluster++) {
        lev2::ContextDummy DummyTarget;
        auto clusterbuilder = clusterizer->GetCluster(icluster);
        clusterbuilder->buildVertexBuffer(DummyTarget, VertexFormat);

        auto xgm_cluster = std::make_shared<lev2::XgmCluster>();
        xgm_submesh->_clusters.push_back(xgm_cluster);

        // logchan_meshutilassimp->log("building tristrip cluster<%d>\n", icluster);

        buildXgmCluster(DummyTarget, xgm_cluster, clusterbuilder,true);

        const int imaxvtx = xgm_cluster->_vertexBuffer->GetNumVertices();
        //logchan_meshutilassimp->log("xgm_cluster->_vertexBuffer<%p> imaxvtx<%d>\n", (void*) xgm_cluster->_vertexBuffer.get(), imaxvtx);
        // OrkAssert(false);
        // int inumclusjoints = XgmClus.mJoints.size();
        // for( int ib=0; ib<inumclusjoints; ib++ )
        //{
        //	const PoolString JointName = XgmClus.mJoints[ ib ];
        //	orklut<PoolString,int>::const_iterator itfind = mXgmModel.skeleton().mmJointNameMap.find( JointName );
        //	int iskelindex = (*itfind).second;
        //	XgmClus.mJointSkelIndices.push_back(iskelindex);
        //}
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
datablock_ptr_t assimpToXgm(datablock_ptr_t inp_datablock) {

  Mesh tmesh;
  tmesh.readFromAssimp(inp_datablock);

  ork::lev2::XgmModel xgmmdlout;
  bool is_skinned = false;
  if (auto as_bool = tmesh._varmap->valueForKey("is_skinned").tryAs<bool>()) {
    is_skinned = as_bool.value();
    if (is_skinned)
      configureXgmSkeleton(tmesh, xgmmdlout);
  }
  //logchan_meshutilassimp->log("clusterizing..\n");
  clusterizeToolMeshToXgmMesh<ork::meshutil::XgmClusterizerStd>(tmesh, xgmmdlout);

  auto vmin = tmesh._vertexExtents.Min();
  auto vmax = tmesh._vertexExtents.Max();
  auto smin = tmesh._skeletonExtents.Min();
  auto smax = tmesh._skeletonExtents.Max();
  auto center = (vmax+vmin)*0.5f;
  
  /////////////////////////
  // compute aabb
  /////////////////////////

  auto abs_vmaxdelta = (vmax-center).absolute();
  auto abs_vmindelta = (vmin-center).absolute();
  auto abs_max = abs_vmaxdelta.maxXYZ(abs_vmindelta);

  /////////////////////////
  // compute radius
  /////////////////////////

  float radius = 0.0f;
  float vmax_mag = (vmax-center).magnitude();
  float vmin_mag = (vmin-center).magnitude();
  if(vmax_mag>radius)
    radius = vmax_mag;
  if(vmin_mag>radius)
    radius = vmin_mag;

  /////////////////////////

  logchan_meshutilassimp->log("vtxext min<%g %g %g>", vmin.x, vmin.y, vmin.z);
  logchan_meshutilassimp->log("vtxext max<%g %g %g>", vmax.x, vmax.y, vmax.z);
  logchan_meshutilassimp->log("vtxext abs_max<%g %g %g>", abs_max.x, abs_max.y, abs_max.z );
  logchan_meshutilassimp->log("vtxext ctr<%g %g %g>", center.x, center.y, center.z);
  logchan_meshutilassimp->log("vtxext aactr<%g %g %g>", center.x, center.y, center.z);
  logchan_meshutilassimp->log("vtxext radius<%g>", radius);
  logchan_meshutilassimp->log("sklext min<%g %g %g>", smin.x, smin.y, smin.z);
  logchan_meshutilassimp->log("sklext max<%g %g %g>", smax.x, smax.y, smax.z);

  xgmmdlout.mBoundingCenter = center;
  xgmmdlout.mBoundingRadius = radius;
  xgmmdlout.mAABoundXYZ = center;
  xgmmdlout.mAABoundWHD = abs_max;

  return writeXgmToDatablock(&xgmmdlout);
}
} // namespace ork::meshutil

