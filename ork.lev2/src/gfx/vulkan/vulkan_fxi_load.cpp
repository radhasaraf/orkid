#include "vulkan_ctx.h"
#include "../shadlang/shadlang_backend_spirv.h"

#if defined(__APPLE__)
// #include <MoltenVK/mvk_vulkan.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::vulkan {
///////////////////////////////////////////////////////////////////////////////
using namespace shadlang;
///////////////////////////////////////////////////////////////////////////////

VkFxShaderObject::VkFxShaderObject(vkcontext_rawptr_t ctx, vkfxshader_bin_t bin) //
    : _contextVK(ctx)                                                            //
    , _spirv_binary(bin) {                                                       //

  _vk_shadermoduleinfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  _vk_shadermoduleinfo.codeSize = bin.size() * sizeof(uint32_t);
  _vk_shadermoduleinfo.pCode    = bin.data();
  VkResult result               = vkCreateShaderModule( //
      _contextVK->_vkdevice,              //
      &_vk_shadermoduleinfo,              //
      nullptr,                            //
      &_vk_shadermodule);                 //
  OrkAssert(result == VK_SUCCESS);
}

VkFxShaderObject::~VkFxShaderObject() {
  vkDestroyShaderModule(_contextVK->_vkdevice, _vk_shadermodule, nullptr);
}

///////////////////////////////////////////////////////////////////////////////

bool VkFxInterface::LoadFxShader(const AssetPath& input_path, FxShader* pshader) {

  auto it = _fxshaderfiles.find(input_path);
  vkfxsfile_ptr_t vulkan_shaderfile;
  ////////////////////////////////////////////
  // if not yet loaded, load...
  ////////////////////////////////////////////
  if (it != _fxshaderfiles.end()) { // shader already loaded...
    vulkan_shaderfile = it->second;
  }
  else { // load
    auto str_read = ork::File::readAsString(input_path);
    OrkAssert(str_read != nullptr);
    vulkan_shaderfile = _loadShaderFromShaderText(pshader, input_path.c_str(), str_read->_data);
    _fxshaderfiles[input_path]     = vulkan_shaderfile;
  } 
  bool OK = (vulkan_shaderfile != nullptr);
  if(OK){
    vulkan_shaderfile->_shader_name = input_path.c_str();
    pshader->_internalHandle.set<vkfxsfile_ptr_t>(vulkan_shaderfile);
  }
  return OK;
}

///////////////////////////////////////////////////////////////////////////////

FxShader* VkFxInterface::shaderFromShaderText(const std::string& name, const std::string& shadertext) {
  FxShader* shader = new FxShader;
  vkfxsfile_ptr_t vulkan_shaderfile = _loadShaderFromShaderText(shader, name, shadertext);
  if (vulkan_shaderfile) {
    shader->_internalHandle.set<vkfxsfile_ptr_t>(vulkan_shaderfile);
    _fxshaderfiles[name]     = vulkan_shaderfile;
    vulkan_shaderfile->_shader_name = name;
  } else {
    delete shader;
    shader = nullptr;
  }
  return shader;
};

///////////////////////////////////////////////////////////////////////////////

