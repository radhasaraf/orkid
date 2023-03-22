////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/math/plane.hpp>
#include <ork/lev2/gfx/meshutil/submesh.h>
#include <ork/lev2/gfx/meshutil/meshutil.h>
#include <deque>

///////////////////////////////////////////////////////////////////////////////
namespace ork::meshutil {
///////////////////////////////////////////////////////////////////////////////

std::vector<island_ptr_t> PolySet::splitByIsland() const{

  std::vector<island_ptr_t> islands;

  auto copy_of_polys = _polys;

  while(copy_of_polys.size()>0){

    std::unordered_set<poly_ptr_t> processed;

    for( auto p : copy_of_polys ){
      submesh::PolyVisitContext visit_ctx;
      visit_ctx._visitor = [&](poly_ptr_t p) -> bool {
          processed.insert(p);
          return true;
      };
      auto par_submesh = p->_parentSubmesh;
      par_submesh->visitConnectedPolys(p,visit_ctx);
    }

    if( processed.size() ){
      auto island = std::make_shared<Island>();
      islands.push_back(island);
      for( auto p : processed ){
        auto itp = copy_of_polys.find(p);
        if(itp!=copy_of_polys.end()){
          //OrkAssert(itp!=copy_of_polys.end());
          copy_of_polys.erase(itp);
          island->_polys.insert(p);
        }
      }
    }
  }
  return islands;
}

///////////////////////////////////////////////////////////////////////////////

std::unordered_map<uint64_t,polyset_ptr_t> PolySet::splitByPlane() const {

  OrkAssert(_polys.size()>0);
  std::unordered_map<uint64_t,polyset_ptr_t> polyset_by_plane;

  for (auto inp_poly : _polys ) {

    auto plane = inp_poly->computePlane();

    //////////////////////////////////////////////////////////
    // quantize normals
    //  2^28 possible encodings more or less equally distributed (octahedral encoding)
    //  -> each encoding covers 4.682e-8 steradians (12.57 steradians / 2^28)
    // TODO: make an argument ?
    //////////////////////////////////////////////////////////

    fvec2 nenc = plane.n.normalOctahedronEncoded();
    double normal_quantization = 16383.0;
    uint64_t ux = uint64_t(double(nenc.x)*normal_quantization);        // 14 bits
    uint64_t uy = uint64_t(double(nenc.y)*normal_quantization);        // 14 bits  (total of 2^28 possible normals ~= )

    //////////////////////////////////////////////////////////
    // quantize plane distance
    //   (64km [-32k..+32k] range with .25 millimeter precision)
    // TODO: make an argument ?
    //////////////////////////////////////////////////////////

    double distance_quantization = 4096.0;
    uint64_t ud = uint64_t( (plane.d+32767.0)*distance_quantization ); //  16+12 bits 
    uint64_t hash = ud | (ux<<32) | (uy<<48);

    auto it = polyset_by_plane.find(hash);
    polyset_ptr_t dest_set;
    if(it!=polyset_by_plane.end()){
      dest_set = it->second;
    }
    else{
      dest_set = std::make_shared<PolySet>();
      polyset_by_plane[hash] = dest_set;
    }
    dest_set->_polys.insert(inp_poly);

  }
  OrkAssert(polyset_by_plane.size()>0);
  return polyset_by_plane;
}

///////////////////////////////////////////////////////////////////////////////

edge_vect_t Island::boundaryEdges() const {

  //////////////////////////////////////////
  // grab poly indices present in island
  //////////////////////////////////////////
  std::unordered_set<int> polyidcs_in_island;
  for( auto p : _polys ){
    polyidcs_in_island.insert(p->_submeshIndex);
  }
  //////////////////////////////////////////
  // 
  //////////////////////////////////////////

  std::unordered_set<edge_ptr_t> loose_edges;
  for( auto p : _polys ){
    size_t num_edges = p->_edges.size();
    OrkAssert(num_edges!=2);
    int poly_index = p->_submeshIndex;

    // find num connections within island
    for(auto e : p->_edges) {
      int inumcon_in_island = 0;
      for( int con : e->_connectedPolys ){
        if(con!=poly_index){
          ///////////////////////////////
          // is connected poly in island?
          ///////////////////////////////
          auto it_in_island = polyidcs_in_island.find(con);
          if(it_in_island!=polyidcs_in_island.end()){
            inumcon_in_island++;
          }
          else{
          }
        }
      }
      if(inumcon_in_island==1){
        int va = e->_vertexA->_poolindex;
        int vb = e->_vertexB->_poolindex;
        printf("poly<%d> edge[%d->%d] inumcon_in_island<%d>\n", poly_index, va,vb,inumcon_in_island );
        loose_edges.insert(e);
      }
    } // for(auto e : p->_edges) {
  } // for( auto p : _polys ){

  edge_vect_t rval;
  for (auto edge : loose_edges) {
    rval.push_back(edge);
  }
  //////////////////////////////////////////
  return rval;
}

///////////////////////////////////////////////////////////////////////////////

edge_vect_t Island::boundaryLoop() const {

  //////////////////////////////////////////
  // grab poly indices present in island
  //////////////////////////////////////////
  std::unordered_set<int> polyidcs_in_island;
  for( auto p : _polys ){
    polyidcs_in_island.insert(p->_submeshIndex);
  }
  //////////////////////////////////////////
  // 
  //////////////////////////////////////////

  std::unordered_set<edge_ptr_t> loose_edges;
  for( auto p : _polys ){
    size_t num_edges = p->_edges.size();
    OrkAssert(num_edges!=2);
    int poly_index = p->_submeshIndex;

    // find num connections within island
    for(auto e : p->_edges) {
      int inumcon_in_island = 0;
      for( int con : e->_connectedPolys ){
        if(con!=poly_index){
          ///////////////////////////////
          // is connected poly in island?
          ///////////////////////////////
          auto it_in_island = polyidcs_in_island.find(con);
          if(it_in_island!=polyidcs_in_island.end()){
            inumcon_in_island++;
          }
          else{
          }
        }
      }
      int va = e->_vertexA->_poolindex;
      int vb = e->_vertexB->_poolindex;
      printf("poly<%d> edge[%d->%d] inumcon_in_island<%d>\n", poly_index, va,vb,inumcon_in_island );
      if(inumcon_in_island==0){
        loose_edges.insert(e);
      }
    } // for(auto e : p->_edges) {
  } // for( auto p : _polys ){

  EdgeChainLinker _linker;
  _linker._name = "findboundaryedges";
  for (auto edge : loose_edges) {
    _linker.add_edge(edge);
  }
  _linker.link();
  printf( "boundary edge_count<%zu> loop_count<%zu>\n", loose_edges.size(), _linker._edge_loops.size() );

  //////////////////////////////////////////
  edge_vect_t rval;
  if( _linker._edge_loops.size() == 1 ){
    auto loop = _linker._edge_loops[0];
    for( auto e : loop->_edges ){
      rval.push_back(e);
    }
  }
  //////////////////////////////////////////
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
} //namespace ork::meshutil {
///////////////////////////////////////////////////////////////////////////////
