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

VkTextureInterface::VkTextureInterface(vkcontext_rawptr_t ctx)
    : TextureInterface(ctx)
    , _contextVK(ctx) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::TexManInit() {
}

///////////////////////////////////////////////////////////////////////////////

bool VkTextureInterface::destroyTexture(texture_ptr_t ptex) {
  /*
  auto glto = tex->_impl.get<gltexobj_ptr_t>();
tex->_impl.set<void*>(nullptr);

void_lambda_t lamb = [=]() {
  if (glto) {
    if (glto->mObject != 0)
      glDeleteTextures(1, &glto->mObject);
  }
};
// opq::mainSerialQueue()->push(lamb,get_backtrace());
opq::mainSerialQueue()->enqueue(lamb);
*/
  return false;
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::ApplySamplingMode(Texture* ptex) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::UpdateAnimatedTexture(Texture* ptex, TextureAnimationInst* tai) {
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::generateMipMaps(Texture* ptex) {

  vktexobj_ptr_t vktex;
  if( auto as_vktext = ptex->_impl.tryAsShared<VulkanTextureObject>() ){
    vktex = as_vktext.value();
  } else {
    vktex = ptex->_impl.makeShared<VulkanTextureObject>(this);
  }

  auto cmdbuf = std::make_shared<CommandBuffer>();
  _contextVK->pushCommandBuffer(cmdbuf);
  auto cmdbuf_impl = _contextVK->_cmdbufcur_gfx;
  auto vk_cmdbuf   = cmdbuf_impl->_vkcmdbuf;

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = vktex->_vkimage;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  int32_t mipWidth  = ptex->_width;
  int32_t mipHeight = ptex->_height;

  bool keep_going = true;

    int mip_level = 0;
    while( keep_going ) {
        barrier.subresourceRange.baseMipLevel = mip_level;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            vk_cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0]                 = {0, 0, 0};
        blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = mip_level;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = {0, 0, 0};
        blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = mip_level+1;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;

        vkCmdBlitImage(
            vk_cmdbuf,
            vktex->_vkimage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vktex->_vkimage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            vk_cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1)
        mipWidth /= 2;
        if (mipHeight > 1)
        mipHeight /= 2;

        keep_going = (mipWidth > 1) || (mipHeight > 1);
        mip_level++;
    }

  barrier.subresourceRange.baseMipLevel = mip_level - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(
      vk_cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  _contextVK->popCommandBuffer();
  _contextVK->enqueueSecondaryCommandBuffer(cmdbuf);
  
}

///////////////////////////////////////////////////////////////////////////////

Texture* VkTextureInterface::createFromMipChain(MipChain* from_chain) {
    auto ptex = new Texture;
    auto vktex  = ptex->_impl.makeShared<VulkanTextureObject>(this);

    auto format = from_chain->_format;
    auto type = from_chain->_type;
    size_t num_levels = from_chain->_levels.size();

    VkImageCreateInfo imageInfo{};
      initializeVkStruct(imageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = VkFormat(format);
    imageInfo.extent.width  = from_chain->_width;
    imageInfo.extent.height = from_chain->_height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = num_levels;

    for( size_t l=0; l<num_levels; l++ ){

        auto level = from_chain->_levels[l];
        int level_width = level->_width;
        int level_height = level->_height;
        void* level_data = level->_data;
        size_t level_length = level->_length;

        // register level data with vulkan

        VkBufferCreateInfo BUFINFO;
        initializeVkStruct(BUFINFO, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        BUFINFO.size        = level_length;
        BUFINFO.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        BUFINFO.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkBuffer stagingBuffer;
        initializeVkStruct(stagingBuffer);
        VkResult ok = vkCreateBuffer(_contextVK->_vkdevice, &BUFINFO, nullptr, &stagingBuffer);

        /////////////////////////////////////
        // allocate image memory
        /////////////////////////////////////

        VkMemoryRequirements MEMREQ;
        VkMemoryAllocateInfo ALLOCINFO = {};
        initializeVkStruct(MEMREQ);
        initializeVkStruct(ALLOCINFO, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        vkGetBufferMemoryRequirements(_contextVK->_vkdevice, stagingBuffer, &MEMREQ);
        printf( "alignment<%zu>\n", MEMREQ.alignment );
        ALLOCINFO.allocationSize  = MEMREQ.size;
        VkMemoryPropertyFlags vkmemflags;
        vkmemflags               = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT //
                                 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // do not need flush...
        ALLOCINFO.memoryTypeIndex = _contextVK->_findMemoryType(MEMREQ.memoryTypeBits, vkmemflags);
        // printf( "memtypeindex = %u\n", ALLOCINFO.memoryTypeIndex );
        VkDeviceMemory vkmem;
        initializeVkStruct(vkmem);
        vkAllocateMemory(_contextVK->_vkdevice, &ALLOCINFO, nullptr, &vkmem);
        vkBindBufferMemory(_contextVK->_vkdevice, stagingBuffer, vkmem, 0);

        /////////////////////////////////////
        // map and copy
        /////////////////////////////////////

        void* data = nullptr;
        vkMapMemory(_contextVK->_vkdevice, vkmem, 0, level_length, 0, &data);
        memcpy(data, level_data, level_length);
        vkUnmapMemory(_contextVK->_vkdevice, vkmem);

        /////////////////////////////////////
        // add mip to texture image
        /////////////////////////////////////

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel   = l;
        mipSubRange.levelCount     = 1;
        mipSubRange.baseArrayLayer = 0;
        mipSubRange.layerCount     = 1;

        VkImageMemoryBarrier barrier = {};
        initializeVkStruct(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        barrier.oldLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout         = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image             = vktex->_vkimage;
        barrier.subresourceRange  = mipSubRange;
        barrier.srcAccessMask     = 0;
        barrier.dstAccessMask     = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            _contextVK->_cmdbufcur_gfx->_vkcmdbuf,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
        
        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageOffset       = {0, 0, 0};
        region.imageExtent       = {uint32_t(level_width), uint32_t(level_height), 1};

        vkCmdCopyBufferToImage(
            _contextVK->_cmdbufcur_gfx->_vkcmdbuf,
            stagingBuffer,
            vktex->_vkimage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);
        
        barrier.oldLayout         = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask     = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask     = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            _contextVK->_cmdbufcur_gfx->_vkcmdbuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        vkDestroyBuffer(_contextVK->_vkdevice, stagingBuffer, nullptr);
        vkFreeMemory(_contextVK->_vkdevice, vkmem, nullptr);


    }

    /////////////////////////////////////
    // create image view
    /////////////////////////////////////

    VkImageViewCreateInfo viewInfo{};
    initializeVkStruct(viewInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    viewInfo.image                           = vktex->_vkimage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VkFormat(format);
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = num_levels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;
    
    VkImageView vkimageview;
    initializeVkStruct(vkimageview);
    VkResult ok = vkCreateImageView(_contextVK->_vkdevice, &viewInfo, nullptr, &vkimageview);
    OrkAssert(VK_SUCCESS == ok);

    //vktex->_vkimageview = vkimageview;

    /////////////////////////////////////
    // create sampler
    /////////////////////////////////////

    VkSamplerCreateInfo samplerInfo{};
    initializeVkStruct(samplerInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable        = VK_TRUE;
    samplerInfo.maxAnisotropy           = 16;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = float(num_levels);

    VkSampler vksampler;
    initializeVkStruct(vksampler);
    ok = vkCreateSampler(_contextVK->_vkdevice, &samplerInfo, nullptr, &vksampler);
    OrkAssert(VK_SUCCESS == ok);

    //vktex->_vksampler = vksampler;

    /////////////////////////////////////
    // create descriptor image info
    /////////////////////////////////////

    VkDescriptorImageInfo descimageInfo{};
    descimageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descimageInfo.imageView   = vkimageview;
    descimageInfo.sampler     = vksampler;

    /////////////////////////////////////

    ptex->_texFormat = format;
    ptex->_width     = from_chain->_width;
    ptex->_height    = from_chain->_height;
    ptex->_depth     = 1;
    ptex->_num_mips  = num_levels;
   //ptex->_target    = ETEXTARGET_2D;
    ptex->_debugName = "vulkan_texture";

    return ptex;
}

///////////////////////////////////////////////////////////////////////////////

void VkTextureInterface::initTextureFromData(Texture* ptex, TextureInitData tid) {
  auto vktex  = ptex->_impl.makeShared<VulkanTextureObject>(this);
  ptex->_texFormat = tid._dst_format;
  ptex->_width     = tid._w;
  ptex->_height    = tid._h;
  ptex->_depth     = tid._d;
  ptex->_num_mips  = 1;
  ptex->_debugName = "vulkan_texture";

  VkImageCreateInfo imageInfo{};
  initializeVkStruct(imageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.format        = VkFormat(tid._dst_format);
  imageInfo.extent.width  = tid._w;
  imageInfo.extent.height = tid._h;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImage vkimage;
  initializeVkStruct(vkimage);
  VkResult ok = vkCreateImage(_contextVK->_vkdevice, &imageInfo, nullptr, &vkimage);
  OrkAssert(VK_SUCCESS == ok);
  vktex->_vkimage = vkimage;

  /////////////////////////////////////

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(_contextVK->_vkdevice, vkimage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  initializeVkStruct(allocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = _contextVK->_findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkDeviceMemory vkmem;
  initializeVkStruct(vkmem);
  ok = vkAllocateMemory(_contextVK->_vkdevice, &allocInfo, nullptr, &vkmem);
  OrkAssert(VK_SUCCESS == ok);
  //vktex->_vkmem = vkmem;

  vkBindImageMemory(_contextVK->_vkdevice, vkimage, vkmem, 0);

  /////////////////////////////////////

  VkImageViewCreateInfo viewInfo{};
  initializeVkStruct(viewInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
  viewInfo.image                           = vkimage;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = VkFormat(tid._dst_format);
  viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  VkImageView vkimageview;
  initializeVkStruct(vkimageview);
  ok = vkCreateImageView(_contextVK->_vkdevice, &viewInfo, nullptr, &vkimageview);
  OrkAssert(VK_SUCCESS == ok);

  //vktex->_vkimageview = vkimageview;

  /////////////////////////////////////

  VkSamplerCreateInfo samplerInfo{};
  initializeVkStruct(samplerInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = 16;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = 1.0f;

  VkSampler vksampler;
  initializeVkStruct(vksampler);
  ok = vkCreateSampler(_contextVK->_vkdevice, &samplerInfo, nullptr, &vksampler);
  OrkAssert(VK_SUCCESS == ok);

  //vktex->_vksampler = vksampler;

  /////////////////////////////////////

  VkDescriptorImageInfo descimageInfo{};
  descimageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descimageInfo.imageView   = vkimageview;
  descimageInfo.sampler     = vksampler;

  /////////////////////////////////////

  auto cmdbuf = std::make_shared<CommandBuffer>();
  _contextVK->pushCommandBuffer(cmdbuf);
  auto cmdbuf_impl = _contextVK->_cmdbufcur_gfx;
  auto vk_cmdbuf   = cmdbuf_impl->_vkcmdbuf;

  VkImageMemoryBarrier barrier{};
  initializeVkStruct(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
  barrier.oldLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout         = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image             = vkimage;
  barrier.subresourceRange  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  barrier.srcAccessMask     = 0;
  barrier.dstAccessMask     = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(
      vk_cmdbuf,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);

  VkBufferCreateInfo BUFINFO;
  initializeVkStruct(BUFINFO, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
  BUFINFO.size        = tid.computeDstSize();
  BUFINFO.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  BUFINFO.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer stagingBuffer;
  initializeVkStruct(stagingBuffer);
  ok = vkCreateBuffer(_contextVK->_vkdevice, &BUFINFO, nullptr, &stagingBuffer);
  OrkAssert(VK_SUCCESS == ok);

  /////////////////////////////////////
  // allocate image memory
  /////////////////////////////////////

  VkMemoryRequirements MEMREQ;
  VkMemoryAllocateInfo ALLOCINFO = {};
  initializeVkStruct(MEMREQ);
  initializeVkStruct(ALLOCINFO, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
  vkGetBufferMemoryRequirements(_contextVK->_vkdevice, stagingBuffer, &MEMREQ);
  printf( "alignment<%zu>\n", MEMREQ.alignment );
  ALLOCINFO.allocationSize  = MEMREQ.size;
  VkMemoryPropertyFlags vkmemflags;
  vkmemflags               = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT //
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // do not need flush...
  ALLOCINFO.memoryTypeIndex = _contextVK->_findMemoryType(MEMREQ.memoryTypeBits, vkmemflags);
  // printf( "memtypeindex = %u\n", ALLOCINFO.memoryTypeIndex );
  initializeVkStruct(vkmem);
  vkAllocateMemory(_contextVK->_vkdevice, &ALLOCINFO, nullptr, &vkmem);
  vkBindBufferMemory(_contextVK->_vkdevice, stagingBuffer, vkmem, 0);

  /////////////////////////////////////
  // map and copy
  /////////////////////////////////////

  void* data;
  vkMapMemory(_contextVK->_vkdevice, vkmem, 0, memRequirements.size, 0, &data);
  memcpy(data, tid._data, tid._truncation_length);
  vkUnmapMemory(_contextVK->_vkdevice, vkmem);

  /////////////////////////////////////

  VkBufferImageCopy region{};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  region.imageOffset       = {0, 0, 0};
  region.imageExtent       = {uint32_t(tid._w), uint32_t(tid._h), 1};

  vkCmdCopyBufferToImage(
      vk_cmdbuf,
      stagingBuffer,
      vkimage,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  vkDestroyBuffer(_contextVK->_vkdevice, stagingBuffer, nullptr);
  vkFreeMemory(_contextVK->_vkdevice, vkmem, nullptr);

  barrier.oldLayout         = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask     = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask     = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(
      vk_cmdbuf,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);
  
  _contextVK->popCommandBuffer();

  /////////////////////////////////////

  //ptex->_target = tid._type;
  ptex->_impl = vktex;
  ptex->_dirty = false;


}

///////////////////////////////////////////////////////////////////////////////

VkTextureAsyncTask::VkTextureAsyncTask() {
}

///////////////////////////////////////////////////////////////////////////////

VulkanTextureObject::VulkanTextureObject(vktxi_rawptr_t txi) {
}

///////////////////////////////////////////////////////////////////////////////

VulkanTextureObject::~VulkanTextureObject() {
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////
