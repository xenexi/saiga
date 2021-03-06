/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "saiga/opengl/rendering/deferredRendering/deferredRendering.h"
#include "saiga/opengl/vertexBuffer.h"

namespace Saiga
{
GBuffer::GBuffer() {}

GBuffer::GBuffer(int w, int h, GBufferParameters params)
{
    init(w, h, params);
}

void GBuffer::init(int w, int h, GBufferParameters params)
{
    this->params = params;
    this->create();

    std::shared_ptr<Texture> color = std::make_shared<Texture>();

    switch (params.colorQuality)
    {
        case Quality::LOW:
            color->create(w, h, GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
            break;
        case Quality::MEDIUM:
            color->create(w, h, GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
            break;
        case Quality::HIGH:
            color->create(w, h, GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
            break;
    }

    //    attachTexture(color);
    attachTexture(framebuffer_texture_t(color));


    std::shared_ptr<Texture> normal = std::make_shared<Texture>();
    //    multisampled_Texture_2D* normal = new multisampled_Texture_2D(samples);
    switch (params.normalQuality)
    {
        case Quality::LOW:
            normal->create(w, h, GL_RG, GL_RG8, GL_UNSIGNED_BYTE);
            break;
        case Quality::MEDIUM:
            normal->create(w, h, GL_RG, GL_RG16, GL_UNSIGNED_SHORT);
            break;
        case Quality::HIGH:
            normal->create(w, h, GL_RG, GL_RG16, GL_UNSIGNED_SHORT);
            break;
    }
    attachTexture(framebuffer_texture_t(normal));

    // specular and emissive texture
    std::shared_ptr<Texture> data = std::make_shared<Texture>();
    //    multisampled_Texture_2D* data = new multisampled_Texture_2D(samples);
    switch (params.dataQuality)
    {
        case Quality::LOW:
            data->create(w, h, GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
            break;
        case Quality::MEDIUM:
            data->create(w, h, GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
            break;
        case Quality::HIGH:
            data->create(w, h, GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
            break;
    }
    attachTexture(framebuffer_texture_t(data));


    //    std::shared_ptr<Texture> depth = new Texture();
    std::shared_ptr<Texture> depth_stencil = std::make_shared<Texture>();
    depth_stencil->create(w, h, GL_DEPTH_STENCIL, GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8);
    attachTextureDepthStencil(framebuffer_texture_t(depth_stencil));

    drawToAll();

    check();
    unbind();
}

void GBuffer::sampleNearest()
{
    depthBuffer->setFiltering(GL_NEAREST);
    for (auto t : colorBuffers)
    {
        t->setFiltering(GL_NEAREST);
    }
}

void GBuffer::sampleLinear()
{
    depthBuffer->setFiltering(GL_LINEAR);
    for (auto t : colorBuffers)
    {
        t->setFiltering(GL_LINEAR);
    }
}

void GBuffer::clampToEdge()
{
    depthBuffer->setWrap(GL_CLAMP);
    for (auto t : colorBuffers)
    {
        t->setWrap(GL_CLAMP);
    }
}

}  // namespace Saiga
