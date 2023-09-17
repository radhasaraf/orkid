////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "vulkan_ctx.h"
#include <ork/lev2/gfx/shadman.h>

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

VkRasterState::VkRasterState(rasterstate_ptr_t rstate){
  initializeVkStruct(_VKRSCI, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
  initializeVkStruct(_VKDSSCI, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
  initializeVkStruct(_VKCBSI, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);

  _VKRSCI.depthClampEnable = VK_FALSE;
  _VKRSCI.rasterizerDiscardEnable = VK_FALSE;
  _VKRSCI.polygonMode = VK_POLYGON_MODE_FILL;
  _VKRSCI.lineWidth = 1.0f;
  _VKRSCI.cullMode = VK_CULL_MODE_NONE;
  _VKRSCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  _VKRSCI.depthBiasEnable = VK_FALSE;
  _VKRSCI.depthBiasConstantFactor = 0.0f; // Optional
  _VKRSCI.depthBiasClamp = 0.0f;          // Optional
  _VKRSCI.depthBiasSlopeFactor = 0.0f;    // Optional

  _VKDSSCI.depthTestEnable = VK_TRUE;
  _VKDSSCI.depthWriteEnable = VK_TRUE;
  _VKDSSCI.depthCompareOp = VK_COMPARE_OP_LESS;
  _VKDSSCI.depthBoundsTestEnable = VK_FALSE;
  _VKDSSCI.minDepthBounds = 0.0f; // Optional
  _VKDSSCI.maxDepthBounds = 1.0f; // Optional
  _VKDSSCI.stencilTestEnable = VK_FALSE;
  _VKDSSCI.front = {}; // Optional
  _VKDSSCI.back = {};  // Optional

  _VKCBATT.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  _VKCBATT.blendEnable = VK_FALSE;
  _VKCBATT.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  _VKCBATT.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  _VKCBATT.colorBlendOp = VK_BLEND_OP_ADD;
  _VKCBATT.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  _VKCBATT.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  _VKCBATT.alphaBlendOp = VK_BLEND_OP_ADD;

  _VKCBSI.logicOpEnable = VK_FALSE;
  _VKCBSI.logicOp = VK_LOGIC_OP_COPY; // Optional
  _VKCBSI.attachmentCount = 1;
  _VKCBSI.pAttachments = &_VKCBATT;
  _VKCBSI.blendConstants[0] = 0.0f; // Optional
  _VKCBSI.blendConstants[1] = 0.0f; // Optional
  _VKCBSI.blendConstants[2] = 0.0f; // Optional
  _VKCBSI.blendConstants[3] = 0.0f; // Optional

  static int counter = 0;
  _pipeline_bits = counter++;

}

///////////////////////////////////////////////////////////////////////////////

vkpipeline_obj_ptr_t VkFxInterface::_fetchPipeline(vkvtxbuf_ptr_t vb, //
                                                   vkprimclass_ptr_t primclass){ //

  vkpipeline_obj_ptr_t rval;
  auto fbi = _contextVK->_fbi;
  auto gbi = _contextVK->_gbi;

  ////////////////////////////////////////////////////
  // rasterstate info
  ////////////////////////////////////////////////////

  OrkAssert(_current_rasterstate!=nullptr);
  vkrasterstate_ptr_t vkrstate;
  if( auto try_vkrs = _current_rasterstate->_impl.tryAsShared<VkRasterState>() ){
    vkrstate = try_vkrs.value();
  }
  else{
    vkrstate = _current_rasterstate->_impl.makeShared<VkRasterState>(_current_rasterstate);
  }

  ////////////////////////////////////////////////////
  // get pipeline hash from permutations
  ////////////////////////////////////////////////////

  auto check_pb_range = [](int& inp,int nbits) -> int {
    int maxval = (1<<(nbits-1));
    OrkAssert(inp>=0);
    OrkAssert(inp<maxval);
    return inp;
  };

  int vb_pbits = check_pb_range(vb->_vertexConfig->_pipeline_bits,4);

  auto rtg = fbi->_active_rtgroup;
  auto rtg_impl = rtg->_impl.getShared<VkRtGroupImpl>();
  auto msaa_impl = rtg_impl->_msaaState;

  int rtg_pbits = check_pb_range(rtg_impl->_pipeline_bits,4);
  int pc_pbits = check_pb_range(primclass->_pipeline_bits,4);

  auto shprog = _currentVKPASS->_vk_program;
  int sh_pbits = check_pb_range(shprog->_pipeline_bits,8);


  uint64_t pipeline_hash = vb_pbits
                         | (rtg_pbits<<4)
                         | (pc_pbits<<8)
                         | (sh_pbits<<16);


  ////////////////////////////////////////////////////
  // find or create pipeline
  ////////////////////////////////////////////////////

  auto it = _pipelines.find(pipeline_hash);
  if( it == _pipelines.end() ){ // create pipeline
    rval = std::make_shared<VkPipelineObject>();
    _pipelines[pipeline_hash] = rval;
    rval->_vk_program = shprog;

    auto& CINFO = rval->_VKGFXPCI;
    initializeVkStruct(CINFO, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

    CINFO.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    CINFO.renderPass =  _contextVK->_fbi->_swapchain->_mainRenderPass->_vkrp;
    CINFO.subpass = 0;

    // count shader stages
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    if( shprog->_vtxshader )
      stages.push_back(shprog->_vtxshader->_shaderstageinfo);
    if( shprog->_frgshader )
      stages.push_back(shprog->_frgshader->_shaderstageinfo);
      
    CINFO.stageCount = stages.size();
    CINFO.pStages = stages.data();
    CINFO.pVertexInputState = &vb->_vertexConfig->_vertex_input_state;
    CINFO.pInputAssemblyState = &primclass->_input_assembly_state;

    ////////////////////////////////////////////////////
    // dynamic states (viewport, scissor)
    ////////////////////////////////////////////////////

    std::vector<VkDynamicState> dynamic_states = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    initializeVkStruct(dynamicState, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
    dynamicState.dynamicStateCount = dynamic_states.size(); // We have two dynamic states: viewport and scissor
    dynamicState.pDynamicStates = dynamic_states.data();

    CINFO.pDynamicState = & dynamicState; 

    ////////////////////////////////////////////////////
    // msaa state
    ////////////////////////////////////////////////////

    VkPipelineMultisampleStateCreateInfo MSAA = {};
    initializeVkStruct(MSAA, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    MSAA.sampleShadingEnable = VK_FALSE; // Enable/Disable sample shading
    MSAA.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    MSAA.minSampleShading = 1.0f; // Minimum fraction for sample shading; closer to 1 is smoother
    MSAA.pSampleMask = nullptr; // Optional
    MSAA.alphaToCoverageEnable = VK_FALSE; // Enable/Disable alpha to coverage
    MSAA.alphaToOneEnable = VK_FALSE; // Enable/Disable alpha to one

    CINFO.pMultisampleState = &msaa_impl->_VKSTATE; // todo : dynamic

    ////////////////////////////////////////////////////
    // raster states
    ////////////////////////////////////////////////////

    CINFO.pRasterizationState = & vkrstate->_VKRSCI; 
    CINFO.pDepthStencilState = & vkrstate->_VKDSSCI; 
    CINFO.pColorBlendState = & vkrstate->_VKCBSI; 

    ////////////////////////////////////////////////////
    // pipeline layout...
    ////////////////////////////////////////////////////

    VkPipelineLayoutCreateInfo PLCI;
    VkPipelineLayout PL;
    initializeVkStruct(PLCI, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);

    ////////////////////////////////////////////////////
    // push constants
    ////////////////////////////////////////////////////

    if(shprog->_pushConstantBlock){
      PLCI.pushConstantRangeCount = shprog->_pushConstantBlock->_ranges.size();
      PLCI.pPushConstantRanges = shprog->_pushConstantBlock->_ranges.data();
    }

    ////////////////////////////////////////////////////
    // descriptors
    ////////////////////////////////////////////////////

    /*
    VkDescriptorSetLayoutBinding bindings[16];
    VkDescriptorSetLayoutCreateInfo layoutinfo;
    VkDescriptorSetLayout dset_layout;
    */

    ////////////////////////////////////////////////////

    VkResult OK = vkCreatePipelineLayout( _contextVK->_vkdevice, // device 
                                          &PLCI,                 // pipeline layout create info
                                          nullptr,               // allocator
                                          &PL);                  // pipeline layout
    OrkAssert(VK_SUCCESS == OK);

    CINFO.layout = PL; // shader input data layout / bindings
                                   // TODO: from geometry, shader
    if(0)
    vkCreateGraphicsPipelines( _contextVK->_vkdevice, // device
                               VK_NULL_HANDLE,        // pipeline cache
                               1,                     // count
                               &CINFO,                // create info
                               nullptr,               // allocator
                               &rval->_pipeline);
  }
  else{ // pipeline already cached!
    rval = it->second;
  }

  ////////////////////////////////////////////////////
  OrkAssert(rval!=nullptr);
  ////////////////////////////////////////////////////
  return rval;
}

///////////////////////////////////////////////////////////////////////////////
  
VkFxShaderProgram::VkFxShaderProgram(){
  _pushdatabuffer.reserve(1024); // todo : grow as needed
}

///////////////////////////////////////////////////////////////////////////////

void VkFxShaderProgram::applyPendingParams(vkcmdbufimpl_ptr_t cmdbuf){

  OrkAssert(_pushConstantBlock!=nullptr);
  size_t num_params = _pending_params.size();
  /*BufferLayout layout(_pushdatabuffer);
  for( size_t i=0; i<num_params; i++ ){
    const auto& P = _pending_params[i];
    const auto& V = P._value;
    if(auto as_float = V.tryAs<float>()){
      layout.addItem<float>(as_float.value());
    }
    else if(auto as_vec4 = V.tryAs<fvec4>() ){
      layout.addItem<fvec4>(as_vec4.value());
    }
    else if(auto as_mtx4 = V.tryAs<fmtx4>() ){
      layout.addItem<fmtx4>(as_mtx4.value());
    }
    else{
      OrkAssert(false);
    }
  }
  size_t data_size = layout.size();*/
  //OrkAssert(data_size<=_pushConstantBlock->_size);

  _pending_params.clear();
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

VkFxShaderTechnique::VkFxShaderTechnique(){
  _orktechnique = std::make_shared<FxShaderTechnique>();
  _orktechnique->_impl.set<VkFxShaderTechnique*>(this);
}

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
