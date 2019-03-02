﻿/*
 * Vulkan Example - imGui (https://github.com/ocornut/imgui)
 *
 * Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "saiga/core/sdl/sdl_camera.h"
#include "saiga/core/sdl/sdl_eventhandler.h"
#include "saiga/core/window/Interfaces.h"
#include "saiga/vulkan/VulkanForwardRenderer.h"
#include "saiga/vulkan/memory/BufferChunkAllocator.h"
#include "saiga/vulkan/memory/ChunkCreator.h"
#include "saiga/vulkan/memory/VulkanMemory.h"
#include "saiga/vulkan/renderModules/AssetRenderer.h"
#include "saiga/vulkan/renderModules/LineAssetRenderer.h"
#include "saiga/vulkan/renderModules/PointCloudRenderer.h"
#include "saiga/vulkan/renderModules/TextureDisplay.h"
#include "saiga/vulkan/renderModules/TexturedAssetRenderer.h"

#include <random>
#include <utility>
#include <vector>

using Saiga::Vulkan::Memory::MemoryLocation;
class VulkanExample : public Saiga::Updating,
                      public Saiga::Vulkan::VulkanForwardRenderingInterface,
                      public Saiga::SDL_KeyListener
{
    std::vector<std::pair<std::shared_ptr<Saiga::Vulkan::Buffer>, uint32_t>> allocations;
    std::vector<std::pair<std::shared_ptr<Saiga::Vulkan::Buffer>, uint32_t>> num_allocations;
    std::mt19937 mersenne_twister, auto_mersenne;

    std::array<vk::DeviceSize, 4> sizes{256 * 256, 512 * 512, 1024 * 1024, 16 * 1024 * 1024};

    Saiga::Vulkan::Memory::BufferType buffer_type{vk::BufferUsageFlagBits::eVertexBuffer,
                                                  vk::MemoryPropertyFlagBits::eDeviceLocal};

    bool enable_auto_index = false;
    bool enable_defragger  = false;
    int auto_allocs        = 0;

   public:
    VulkanExample(Saiga::Vulkan::VulkanWindow& window, Saiga::Vulkan::VulkanForwardRenderer& renderer);
    ~VulkanExample() override;

    void init(Saiga::Vulkan::VulkanBase& base);


    void update(float dt) override;
    void transfer(vk::CommandBuffer cmd) override;
    void render(vk::CommandBuffer cmd) override;
    void renderGUI() override;

   private:
    Saiga::SDLCamera<Saiga::PerspectiveCamera> camera;



    Saiga::Vulkan::VulkanForwardRenderer& renderer;

    bool displayModels = true;


    void keyPressed(SDL_Keysym key) override;
    void keyReleased(SDL_Keysym key) override;

    void alloc_index(int index);

    std::pair<std::shared_ptr<Saiga::Vulkan::Buffer>, uint32_t> allocate(Saiga::Vulkan::Memory::BufferType type,
                                                                         unsigned long long int size);
};