vkfxsfile_ptr_t VkFxInterface::_loadShaderFromShaderText(FxShader* shader, //
                                                         const std::string& parser_name, //
                                                         const std::string& shadertext) { //
  auto basehasher = DataBlock::createHasher();
  basehasher->accumulateString("vkfxshader-1.0");
  basehasher->accumulateString(shadertext);
  uint64_t hashkey    = basehasher->result();
  auto vkfx_datablock = DataBlockCache::findDataBlock(hashkey);
  vkfxsfile_ptr_t vulkan_shaderfile;
  ////////////////////////////////////////////
  // shader binary already cached
  // first check precompiled shader cache
  ////////////////////////////////////////////
  if (vkfx_datablock) {
    OrkAssert(false);
  }
  ////////////////////////////////////////////
  // shader binary not cached, compile and cache
  ////////////////////////////////////////////
  else {

    /////////////////////////////////////////////////////////////////////////////
    // compile all shaders from translation unit
    /////////////////////////////////////////////////////////////////////////////

    vulkan_shaderfile              = std::make_shared<VkFxShaderFile>();
    auto transunit                 = shadlang::parseFromString(parser_name, shadertext);
    vulkan_shaderfile->_trans_unit = transunit;

    auto vtx_shaders = SHAST::AstNode::collectNodesOfType<SHAST::VertexShader>(transunit);
    auto frg_shaders = SHAST::AstNode::collectNodesOfType<SHAST::FragmentShader>(transunit);
    auto cu_shaders  = SHAST::AstNode::collectNodesOfType<SHAST::ComputeShader>(transunit);
    auto techniques  = SHAST::AstNode::collectNodesOfType<SHAST::Technique>(transunit);
    auto unisets     = SHAST::AstNode::collectNodesOfType<SHAST::UniformSet>(transunit);
    auto uniblks     = SHAST::AstNode::collectNodesOfType<SHAST::UniformBlk>(transunit);
    auto imports     = SHAST::AstNode::collectNodesOfType<SHAST::ImportDirective>(transunit);

    size_t num_vtx_shaders = vtx_shaders.size();
    size_t num_frg_shaders = frg_shaders.size();
    size_t num_cu_shaders  = cu_shaders.size();
    size_t num_techniques  = techniques.size();
    size_t num_unisets     = unisets.size();
    size_t num_uniblks     = uniblks.size();
    size_t num_imports     = imports.size();

    printf("num_vtx_shaders<%zu>\n", num_vtx_shaders);
    printf("num_frg_shaders<%zu>\n", num_frg_shaders);
    printf("num_cu_shaders<%zu>\n", num_cu_shaders);
    printf("num_techniques<%zu>\n", num_techniques);
    printf("num_unisets<%zu>\n", num_unisets);
    printf("num_uniblks<%zu>\n", num_uniblks);
    printf("num_imports<%zu>\n", num_imports);

    //////////////////

    auto SPC = std::make_shared<spirv::SpirvCompiler>(transunit,true);

    for( auto spirvuniset : SPC->_spirvuniformsets ){
      printf( "spirvuniset<%s>\n", spirvuniset.first.c_str() );
    }
    for( auto spirvuniblk : SPC->_spirvuniformblks ){
      printf( "spirvuniblk<%s>\n", spirvuniblk.first.c_str() );
    }

    //////////////////
    // uniformsets
    //////////////////

    auto convert_unisets = [&](std::unordered_map<std::string, spirv::spirvuniset_ptr_t>& spirv_unisets) //
        -> std::unordered_map<std::string, vkfxsuniset_ptr_t> {                              //
      std::unordered_map<std::string, vkfxsuniset_ptr_t> rval;
      for (auto spirv_item : spirv_unisets) {
        std::string name = spirv_item.first;
        auto spirv_uniset      = spirv_item.second;
        /////////////////////////////////////////////
        auto vk_uniset = std::make_shared<VkFxShaderUniformSet>();
        rval[name]     = vk_uniset;
        vulkan_shaderfile->_vk_uniformsets[name] = vk_uniset;
        /////////////////////////////////////////////
        vk_uniset->_descriptor_set_id = spirv_uniset->_descriptor_set_id;
        /////////////////////////////////////////////
        // rebuild _samplers_by_name
        /////////////////////////////////////////////
        for (auto item : spirv_uniset->_samplers_by_name) {
          auto vk_samp                             = std::make_shared<VkFxShaderUniformSetSampler>();
          vk_samp->_datatype                       = item.second->_datatype;
          vk_samp->_identifier                     = item.second->_identifier;
          vk_samp->_orkparam = std::make_shared<FxShaderParam>();
          vk_samp->_orkparam->_impl.set<VkFxShaderUniformSetSampler*>(vk_samp.get());
          vk_uniset->_samplers_by_name[item.first] = vk_samp;
          //printf( "uniset<%s> ADDING Sampler PARAM<%s>\n", name.c_str(), item.second->_identifier.c_str());
        }
        /////////////////////////////////////////////
        // rebuild _items_by_name
        /////////////////////////////////////////////
        for (auto item : spirv_uniset->_items_by_name) {
          auto vk_item                          = std::make_shared<VkFxShaderUniformSetItem>();
          vk_item->_datatype                    = item.second->_datatype;
          vk_item->_identifier                  = item.second->_identifier;
          vk_item->_orkparam = std::make_shared<FxShaderParam>();
          vk_item->_orkparam->_impl.set<VkFxShaderUniformSetItem*>(vk_item.get());
          vk_uniset->_items_by_name[item.first] = vk_item;
          //printf( "uniset<%s> ADDING Item PARAM<%s>\n", name.c_str(), item.second->_identifier.c_str());
        }
        /////////////////////////////////////////////
        // rebuild items_by_order
        /////////////////////////////////////////////
        vk_uniset->_items_by_order.clear();
        for (auto item : spirv_uniset->_items_by_order) {
          for (auto t : spirv_uniset->_items_by_name) {
            if (t.second == item) {
              auto vk_item = vk_uniset->_items_by_name[t.first];
              vk_uniset->_items_by_order.push_back(vk_item);
            }
          }
        }
      }
      return rval;
    };

    //////////////////
    // uniformblks
    //////////////////

    auto convert_uniblks = [&](std::unordered_map<std::string, spirv::spirvuniblk_ptr_t>& spirv_uniblks) //
        -> std::unordered_map<std::string, vkfxsuniblk_ptr_t> {                              //
      std::unordered_map<std::string, vkfxsuniblk_ptr_t> rval;
      for (auto spirv_item : spirv_uniblks) {
        std::string name = spirv_item.first;
        auto spirv_uniblk      = spirv_item.second;
        /////////////////////////////////////////////
        auto vk_uniblk = std::make_shared<VkFxShaderUniformBlk>();
        rval[name]     = vk_uniblk;
        vulkan_shaderfile->_vk_uniformblks[name] = vk_uniblk;
        /////////////////////////////////////////////
        vk_uniblk->_descriptor_set_id = spirv_uniblk->_descriptor_set_id;
        /////////////////////////////////////////////
        // rebuild _items_by_name
        /////////////////////////////////////////////
        for (auto item : spirv_uniblk->_items_by_name) {
          auto vk_item                          = std::make_shared<VkFxShaderUniformBlkItem>();
          vk_item->_datatype                    = item.second->_datatype;
          vk_item->_identifier                  = item.second->_identifier;
          vk_item->_orkparam = std::make_shared<FxShaderParam>();
          vk_item->_orkparam->_impl.set<VkFxShaderUniformBlkItem*>(vk_item.get());
          vk_uniblk->_items_by_name[item.first] = vk_item;
          //printf( "uniset<%s> ADDING Item PARAM<%s>\n", name.c_str(), item.second->_identifier.c_str());
        }
        /////////////////////////////////////////////
        // rebuild items_by_order
        /////////////////////////////////////////////
        vk_uniblk->_items_by_order.clear();
        for (auto item : spirv_uniblk->_items_by_order) {
          for (auto t : spirv_uniblk->_items_by_name) {
            if (t.second == item) {
              auto vk_item = vk_uniblk->_items_by_name[t.first];
              vk_uniblk->_items_by_order.push_back(vk_item);
            }
          }
        }
      }
      return rval;
    };

    //////////////////
    // vertex shaders
    //////////////////

    for (auto vshader : vtx_shaders) {
      SPC->processShader(vshader);
      auto vulkan_shobj                                       = std::make_shared<VkFxShaderObject>(_contextVK, SPC->_spirv_binary);
      vulkan_shobj->_astnode                                  = vshader;
      vulkan_shobj->_vk_uniformsets                           = convert_unisets(SPC->_spirvuniformsets);
      vulkan_shobj->_vk_uniformblks                           = convert_uniblks(SPC->_spirvuniformblks);
      vulkan_shobj->_STAGE                                    = "vertex"_crcu;
      vulkan_shaderfile->_vk_shaderobjects[SPC->_shader_name] = vulkan_shobj;
      auto& STGIV                                             = vulkan_shobj->_shaderstageinfo;
      STGIV.sType                                             = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      STGIV.stage                                             = VK_SHADER_STAGE_VERTEX_BIT;
      STGIV.module                                            = vulkan_shobj->_vk_shadermodule;
      STGIV.pName                                             = "main";
    }

    //////////////////
    // fragment shaders
    //////////////////

    for (auto fshader : frg_shaders) {
      SPC->processShader(fshader);
      auto vulkan_shobj                                       = std::make_shared<VkFxShaderObject>(_contextVK, SPC->_spirv_binary);
      vulkan_shobj->_astnode                                  = fshader;
      vulkan_shobj->_vk_uniformsets                           = convert_unisets(SPC->_spirvuniformsets);
      vulkan_shobj->_vk_uniformblks                           = convert_uniblks(SPC->_spirvuniformblks);
      vulkan_shobj->_STAGE                                    = "fragment"_crcu;
      vulkan_shaderfile->_vk_shaderobjects[SPC->_shader_name] = vulkan_shobj;
      auto& STGIF                                             = vulkan_shobj->_shaderstageinfo;
      STGIF.sType                                             = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      STGIF.stage                                             = VK_SHADER_STAGE_FRAGMENT_BIT;
      STGIF.module                                            = vulkan_shobj->_vk_shadermodule;
      STGIF.pName                                             = "main";
    }

    //////////////////
    // compute shaders
    //////////////////

    for (auto cshader : cu_shaders) {
      SPC->processShader(cshader); //
      auto vulkan_shobj                                       = std::make_shared<VkFxShaderObject>(_contextVK, SPC->_spirv_binary);
      vulkan_shobj->_astnode                                  = cshader;
      vulkan_shobj->_vk_uniformsets                           = convert_unisets(SPC->_spirvuniformsets);
      vulkan_shobj->_vk_uniformblks                           = convert_uniblks(SPC->_spirvuniformblks);
      vulkan_shobj->_STAGE                                    = "compute"_crcu;
      vulkan_shaderfile->_vk_shaderobjects[SPC->_shader_name] = vulkan_shobj;
      auto& STCIF                                             = vulkan_shobj->_shaderstageinfo;
      STCIF.sType                                             = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      STCIF.stage                                             = VK_SHADER_STAGE_COMPUTE_BIT;
      STCIF.module                                            = vulkan_shobj->_vk_shadermodule;
      STCIF.pName                                             = "main";
    }

    //////////////////
    // techniques (always VTG for now)
    //////////////////

    for (auto tek : techniques) {
      auto passes   = SHAST::AstNode::collectNodesOfType<SHAST::Pass>(tek);
      auto tek_name = tek->typedValueForKey<std::string>("object_name").value();
      auto vk_tek   = std::make_shared<VkFxShaderTechnique>();
      //
      auto ork_tek            = vk_tek->_orktechnique;
      ork_tek->_shader        = shader;
      ork_tek->_techniqueName = tek_name;
      //
      for (auto p : passes) {
        auto vk_pass    = std::make_shared<VkFxShaderPass>();
        auto vk_program = std::make_shared<VkFxShaderProgram>();

        auto vtx_shader_ref = p->findFirstChildOfType<SHAST::VertexShaderRef>();
        auto frg_shader_ref = p->findFirstChildOfType<SHAST::FragmentShaderRef>();
        auto stateblock_ref = p->findFirstChildOfType<SHAST::StateBlockRef>();
        OrkAssert(vtx_shader_ref);
        OrkAssert(frg_shader_ref);
        auto vtx_sema_id = vtx_shader_ref->findFirstChildOfType<SHAST::SemaIdentifier>();
        auto frg_sema_id = frg_shader_ref->findFirstChildOfType<SHAST::SemaIdentifier>();
        OrkAssert(vtx_sema_id);
        OrkAssert(frg_sema_id);
        auto vtx_name = vtx_sema_id->typedValueForKey<std::string>("identifier_name").value();
        auto frg_name = frg_sema_id->typedValueForKey<std::string>("identifier_name").value();
        auto vtx_obj  = vulkan_shaderfile->_vk_shaderobjects[vtx_name];
        auto frg_obj  = vulkan_shaderfile->_vk_shaderobjects[frg_name];
        OrkAssert(vtx_obj);
        OrkAssert(frg_obj);

        vk_program->_vtxshader = vtx_obj;
        vk_program->_frgshader = frg_obj;
        vk_pass->_vk_program   = vk_program;
        vk_tek->_vk_passes.push_back(vk_pass);
      }
      vulkan_shaderfile->_vk_techniques[tek_name] = vk_tek;
    } // for (auto tek : techniques) {
  }   // shader binary not cached, compile and cache..
  return vulkan_shaderfile;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////