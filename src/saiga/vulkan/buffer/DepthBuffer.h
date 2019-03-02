﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */


#pragma once

#include "saiga/vulkan/svulkan.h"

#include "Buffer.h"

namespace Saiga
{
namespace Vulkan
{
class SAIGA_VULKAN_API DepthBuffer
{
   public:
    DepthBuffer(vk::Format format = vk::Format::eD16Unorm) : format(format) {}
    ~DepthBuffer() { destroy(); }

    // depth image
    vk::Format format;
    vk::Image depthimage = nullptr;
    vk::ImageView depthview;

    void init(Saiga::Vulkan::VulkanBase& base, int width, int height);
    void destroy();

   private:
    VulkanBase* base;
    Memory::MemoryLocation* location;
};

}  // namespace Vulkan
}  // namespace Saiga
