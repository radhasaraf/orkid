#include "vulkan_ctx.h"

///////////////////////////////////////////////////////////////////////////////
namespace ork::lev2::vulkan {
///////////////////////////////////////////////////////////////////////////////

VkDrawingInterface::VkDrawingInterface(vkcontext_rawptr_t ctx)
    : DrawingInterface(*ctx)
    , _contextVK(ctx) {
}

///////////////////////////////////////////////////////////////////////////////

VkImiInterface::VkImiInterface(vkcontext_rawptr_t ctx)
    : ImmInterface(*ctx)
    , _contextVK(ctx) {
}

///////////////////////////////////////////////////////////////////////////////

void VkImiInterface::_doBeginFrame() {
}

///////////////////////////////////////////////////////////////////////////////

void VkImiInterface::_doEndFrame() {
}

///////////////////////////////////////////////////////////////////////////////
} // namespace ork::lev2::vulkan
///////////////////////////////////////////////////////////////////////////////
