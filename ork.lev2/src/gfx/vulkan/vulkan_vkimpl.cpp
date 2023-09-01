////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <ork/kernel/string/deco.inl>
#include <ork/lev2/lev2_asset.h>
#include <ork/asset/Asset.inl>
#if defined(ENABLE_VULKAN)
#include "vulkan_ctx.h"

namespace ork::lev2::vulkan {

vkinstance_ptr_t _GVI = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////

VulkanInstance::VulkanInstance() {
  
  auto yel = fvec3::Yellow();

  _appdata.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  _appdata.pNext              = NULL;
  _appdata.pApplicationName   = "Orkid";
  _appdata.applicationVersion = 1;
  _appdata.pEngineName        = "Orkid";
  _appdata.engineVersion      = 1;
  _appdata.apiVersion         = VK_API_VERSION_1_2;

  _instancedata.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  _instancedata.pNext                   = NULL;
  _instancedata.flags                   = 0;
  _instancedata.pApplicationInfo        = &_appdata;
  _instancedata.enabledExtensionCount   = 0;
  _instancedata.ppEnabledExtensionNames = NULL;
  _instancedata.enabledLayerCount       = 0;
  _instancedata.ppEnabledLayerNames     = NULL;

  _slp_cache = std::make_shared<shadlang::ShadLangParserCache>();

  static std::vector<const char*> instanceExtensions;

#if defined(__APPLE__)
  instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
  //instanceExtensions.push_back("VK_KHR_portability_subset");
  _instancedata.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  _instancedata.enabledExtensionCount = instanceExtensions.size();
  _instancedata.ppEnabledExtensionNames = instanceExtensions.data();

  VkResult res = vkCreateInstance(&_instancedata, NULL, &_instance);
  OrkAssert(res == 0);

  deco::printf(yel, "vulkan::_init res<%d>\n", int(res));

  /////////////////////////////////////////////////////////////////////////////
  // check device groups (for later multidevice support)
  /////////////////////////////////////////////////////////////////////////////

  res = vkEnumeratePhysicalDeviceGroups(_instance, &_numgroups, nullptr);
  _phygroups.resize(_numgroups);
  vkEnumeratePhysicalDeviceGroups(_instance, &_numgroups, _phygroups.data());
  deco::printf(yel, "vulkan::_init numgroups<%u>\n", _numgroups);
  int igroup = 0;
  for (auto& group : _phygroups) {
    vkdevgrp_ptr_t dev_group_out = std::make_shared<VulkanDeviceGroup>();
    _devgroups.push_back(dev_group_out);

    dev_group_out->_deviceCount = group.physicalDeviceCount;
    deco::printf(yel, "vulkan::_init grp<%d> numgpus<%zu>\n", igroup, dev_group_out->_deviceCount);
    for (int idev = 0; idev < dev_group_out->_deviceCount; idev++) {
      auto device_info = std::make_shared<VulkanDeviceInfo>();
      dev_group_out->_device_infos.push_back(device_info);
      _device_infos.push_back(device_info);
      device_info->_phydev = group.physicalDevices[idev];
      vkGetPhysicalDeviceProperties(device_info->_phydev, &device_info->_devprops);
      device_info->_is_discrete = (device_info->_devprops.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
      deco::printf(
          yel,
          "    gouup<%d> gpu<%d:%s> is_discrete<%d>\n",
          igroup,
          device_info->_devprops.deviceID,
          device_info->_devprops.deviceName,
          int(device_info->_is_discrete));
    }
    igroup++;
  }

  /////////////////////////////////////////////////////////////////////////////

  res              = vkEnumeratePhysicalDevices(_instance, &_numgpus, nullptr);
  OrkAssert(_device_infos.size() == _numgpus);
  
  //std::vector<VkPhysicalDevice> phydevs(_numgpus);
  //vkEnumeratePhysicalDevices(_instance, &_numgpus, phydevs.data());

  deco::printf(yel, "vulkan::_init numgpus<%u>\n", _numgpus);
  for (auto device_info : _device_infos) {

    const auto& phy = device_info->_phydev;
    auto& dev_props = device_info->_devprops;
    auto& dev_feats = device_info->_devfeatures;
    auto& dev_memprops = device_info->_devmemprops;

    vkGetPhysicalDeviceProperties(phy, &dev_props);
    vkGetPhysicalDeviceFeatures(phy, &dev_feats);
    bool is_discrete = (dev_props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
    device_info->_maxWkgCountX    = dev_props.limits.maxComputeWorkGroupCount[0];
    device_info->_maxWkgCountY    = dev_props.limits.maxComputeWorkGroupCount[1];
    device_info->_maxWkgCountZ    = dev_props.limits.maxComputeWorkGroupCount[2];

    deco::printf(
        yel, "vulkan::_init gpu<%d:%s> is_discrete<%d>\n", dev_props.deviceID, dev_props.deviceName, int(is_discrete));
    deco::printf(yel, "         apiver<%u>\n", dev_props.apiVersion);
    deco::printf(yel, "         maxdim3d<%u>\n", dev_props.limits.maxImageDimension3D);
    deco::printf(yel, "         maxubrange<%u>\n", dev_props.limits.maxUniformBufferRange);
    deco::printf(yel, "         maxfbwidth<%u>\n", dev_props.limits.maxFramebufferWidth);
    deco::printf(yel, "         maxfblayers<%u>\n", dev_props.limits.maxFramebufferLayers);
    deco::printf(yel, "         maxcolorattachments<%u>\n", dev_props.limits.maxColorAttachments);
    deco::printf(yel, "         maxcomputeshmsize<%u>\n", dev_props.limits.maxComputeSharedMemorySize);
    deco::printf(yel, "         maxcomputewkgsize<%u>\n", dev_props.limits.maxComputeWorkGroupSize);
    deco::printf(yel, "         maxcomputewkgcount<%u,%u,%u>\n", device_info->_maxWkgCountX, device_info->_maxWkgCountY, device_info->_maxWkgCountZ);
    deco::printf(yel, "         feat.fragmentStoresAndAtomics<%u>\n", int(dev_feats.fragmentStoresAndAtomics));
    deco::printf(yel, "         feat.shaderFloat64<%u>\n", int(dev_feats.shaderFloat64));
    deco::printf(yel, "         feat.sparseBinding<%u>\n", int(dev_feats.sparseBinding));
    deco::printf(yel, "         feat.multiDrawIndirect<%u>\n", int(dev_feats.multiDrawIndirect));


    vkGetPhysicalDeviceMemoryProperties(phy, &dev_memprops);
    auto heaps = dev_memprops.memoryHeaps;
    std::vector<VkMemoryHeap> heapsvect(heaps, heaps + dev_memprops.memoryHeapCount);
    for (const auto& heap : heapsvect) {
      if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        // deco::printf(yel, "         heap.size<%zu>\n", heap.size);
      }
    }
    uint32_t numqfamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phy, &numqfamilies, nullptr);
    device_info->_queueprops.resize(numqfamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(phy, &numqfamilies, device_info->_queueprops.data());

    uint32_t numextensions = 0;
    vkEnumerateDeviceExtensionProperties(phy, nullptr, &numextensions, nullptr);
    device_info->_extensions.resize(numextensions);
    vkEnumerateDeviceExtensionProperties(phy, nullptr, &numextensions, device_info->_extensions.data());
    for (auto ext : device_info->_extensions) {
       deco::printf(yel,"         extension: <%s>\n", ext.extensionName);
       device_info->_extension_set.insert(ext.extensionName);
    }
  } // for(auto& phy : phydevs){

  for (int i = 0; i < 1; i++) {
    auto loadctx = std::make_shared<VkLoadContext>();
    //loadctx->_global_plato  = GlOsxPlatformObject::_global_plato;
    load_token_t token;
    token.setShared<VkLoadContext>(loadctx);
    _loadTokens.push(token);
  }

}

///////////////////////////////////////////////////////////////////////////////////////////////

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(_instance, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void touchClasses() {
  VkContext::GetClassStatic();
}

context_ptr_t createLoaderContext() {

	///////////////////////////////////////////////////////////
  auto loader = std::make_shared<FxShaderLoader>();
  FxShader::RegisterLoaders("shaders/fxv2/", "fxv2");
  auto shadctx = FileEnv::contextForUriProto("orkshader://");
  auto democtx = FileEnv::contextForUriProto("demo://");
  loader->addLocation(shadctx, ".fxv2"); // for glsl targets
  if( democtx ){
    loader->addLocation(democtx, ".fxv2"); // for glsl targets
  }
	///////////////////////////////////////////////////////////

  asset::registerLoader<FxShaderAsset>(loader);

  _GVI = std::make_shared<VulkanInstance>();
  auto clazz                   = dynamic_cast<object::ObjectClass*>(VkContext::GetClassStatic());
  GfxEnv::setContextClass(clazz);
  auto target = VkContext::makeShared();
  target->initializeLoaderContext();
  GfxEnv::initializeWithContext(target);
  return target;
}


///////////////////////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan

#endif
