﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */


#pragma once

#include "saiga/geometry/triangle_mesh.h"
#include "saiga/vulkan/svulkan.h"
#include "saiga/vulkan/Base.h"
#include "saiga/vulkan/VulkanBuffer.hpp"
#include "saiga/vulkan/VulkanAsset.h"
#include "saiga/vulkan/pipeline/Pipeline.h"
#include "saiga/vulkan/texture/Texture.h"
#include "saiga/vulkan/buffer/UniformBuffer.h"

namespace Saiga {
namespace Vulkan {



class SAIGA_GLOBAL TextureDisplay : public Pipeline
{
public:
    using VertexType = VertexNC;



    Saiga::Vulkan::VulkanVertexColoredAsset blitMesh;

    ~TextureDisplay() { destroy(); }
    void destroy();

    void bind(vk::CommandBuffer cmd);


    void bindTexture(vk::CommandBuffer cmd, vk::DescriptorSet ds);

    void pushModel(vk::CommandBuffer cmd, mat4 model);

    void init(Saiga::Vulkan::VulkanBase& vulkanDevice, VkRenderPass renderPass);

    void prepareUniformBuffers(Saiga::Vulkan::VulkanBase* vulkanDevice);
    void setupLayoutsAndDescriptors();

    vk::DescriptorSet createAndUpdateDescriptorSet( Texture& texture );
};



}
}