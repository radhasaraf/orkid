////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include "vulkan_ctx.h"

ImplementReflectionX(ork::lev2::vulkan::VkContext, "VkContext");

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::vulkan {
///////////////////////////////////////////////////////////////////////////////

void VkContext::describeX(class_t* clazz) {

  clazz->annotateTyped<context_factory_t>("context_factory", [](){
    return VkContext::makeShared();
  });
}

///////////////////////////////////////////////////////

bool VkContext::HaveExtension(const std::string& extname) {
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

vkcontext_ptr_t VkContext::makeShared(){
  struct VkContextX : public VkContext {
    VkContextX() : VkContext() {}
  };
  auto ctx = std::make_shared<VkContextX>();
  return ctx;
}

///////////////////////////////////////////////////////////////////////////////

VkContext::VkContext() {

  ///////////////////////////////////////////////////////////////
  OrkAssert(_GVI != nullptr);
  auto vk_devinfo = _GVI->_device_infos[0];
  VkDeviceCreateInfo DCI = {};
  DCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  vkCreateDevice(vk_devinfo->_phydev, &DCI, nullptr, &_vkdevice);
  _vkphysicaldevice = vk_devinfo->_phydev;
  ///////////////////////////////////////////////////////////////

  _dwi = std::make_shared<VkDrawingInterface>(this);
  _imi = std::make_shared<VkImiInterface>(this);
  _rsi = std::make_shared<VkRasterStateInterface>(this);
  _msi = std::make_shared<VkMatrixStackInterface>(this);
  _fbi = std::make_shared<VkFrameBufferInterface>(this);
  _gbi = std::make_shared<VkGeometryBufferInterface>(this);
  _txi = std::make_shared<VkTextureInterface>(this);
  _fxi = std::make_shared<VkFxInterface>(this);
#if defined(ENABLE_COMPUTE_SHADERS)
  _ci = std::make_shared<VkComputeInterface>(this);
#endif 
}

///////////////////////////////////////////////////////

VkContext::~VkContext() {
}

///////////////////////////////////////////////////////

uint32_t VkContext::_findMemoryType(    //
    uint32_t typeFilter,                //
    VkMemoryPropertyFlags properties) { //
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(_vkphysicaldevice, &memProperties);
  for (uint32_t i=0; i<memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  OrkAssert(false);
  return 0;
}

///////////////////////////////////////////////////////

void VkContext::FxInit() {
}

///////////////////////////////////////////////////////////////////////

void VkContext::_doResizeMainSurface(int iw, int ih) {
}

///////////////////////////////////////////////////////

void VkContext::_doBeginFrame() {
}

///////////////////////////////////////////////////////
void VkContext::_doEndFrame() {
}

///////////////////////////////////////////////////////

void* VkContext::_doClonePlatformHandle() const {
  return nullptr;
}

//////////////////////////////////////////////
// Interfaces

FxInterface* VkContext::FXI() {
  return _fxi.get();
}

///////////////////////////////////////////////////////

ImmInterface* VkContext::IMI() {
  return _imi.get();
}

///////////////////////////////////////////////////////
RasterStateInterface* VkContext::RSI() {

  return _rsi.get();
}
///////////////////////////////////////////////////////

MatrixStackInterface* VkContext::MTXI() {
  return _msi.get();
}
///////////////////////////////////////////////////////

GeometryBufferInterface* VkContext::GBI() {
  return _gbi.get();
}
///////////////////////////////////////////////////////

FrameBufferInterface* VkContext::FBI() {
  return _fbi.get();
}
///////////////////////////////////////////////////////

TextureInterface* VkContext::TXI() {
  return _txi.get();
}
///////////////////////////////////////////////////////

#if defined(ENABLE_COMPUTE_SHADERS)
ComputeInterface* VkContext::CI() {
  return _ci.get();
};
#endif
///////////////////////////////////////////////////////

DrawingInterface* VkContext::DWI() {
  return _dwi.get();
}

///////////////////////////////////////////////////////////////////////

void VkContext::makeCurrentContext(void) {
}

// void debugLabel(GLenum target, GLuint object, std::string name);

//////////////////////////////////////////////

//////////////////////////////////////////////

// void AttachGLContext(CTXBASE* pCTFL);
// void SwapGLContext(CTXBASE* pCTFL);

///////////////////////////////////////////////////////

void VkContext::swapBuffers(CTXBASE* ctxbase) {
}

///////////////////////////////////////////////////////

void VkContext::initializeWindowContext(Window* pWin, CTXBASE* pctxbase) {

} // make a window

///////////////////////////////////////////////////////

void VkContext::initializeOffscreenContext(DisplayBuffer* pBuf) {

} // make a pbuffer

///////////////////////////////////////////////////////
void VkContext::initializeLoaderContext() {
}

///////////////////////////////////////////////////////

void VkContext::debugPushGroup(const std::string str) {
}

///////////////////////////////////////////////////////

void VkContext::debugPopGroup() {
}

///////////////////////////////////////////////////////

void VkContext::debugMarker(const std::string str) {
}

///////////////////////////////////////////////////////

void VkContext::TakeThreadOwnership() {
}

///////////////////////////////////////////////////////

bool VkContext::SetDisplayMode(DisplayMode* mode) {
  return false;
}
///////////////////////////////////////////////////////
void* VkContext::_doBeginLoad() {
  return nullptr;
}
///////////////////////////////////////////////////////
void VkContext::_doEndLoad(void* ploadtok) {
}
///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////
