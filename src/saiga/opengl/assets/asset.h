/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/core/camera/camera.h"
#include "saiga/core/geometry/aabb.h"
#include "saiga/core/geometry/triangle_mesh.h"
#include "saiga/core/model/all.h"
#include "saiga/core/model/animation.h"
#include "saiga/opengl/animation/boneVertex.h"
#include "saiga/opengl/indexedVertexBuffer.h"
#include "saiga/opengl/shader/basic_shaders.h"

#include <memory>

namespace Saiga
{
class SAIGA_OPENGL_API Asset
{
   public:
    virtual ~Asset() {}
    virtual void render(Camera* cam, const mat4& model)          = 0;
    virtual void renderForward(Camera* cam, const mat4& model)   = 0;
    virtual void renderDepth(Camera* cam, const mat4& model)     = 0;
    virtual void renderWireframe(Camera* cam, const mat4& model) = 0;
    virtual void renderRaw()                                     = 0;
};


template <typename ModelType, typename ShaderType>
class SAIGA_TEMPLATE BasicAsset : public Asset, public ModelType
{
   public:
    using VertexType = typename ModelType::VertexType;
    using IndexType  = typename ModelType::IndexType;

    std::shared_ptr<ShaderType> deferredShader;
    std::shared_ptr<ShaderType> forwardShader;
    std::shared_ptr<ShaderType> depthshader;
    std::shared_ptr<ShaderType> wireframeshader;

    IndexedVertexBuffer<VertexType, IndexType> buffer;

    /**
     * Use these for simple inefficient rendering.
     * Every call binds and unbinds the shader and uploads the camera matrices again.
     */

    virtual ~BasicAsset() {}
    virtual void render(Camera* cam, const mat4& model) override;
    virtual void renderForward(Camera* cam, const mat4& model) override;
    virtual void renderDepth(Camera* cam, const mat4& model) override;
    virtual void renderWireframe(Camera* cam, const mat4& model) override;

    /**
     * Renders the mesh.
     * This maps to a single glDraw call and nothing else, so the shader
     * has to be setup before this renderRaw is called.
     */
    virtual void renderRaw() override;


    virtual void loadDefaultShaders() = 0;

    void setShader(std::shared_ptr<ShaderType> deferredShader, std::shared_ptr<ShaderType> forwardShader,
                   std::shared_ptr<ShaderType> depthshader, std::shared_ptr<ShaderType> wireframeshader);
    void create();
};

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::render(Camera* cam, const mat4& model)
{
    (void)cam;
    deferredShader->bind();
    //    shader->uploadAll(cam,model);
    deferredShader->uploadModel(model);

    //    glEnable(GL_POLYGON_OFFSET_FILL);
    //    glPolygonOffset(1.0f,1.0f);

    buffer.bindAndDraw();

    //    glDisable(GL_POLYGON_OFFSET_FILL);

    deferredShader->unbind();
}

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::renderForward(Camera* cam, const mat4& model)
{
    (void)cam;
    forwardShader->bind();
    //    shader->uploadAll(cam,model);
    forwardShader->uploadModel(model);

    //    glEnable(GL_POLYGON_OFFSET_FILL);
    //    glPolygonOffset(1.0f,1.0f);

    buffer.bindAndDraw();

    //    glDisable(GL_POLYGON_OFFSET_FILL);

    forwardShader->unbind();
}

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::renderDepth(Camera* cam, const mat4& model)
{
    (void)cam;
    depthshader->bind();
    depthshader->uploadModel(model);
    buffer.bindAndDraw();
    depthshader->unbind();
}

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::renderWireframe(Camera* cam, const mat4& model)
{
    (void)cam;
    wireframeshader->bind();
    wireframeshader->uploadModel(model);

    //    glEnable(GL_POLYGON_OFFSET_LINE);

    // negative values shifts the wireframe towards the camera,
    // but a non zero factors does strange things for lines and increases
    // the depth on lines with high slope towards the camera by too much.
    // a visually better solution is to shift the triangles back a bit glPolygonOffset(1,1);
    // and draw the wireframe without polygon offset.
    //    glPolygonOffset(0.0f,-500.0f);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    buffer.bindAndDraw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //    glDisable(GL_POLYGON_OFFSET_LINE);

    wireframeshader->unbind();
}

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::renderRaw()
{
    buffer.bindAndDraw();
}

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::setShader(std::shared_ptr<ShaderType> _shader,
                                                  std::shared_ptr<ShaderType> _forwardShader,
                                                  std::shared_ptr<ShaderType> _depthshader,
                                                  std::shared_ptr<ShaderType> _wireframeshader)
{
    this->deferredShader  = _shader;
    this->forwardShader   = _forwardShader;
    this->depthshader     = _depthshader;
    this->wireframeshader = _wireframeshader;
}

template <typename ModelType, typename ShaderType>
void BasicAsset<ModelType, ShaderType>::create()
{
    if (!deferredShader)
    {
        loadDefaultShaders();
    }
    buffer.fromMesh(*this);
}

}  // namespace Saiga
