///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2020, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

#include <orktool/orktool_pch.h>
#include <ork/file/cfs.inl>
#include <ork/application/application.h>
#include <ork/math/plane.h>
#include <orktool/filter/gfx/meshutil/meshutil.h>
#include <orktool/filter/filter.h>
#include <ork/kernel/spawner.h>
#include <ork/kernel/string/deco.inl>

///////////////////////////////////////////////////////////////////////////////
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/material.h>

#include <orktool/filter/gfx/collada/collada.h>
#include <orktool/filter/gfx/meshutil/meshutil.h>
#include <orktool/filter/gfx/meshutil/clusterizer.h>

#include <ork/lev2/gfx/material_pbr.inl>

namespace ork::MeshUtil {
///////////////////////////////////////////////////////////////////////////////

inline fmtx4 convertMatrix44(const aiMatrix4x4& inp) {
  fmtx4 rval;
  for (int i = 0; i < 16; i++) {
    int x = i % 4;
    int y = i / 4;
    rval.SetElemXY(x, y, inp[y][x]);
  }
  return rval;
}

///////////////////////////////////////////////////////////////////////////////

inline void
get_bounding_box_for_node(const aiScene* scene, const aiNode* nd, aiVector3D& min, aiVector3D& max, aiMatrix4x4& trafo) {
  aiMatrix4x4 prev;
  unsigned int n = 0, t;

  prev = trafo;
  aiMultiplyMatrix4(&trafo, &nd->mTransformation);

  for (; n < nd->mNumMeshes; ++n) {
    const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

    for (t = 0; t < mesh->mNumVertices; ++t) {

      aiVector3D tmp = mesh->mVertices[t];
      aiTransformVecByMatrix4(&tmp, &trafo);

      min.x = std::min(min.x, tmp.x);
      min.y = std::min(min.y, tmp.y);
      min.z = std::min(min.z, tmp.z);

      max.x = std::max(max.x, tmp.x);
      max.y = std::max(max.y, tmp.y);
      max.z = std::max(max.z, tmp.z);
    }
  }

  for (n = 0; n < nd->mNumChildren; ++n) {
    get_bounding_box_for_node(scene, nd->mChildren[n], min, max, trafo);
  }
  trafo = prev;
}

///////////////////////////////////////////////////////////////////////////////

struct GltfMaterial {
  std::string _name;
  std::string _metallicAndRoughnessMap;
  std::string _colormap;
  std::string _normalmap;
  std::string _amboccmap;
  std::string _emissivemap;
  float _metallicFactor  = 0.0f;
  float _roughnessFactor = 1.0f;
  fvec4 _baseColor       = fvec4(1, 1, 1, 1);
};

typedef std::map<int, GltfMaterial*> gltfmaterialmap_t;
typedef std::map<std::string, ork::lev2::XgmSkelNode*> skelnodemap_t;

///////////////////////////////////////////////////////////////////////////////

struct ParsedSkeleton {
  skelnodemap_t _xgmskelmap;
  bool _isSkinned = false;
};

///////////////////////////////////////////////////////////////////////////////

inline std::string remapSkelName(std::string inp) {
  // fixup blender naming
  auto remapped_name = ork::string::replaced(inp, "Armature_", "");
  remapped_name      = ork::string::replaced(remapped_name, "_", ".");
  return remapped_name;
}
///////////////////////////////////////////////////////////////////////////////

inline ParsedSkeleton parseSkeleton(const aiScene* scene) {
  std::queue<aiNode*> nodestack;
  std::set<std::string> uniqskelnodeset;

  ParsedSkeleton rval;
  skelnodemap_t& xgmskelnodes = rval._xgmskelmap;

  /////////////////////////////////////////////////
  // get nodes
  /////////////////////////////////////////////////

  nodestack = std::queue<aiNode*>();
  nodestack.push(scene->mRootNode);
  while (not nodestack.empty()) {
    auto n = nodestack.front();
    nodestack.pop();
    auto name = remapSkelName(n->mName.data);
    auto itb  = uniqskelnodeset.find(name);
    if (itb == uniqskelnodeset.end()) {
      int index = uniqskelnodeset.size();
      uniqskelnodeset.insert(name);
      auto xgmnode         = new ork::lev2::XgmSkelNode(name);
      xgmnode->miSkelIndex = index;
      xgmskelnodes[name]   = xgmnode;
      auto matrix          = n->mTransformation;
      printf("uniqNODE<%d:%p> xgmnode<%p> <%s>\n", index, n, xgmnode, name.c_str());
      xgmnode->mNodeMatrix = convertMatrix44(matrix);
    }
    for (int i = 0; i < n->mNumChildren; ++i) {
      nodestack.push(n->mChildren[i]);
    }
  }

  /////////////////////////////////////////////////
  // get bones
  /////////////////////////////////////////////////

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
        if (itb != xgmskelnodes.end()) {
          auto xgmnode = itb->second;
          if (false == xgmnode->_varmap["visited_bone"].IsA<bool>()) {
            xgmnode->_varmap["visited_bone"].Set<bool>(true);
            //////////////////////////////////////////
            // according to what I read
            //  aiBone::mOffsetMatrix is the inverse bind pose
            //  an odd name to be sure. we shall see..
            //////////////////////////////////////////

            auto matrix = bone->mOffsetMatrix;
            fmtx4 invbindpose;
            for (int j = 0; j < 4; j++) {
              for (int k = 0; k < 4; k++) {
                invbindpose.SetElemYX(j, k, matrix[j][k]);
              }
            }
            xgmnode->mBindMatrixInverse = invbindpose;
            rval._isSkinned             = true;
          }
        }
      }
    }
    for (int i = 0; i < n->mNumChildren; ++i) {
      nodestack.push(n->mChildren[i]);
    }
  }

  /////////////////////////////////////////////////
  // set parents
  /////////////////////////////////////////////////

  nodestack = std::queue<aiNode*>();
  nodestack.push(scene->mRootNode);
  while (not nodestack.empty()) {
    auto p = nodestack.front();
    nodestack.pop();
    //////////////////////////////
    auto itp = xgmskelnodes.find(remapSkelName(p->mName.data));
    OrkAssert(itp != xgmskelnodes.end());
    auto pskelnode = itp->second;
    if (p == scene->mRootNode) {
      pskelnode->mJointMatrix = pskelnode->bindMatrix();
    }
    //////////////////////////////
    for (int i = 0; i < p->mNumChildren; ++i) {
      auto c   = p->mChildren[i];
      auto itc = xgmskelnodes.find(remapSkelName(c->mName.data));
      OrkAssert(itc != xgmskelnodes.end());
      auto cskelnode = itc->second;
      nodestack.push(c);
      cskelnode->mpParent = pskelnode;
      fmtx4 cmtx          = cskelnode->bindMatrix();
      fmtx4 pmtx          = pskelnode->bindMatrix();
      cskelnode->mJointMatrix.CorrectionMatrix(pmtx, cmtx);
      pskelnode->mChildren.push_back(cskelnode);
    }
  }

  /////////////////////////////////////////////////

  return rval;
}

// bone   len<1.998>
// bone.1 len<1.42>
// bone.2 len<1.45>
// bone.3 len<1.18>

///////////////////////////////////////////////////////////////////////////////

} // namespace ork::MeshUtil
