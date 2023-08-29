////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "vulkan_ctx.h"

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::vulkan {
///////////////////////////////////////////////////////////////////////////////

VkTextureAsyncTask::VkTextureAsyncTask() {
}

///////////////////////////////////////////////////////////////////////////////

VkTextureObject::VkTextureObject(vktxi_ptr_t txi) {
}

///////////////////////////////////////////////////////////////////////////////

VkTextureObject::~VkTextureObject() {
}

///////////////////////////////////////////////////////////////////////////////

VkTextureInterface::VkTextureInterface(vkcontext_rawptr_t ctx)
    : _contextVK(ctx) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::TexManInit(void) {
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::_loadImageTexture(texture_ptr_t ptex, datablock_ptr_t inpdata) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::_loadXTXTexture(texture_ptr_t ptex, datablock_ptr_t inpdata) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::_loadDDSTexture(const AssetPath& fname, texture_ptr_t ptex) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::_loadDDSTexture(texture_ptr_t ptex, datablock_ptr_t inpdata) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::_loadVDSTexture(const AssetPath& fname, texture_ptr_t ptex) {
    return false;
}
//

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::_loadXTXTextureMainThreadPart(vktexloadreq_ptr_t req){

}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::_loadDDSTextureMainThreadPart(vktexloadreq_ptr_t req){

}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::LoadTexture(texture_ptr_t ptex, datablock_ptr_t inpdata) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::destroyTexture(texture_ptr_t ptex) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::LoadTexture(const AssetPath& fname, texture_ptr_t ptex) {
    return false;
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::SaveTexture(const ork::AssetPath& fname, Texture* ptex) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::ApplySamplingMode(Texture* ptex) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::UpdateAnimatedTexture(Texture* ptex, TextureAnimationInst* tai) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::initTextureFromData(Texture* ptex, TextureInitData tid) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::generateMipMaps(Texture* ptex) {
}

///////////////////////////////////////////////////////////////////////////////

Texture* VkTextureInterface::createFromMipChain(MipChain* from_chain) {
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////