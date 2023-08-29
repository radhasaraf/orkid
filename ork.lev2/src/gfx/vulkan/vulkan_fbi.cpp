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

VkFboObject::VkFboObject() {
}

///////////////////////////////////////////////////////

VkFrameBufferInterface::VkFrameBufferInterface(vkcontext_rawptr_t ctx)
    : FrameBufferInterface(*ctx)
    , _contextVK(ctx) {
}

///////////////////////////////////////////////////////

VkFrameBufferInterface::~VkFrameBufferInterface() {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::SetRtGroup(RtGroup* Base) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::Clear(const fcolor4& rCol, float fdepth) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::clearDepth(float fdepth) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::_setViewport(int iX, int iY, int iW, int iH) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::_setScissor(int iX, int iY, int iW, int iH) {
}

///////////////////////////////////////////////////////
void VkFrameBufferInterface::_doBeginFrame(void) {

}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::_doEndFrame(void) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::capture(const RtBuffer* inpbuf, const file::Path& pth) {
}

///////////////////////////////////////////////////////

bool VkFrameBufferInterface::captureToTexture(const CaptureBuffer& capbuf, Texture& tex) {
  return false;
}

///////////////////////////////////////////////////////

bool VkFrameBufferInterface::captureAsFormat(const RtBuffer* inpbuf, CaptureBuffer* buffer, EBufferFormat destfmt) {
    return false;
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::GetPixel(const fvec4& rAt, PixelFetchContext& ctx) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::rtGroupClear(RtGroup* rtg) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::rtGroupMipGen(RtGroup* rtg) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::msaaBlit(rtgroup_ptr_t src, rtgroup_ptr_t dst) {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::blit(rtgroup_ptr_t src, rtgroup_ptr_t dst) {
}

///////////////////////////////////////////////////////
void VkFrameBufferInterface::downsample2x2(rtgroup_ptr_t src, rtgroup_ptr_t dst) {

}

//////////////////////////////////////////////

void VkFrameBufferInterface::_setAsRenderTarget() {
}

///////////////////////////////////////////////////////

void VkFrameBufferInterface::_initializeContext(DisplayBuffer* pBuf){

}

///////////////////////////////////////////////////////

freestyle_mtl_ptr_t VkFrameBufferInterface::utilshader() {
  return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////