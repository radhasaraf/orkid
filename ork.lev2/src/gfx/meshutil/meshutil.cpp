////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/orklut.hpp>
#include <ork/math/plane.h>
#include <ork/lev2/gfx/meshutil/meshutil.h>
#include <ork/util/logger.h>

namespace ork { namespace meshutil {

static logchannel_ptr_t logchan_meshutil = logger()->createChannel("meshutil",fvec3(1,.9,1));

/////////////////////////////////////////////////////////////////////////

Mesh::Mesh()
    : _mergeEdges(true) {

    _varmap = std::make_shared<varmap::VarMap>();
}

///////////////////////////////////////////////////////////////////////////////

Mesh::~Mesh() {
}

///////////////////////////////////////////////////////////////////////////////

void Mesh::Prune() {
  orkset<std::string> SubsToPrune;
  for (auto it = _submeshesByPolyGroup.begin(); it != _submeshesByPolyGroup.end(); it++) {
    const submesh& src_grp  = *it->second;
    const std::string& name = it->first;
    int inump               = src_grp.GetNumPolys();
    if (0 == inump) {
      SubsToPrune.insert(name);
    }
  }
  while (SubsToPrune.empty() == false) {
    const std::string& name = *SubsToPrune.begin();
    RemoveSubMesh(name);
    SubsToPrune.erase(SubsToPrune.begin());
  }
}

///////////////////////////////////////////////////////////////////////////////

void Mesh::Dump(const std::string& comment) const {
  if (1)
    return; //
  /*
    static int icnt   = 0;
    std::string fname = CreateFormattedString("tmeshout%d.txt", icnt);
    icnt++;
    FILE* fout = fopen(fname.c_str(), "wt");
    fprintf(fout, "////////////////////////////////////////////////\n");
    fprintf(fout, "////////////////////////////////////////////////\n");
    fprintf(fout, "// Mesh dump<%s>\n", comment.c_str());
    fprintf(fout, "////////////////////////////////////////////////\n");
    fprintf(fout, "////////////////////////////////////////////////\n");
    for (const auto& item : mShadingGroupToMaterialMap) {
      const std::string& key = item.first;
      const auto& val        = item.second;
      fprintf(fout,
              "// Mesh::shadinggroup<%s> material_id<%s> material_name<%s>\n",
              key.c_str(),
              val.mMaterialDaeId.c_str(),
              val.mMaterialName.c_str());
    }
    fprintf(fout, "////////////////////////////////////////////////\n");
    for (orkmap<std::string, std::string>::const_iterator itm = _annotations.begin(); itm != _annotations.end(); itm++) {
      const std::string& key = itm->first;
      const std::string& val = itm->second;
      fprintf(fout, "// Mesh::annokey<%s> annoval<%s>\n", key.c_str(), val.c_str());
    }
    fprintf(fout, "////////////////////////////////////////////////\n");
    for (auto it = _submeshesByPolyGroup.begin(); it != _submeshesByPolyGroup.end(); it++)
    { const submesh& src_grp  = *it->second; const std::string& name = it->first; int inump               = src_grp.GetNumPolys();
      fprintf(fout, "// Mesh::polygroup<%s> numpolys<%d>\n", name.c_str(), inump);
      const submesh::AnnotationMap& subannos = src_grp.RefAnnotations();
      for (orkmap<std::string, std::string>::const_iterator itm = subannos.begin(); itm != subannos.end(); itm++) {
        const std::string& key = itm->first;
        const std::string& val = itm->second;
        fprintf(fout, "//		submesh::annokey<%s> annoval<%s>\n", key.c_str(), val.c_str());
      }
      orkset<const AnnoMap*> annosets;
      for (int ip = 0; ip < inump; ip++) {
        const poly& ply     = src_grp.RefPoly(ip);
        const AnnoMap* amap = ply.GetAnnoMap();
        if (amap)
          annosets.insert(amap);
      }
      for (orkset<const AnnoMap*>::const_iterator it2 = annosets.begin(); it2 != annosets.end(); it2++) {
        const AnnoMap* amapp                         = (*it2);
        const orkmap<std::string, std::string>& amap = amapp->_annotations;
        for (orkmap<std::string, std::string>::const_iterator it3 = amap.begin(); it3 != amap.end(); it3++) {
          const std::string& key = it3->first;
          const std::string& val = it3->second;
          fprintf(fout, "//			submeshpoly::annokey<%s> annoval<%s>\n", key.c_str(), val.c_str());
        }
      }
      for (int ip = 0; ip < inump; ip++) {
        fprintf(fout, "poly<%d> <", ip);
        const poly& ply = src_grp.RefPoly(ip);
        int inumv       = ply.GetNumSides();
        for (int iv = 0; iv < inumv; iv++) {
          int i0  = iv;
          int i1  = (iv + 1) % inumv;
          int iv0 = ply.GetVertexID(i0);
          int iv1 = ply.GetVertexID(i1);
          edge Edge(iv0, iv1);
          u64 ue  = src_grp.edgeBetween(iv0, iv1);
          u32 ue0 = u32(ue & 0xffffffff);
          u32 ue1 = u32((ue >> 32) & 0xffffffff);

          fprintf(fout, "%d:%08x%08x ", ply.GetVertexID(iv), ue0, ue1);
        }
        fprintf(fout, ">\n");
      }
    }
    fprintf(fout, "////////////////////////////////////////////////\n");
    fclose(fout);
    */
}

///////////////////////////////////////////////////////////////////////////////

void Mesh::CopyMaterialsFromToolMesh(const Mesh& from) {
  _materialsByShadingGroup   = from._materialsByShadingGroup;
  _materialsByName           = from._materialsByName;
  mShadingGroupToMaterialMap = from.mShadingGroupToMaterialMap;
}

///////////////////////////////////////////////////////////////////////////////

void Mesh::SetAnnotation(const char* annokey, const char* annoval) {
  std::string aval = "";
  if (annoval != 0)
    aval = annoval;
  _annotations[std::string(annokey)] = aval;
}
void submesh::setStringAnnotation(const char* annokey, std::string annoval) {
  _annotations[std::string(annokey)] = annoval;
}

///////////////////////////////////////////////////////////////////////////////

const char* Mesh::GetAnnotation(const char* annokey) const {
  static const char* defret("");
  orkmap<std::string, std::string>::const_iterator it = _annotations.find(std::string(annokey));
  if (it != _annotations.end()) {
    return (*it).second.c_str();
  }
  return defret;
}

///////////////////////////////////////////////////////////////////////////////
/*
void submesh::SplitOnAnno(Mesh& out, const std::string& annokey) const {
  int inumpolys = (int)_orderedPolys.size();
  for (int ip = 0; ip < inumpolys; ip++) {
    const poly& ply    = _orderedPolys[ip];
    std::string pgroup = ply.GetAnnotation(annokey);
    if (pgroup != "") {
      submesh& sub = out.MergeSubMesh(pgroup.c_str());
      int inumpv   = ply.GetNumSides();
      poly NewPoly;
      NewPoly.miNumSides = inumpv;
      for (int iv = 0; iv < inumpv; iv++) {
        int ivi                = ply.GetVertexID(iv);
        const vertex& vtx      = RefVertexPool().GetVertex(ivi);
        int inewvi             = sub.MergeVertex(vtx);
        NewPoly.miVertices[iv] = inewvi;
      }
      NewPoly.SetAnnoMap(ply.GetAnnoMap());
      sub.MergePoly(NewPoly);
    }
  }
}*/

///////////////////////////////////////////////////////////////////////////////
/*
void submesh::SplitOnAnno(Mesh& out, const std::string& prefix, const std::string& annokey) const {
  int inumpolys = (int)_orderedPolys.size();
  AnnotationMap merge_annos;
  merge_annos["SplitPrefix"] = prefix;
  for (int ip = 0; ip < inumpolys; ip++) {
    const poly& ply         = _orderedPolys[ip];
    std::string pgroup      = ply.GetAnnotation(annokey);
    std::string merged_name = prefix + std::string("_") + pgroup;
    if (pgroup != "") {
      submesh& sub = out.MergeSubMesh(merged_name.c_str(), merge_annos);
      int inumpv   = ply.GetNumSides();
      poly NewPoly;
      NewPoly.miNumSides = inumpv;
      for (int iv = 0; iv < inumpv; iv++) {
        int ivi                = ply.GetVertexID(iv);
        const vertex& vtx      = RefVertexPool().GetVertex(ivi);
        int inewvi             = sub.MergeVertex(vtx);
        NewPoly.miVertices[iv] = inewvi;
      }
      NewPoly.SetAnnoMap(ply.GetAnnoMap());
      sub.MergePoly(NewPoly);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void submesh::SplitOnAnno(Mesh& out, const std::string& annokey, const AnnotationMap& mrgannos) const {
  int inumpolys = (int)_orderedPolys.size();
  for (int ip = 0; ip < inumpolys; ip++) {
    const poly& ply    = _orderedPolys[ip];
    std::string pgroup = ply.GetAnnotation(annokey);
    if (pgroup != "") {
      submesh& sub = out.MergeSubMesh(pgroup.c_str());
      sub.MergeAnnos(mrgannos, true);
      int inumpv = ply.GetNumSides();
      poly NewPoly;
      NewPoly.miNumSides = inumpv;
      for (int iv = 0; iv < inumpv; iv++) {
        int ivi                = ply.GetVertexID(iv);
        const vertex& vtx      = RefVertexPool().GetVertex(ivi);
        int inewvi             = sub.MergeVertex(vtx);
        NewPoly.miVertices[iv] = inewvi;
      }
      NewPoly.SetAnnoMap(ply.GetAnnoMap());
      sub.MergePoly(NewPoly);
    }
  }
}*/

///////////////////////////////////////////////////////////////////////////////

AABox Mesh::GetAABox() const {
  AABox outp;
  outp.BeginGrow();
  int inumsub = (int)_submeshesByPolyGroup.size();
  for (auto it : _submeshesByPolyGroup) {
    auto sub = it.second;
    outp.Grow(sub->aabox().Min());
    outp.Grow(sub->aabox().Max());
  }
  outp.EndGrow();
  return outp;
}

///////////////////////////////////////////////////////////////////////////////

const submesh_lut_t& Mesh::RefSubMeshLut() const {
  return _submeshesByPolyGroup;
}

///////////////////////////////////////////////////////////////////////////////

/*const orkvector<submesh *> &Mesh::RefPolyGroupByPolyIndex() const
{
    return mPolyGroupByPolyIndex;
}*/

///////////////////////////////////////////////////////////////////////////////

submesh_constptr_t Mesh::submeshFromMaterialName(const std::string& materialname) const {
  for (const auto& item : mShadingGroupToMaterialMap) {
    if (materialname == item.second.mMaterialDaeId) {
      const std::string& shgrpname = item.first;
      auto itpg                    = _submeshesByPolyGroup.find(shgrpname);
      if (itpg != _submeshesByPolyGroup.end()) {
        return itpg->second;
      }
    }
  }
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

submesh_ptr_t Mesh::submeshFromMaterialName(const std::string& materialname) {
  for (const auto& item : mShadingGroupToMaterialMap) {
    if (materialname == item.second.mMaterialDaeId) {
      const std::string& shgrpname = item.first;
      auto itpg                    = _submeshesByPolyGroup.find(shgrpname);
      if (itpg != _submeshesByPolyGroup.end()) {
        return itpg->second;
      }
    }
  }
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

submesh_constptr_t Mesh::submeshFromGroupName(const std::string& polygroupname) const {
  auto it = _submeshesByPolyGroup.find(polygroupname);
  return (it == _submeshesByPolyGroup.end()) ? nullptr : it->second;
}

///////////////////////////////////////////////////////////////////////////////

submesh_ptr_t Mesh::submeshFromGroupName(const std::string& polygroupname) {
  auto it = _submeshesByPolyGroup.find(polygroupname);
  return (it == _submeshesByPolyGroup.end()) ? nullptr : it->second;
}

///////////////////////////////////////////////////////////////////////////////

void Mesh::SetRangeTransform(const fvec4& vscale, const fvec4& vtrans) {
  fmtx4 MatS, MatT;
  MatS.scale(vscale.x, vscale.y, vscale.z);
  MatT.setTranslation(vtrans.x, vtrans.y, vtrans.z);
  mMatRange = fmtx4::multiply_ltor(MatS,MatT);
}

///////////////////////////////////////////////////////////////////////////////

void Mesh::RemoveSubMesh(const std::string& pgroup) {
  auto it = _submeshesByPolyGroup.find(pgroup);
  if (it != _submeshesByPolyGroup.end()) {
    _submeshesByPolyGroup.erase(it);
  }
}

MaterialInfo::MaterialInfo()
    : _orkMaterial(0) {
}

///////////////////////////////////////////////////////////////////////////////

FlatSubMesh::FlatSubMesh( const AssetPath& from_path, lev2::rendervar_strmap_t assetvars ){
  Mesh mesh;
  mesh.readFromAssimp(from_path);
  const auto& submeshes = mesh.RefSubMeshLut();
  logchan_meshutil->log( "found submeshcount<%d>", int(submeshes.size()));

  auto out_submesh = std::make_shared<submesh>();
  for( auto item : submeshes ){
    auto groupname = item.first;
    logchan_meshutil->log( "merging submesh<%s>", groupname.c_str() );
    out_submesh->MergeSubMesh(*item.second);
  }
  fromSubmesh(*out_submesh);
}

///////////////////////////////////////////////////////////////////////////////

FlatSubMesh::FlatSubMesh(const submesh& mesh) {
    fromSubmesh(mesh);
}

///////////////////////////////////////////////////////////////////////////////

void FlatSubMesh::fromSubmesh(const submesh& mesh){
  ////////////////////////////////////////////////////////
  _aabox = mesh.aabox();
  ////////////////////////////////////////////////////////
  auto vpool = mesh._vtxpool;
  ////////////////////////////////////////////////////////
  mesh.FindNSidedPolys(TrianglePolyIndices, 3);
  mesh.FindNSidedPolys(QuadPolyIndices, 4);
  int inumv   = (int)vpool->GetNumVertices();
  int inumtri = int(TrianglePolyIndices.size());
  int inumqua = int(QuadPolyIndices.size());
  ////////////////////////////////////////////////////////
  evtxformat = lev2::EVtxStreamFormat::V12N12B12T8C4;
  lev2::SVtxV12N12B12T8C4 OutVertex;
  ////////////////////////////////////////////////////////
  // generate vertices
  ////////////////////////////////////////////////////////
  for (int iv0 = 0; iv0 < inumv; iv0++) {
    const vertex& invtx = vpool->GetVertex(iv0);
    OutVertex.mPosition = invtx.mPos;
    OutVertex.mNormal   = invtx.mNrm;
    OutVertex.mBiNormal = invtx.mUV[0].mMapBiNormal;
    OutVertex.mUV0      = invtx.mUV[0].mMapTexCoord;
    OutVertex.mColor    = invtx.mCol[0].RGBAU32();
    MergeVertsT8.push_back(OutVertex);
  }
  ////////////////////////////////////////////////////////
  // generate triangles
  ////////////////////////////////////////////////////////
  for (int it = 0; it < inumtri; it++) {
    int idx           = TrianglePolyIndices[it];
    const poly& intri = mesh.RefPoly(idx);
    int inumsides     = intri.GetNumSides();
    for (int iv = 0; iv < inumsides; iv++) {
      int idx = intri.GetVertexID(iv);
      MergeTriIndices.push_back(idx);
    }
  }
  ////////////////////////////////////////////////////////
  // generate triangles (from quads)
  ////////////////////////////////////////////////////////
  for (int iq = 0; iq < inumqua; iq++) {
    int idx           = QuadPolyIndices[iq];
    const poly& inqua = mesh.RefPoly(idx);
    int inumsides     = inqua.GetNumSides();
    int idx0          = inqua.GetVertexID(0);
    int idx1          = inqua.GetVertexID(1);
    int idx2          = inqua.GetVertexID(2);
    int idx3          = inqua.GetVertexID(3);

    MergeTriIndices.push_back(idx0);
    MergeTriIndices.push_back(idx1);
    MergeTriIndices.push_back(idx2);

    MergeTriIndices.push_back(idx0);
    MergeTriIndices.push_back(idx2);
    MergeTriIndices.push_back(idx3);
  }
  ////////////////////////////////////////////////////////
  inumverts = inumv;
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace ork::meshutil
