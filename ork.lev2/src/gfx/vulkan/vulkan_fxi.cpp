////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "vulkan_ctx.h"
#include "vulkan_ub_layout.inl"
#include <ork/lev2/gfx/shadman.h>
#include <ork/util/hexdump.inl>

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::vulkan {
///////////////////////////////////////////////////////////////////////////////

VkFxInterface::VkFxInterface(vkcontext_rawptr_t ctx)
    : _contextVK(ctx) {
    _slp_cache = _GVI->_slp_cache;

    _default_rasterstate = std::make_shared<lev2::SRasterState>();
}

///////////////////////////////////////////////////////////////////////////////

VkFxInterface::~VkFxInterface(){

}

///////////////////////////////////////////////////////////////////////////////

void VkFxInterface::_doBeginFrame() {
  _currentPipeline = nullptr;
  pushRasterState(_default_rasterstate);
}
void VkFxInterface::_doEndFrame() {
  popRasterState();
}

///////////////////////////////////////////////////////////////////////////////

void VkFxInterface::_doPushRasterState(rasterstate_ptr_t rs) {
  _rasterstate_stack.push(_current_rasterstate);
  _current_rasterstate = rs;
}
rasterstate_ptr_t VkFxInterface::_doPopRasterState() {
  _current_rasterstate = _rasterstate_stack.top();
  _rasterstate_stack.pop();
  return _current_rasterstate;
}

///////////////////////////////////////////////////////////////////////////////

int VkFxInterface::BeginBlock(fxtechnique_constptr_t tek, const RenderContextInstData& data) {
  auto vk_tek = tek->_impl.get<VkFxShaderTechnique*>();
  _currentORKTEK = tek;
  _currentVKTEK = vk_tek;
  return (int) vk_tek->_vk_passes.size();
}

///////////////////////////////////////////////////////////////////////////////

bool VkFxInterface::BindPass(int ipass) {
  auto pass = _currentVKTEK->_vk_passes[ipass];
  _currentVKPASS = pass;
  return true;
}

///////////////////////////////////////////////////////////////////////////////

void VkFxInterface::EndPass() {
}

///////////////////////////////////////////////////////////////////////////////

void VkFxInterface::EndBlock() {
}

///////////////////////////////////////////////////////////////////////////////

void VkFxInterface::CommitParams(void) {
}

///////////////////////////////////////////////////////////////////////////////

void VkFxInterface::reset() {
}

///////////////////////////////////////////////////////////////////////////////

int VkFxInterface::_pipelineBitsForShader(vkfxsprg_ptr_t shprog){

  if(shprog->_pipeline_bits_composite == -1){ // compute ?

    ////////////////////////////
    // compute VIF bits
    ////////////////////////////

    auto vtx_shader = shprog->_vtxshader;
    auto vif_id = vtx_shader->_vk_interfaces[0];
    auto it_vif = shprog->_shader_file->_vk_vtxinterfaces.find(vif_id);
    OrkAssert(it_vif!=shprog->_shader_file->_vk_vtxinterfaces.end());
    auto vif = it_vif->second;
    shprog->_vertexinterface = vif;

    boost::Crc64 crc;
    crc.init();
    for( auto input : vif->_inputs ){
      crc.accumulateString(input->_datatype);
      crc.accumulateString(input->_semantic);
    }
    crc.finish();
    uint64_t hash = crc.result();

    auto it = _vk_vtxinterface_cache.find(hash);
    if( it != _vk_vtxinterface_cache.end() ){
      vif->_pipeline_bits = it->second;
      vif->_hash = hash;
    }
    else{
      int new_index = _vk_vtxinterface_cache.size();
      _vk_vtxinterface_cache[hash] = new_index;
      vif->_pipeline_bits = new_index;
    }

    ////////////////////////////
    // compute composite bits
    ////////////////////////////

    OrkAssert(vif->_pipeline_bits<256);
    OrkAssert(shprog->_pipeline_bits_prg<256);

    shprog->_pipeline_bits_composite = vif->_pipeline_bits //
                                     | (shprog->_pipeline_bits_prg<<8);

  }
  return shprog->_pipeline_bits_composite;
}

///////////////////////////////////////////////////////////////////////////////

VkFxShaderTechnique::VkFxShaderTechnique(){
  _orktechnique = std::make_shared<FxShaderTechnique>();
  _orktechnique->_impl.set<VkFxShaderTechnique*>(this);
}

///////////////////////////////////////////////////////////////////////////////

VkFxShaderTechnique::~VkFxShaderTechnique(){
  _orktechnique->_impl.set<void*>(nullptr);
  _orktechnique->_techniqueName = "destroyed";
  _orktechnique->_passes.clear();
  _orktechnique->_shader = nullptr;
  _orktechnique->_validated = false;
  _orktechnique = nullptr;
}


///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////