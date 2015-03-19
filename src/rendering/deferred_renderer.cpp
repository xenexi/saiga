#include "rendering/deferred_renderer.h"
#include "libhello/util/error.h"
void PostProcessingShader::checkUniforms(){
    Shader::checkUniforms();
    location_texture = Shader::getUniformLocation("image");
    location_screenSize = Shader::getUniformLocation("screenSize");
}


void PostProcessingShader::uploadTexture(raw_Texture *texture){
    texture->bind(0);
    Shader::upload(location_texture,0);
}

void PostProcessingShader::uploadScreenSize(vec4 size){
    Shader::upload(location_screenSize,size);
}


void Deferred_Renderer::init(DeferredShader* deferred_shader, int w, int h){
    setSize(w,h);
    lighting.setSize(w,h);
    deferred_framebuffer.create();
    deferred_framebuffer.makeToDeferredFramebuffer(w,h);

    mix_framebuffer.create();

    Texture* depth_stencil = new Texture();
    depth_stencil->createEmptyTexture(w,h,GL_DEPTH_STENCIL, GL_DEPTH24_STENCIL8,GL_UNSIGNED_INT_24_8);
    mix_framebuffer.attachTextureDepthStencil(depth_stencil);

    Texture* ppsrc = new Texture();
//    ppsrc->createEmptyTexture(w,h,GL_RGBA,GL_SRGB8_ALPHA8,GL_UNSIGNED_BYTE);
    ppsrc->createEmptyTexture(w,h,GL_RGBA,GL_RGBA8,GL_UNSIGNED_BYTE);
    mix_framebuffer.attachTexture(ppsrc);

    glDrawBuffer( GL_COLOR_ATTACHMENT0);

    mix_framebuffer.check();
    mix_framebuffer.unbind();


    postProcess_framebuffer.create();
    Texture* ppdst = new Texture();
    ppdst->createEmptyTexture(w,h,GL_RGBA,GL_RGBA8,GL_UNSIGNED_BYTE);
//     ppdst->createEmptyTexture(w,h,GL_RGBA,GL_SRGB8_ALPHA8,GL_UNSIGNED_BYTE);
    postProcess_framebuffer.attachTexture(ppdst);

    glDrawBuffer( GL_COLOR_ATTACHMENT0);

    postProcess_framebuffer.check();
    postProcess_framebuffer.unbind();


    initCudaPostProcessing(ppsrc,ppdst);


    setDeferredMixer(deferred_shader);


    auto qb = TriangleMeshGenerator::createFullScreenQuadMesh();
    qb->createBuffers(quadMesh);
}

void Deferred_Renderer::setDeferredMixer(DeferredShader* deferred_shader){
    this->deferred_shader = deferred_shader;
}

void Deferred_Renderer::render_intern(float interpolation){

    glViewport(0,0,width,height);
    glClear( GL_COLOR_BUFFER_BIT );
    glClear(GL_DEPTH_BUFFER_BIT);




    renderGBuffer(*currentCamera, interpolation);


    renderDepthMaps(*currentCamera);

    glDisable(GL_DEPTH_TEST);
    glViewport(0,0,width,height);

	Error::quitWhenError("Deferred_Renderer::before blit");

    //copy depth to lighting framebuffer. that is needed for stencil culling
    deferred_framebuffer.blitDepth(mix_framebuffer.id);

	Error::quitWhenError("Deferred_Renderer::after blit");

//    glEnable(GL_FRAMEBUFFER_SRGB);

    mix_framebuffer.bind();
    glClear( GL_COLOR_BUFFER_BIT );

    renderLighting(*currentCamera);


    renderOverlay(*currentCamera, interpolation);
    mix_framebuffer.unbind();

//    glDisable(GL_FRAMEBUFFER_SRGB);

    if(postProcessing)
        postProcess();
    else{
//        postProcess();
        mix_framebuffer.blitColor(0);
    }



    Error::quitWhenError("Deferred_Renderer::render_intern");

}

void Deferred_Renderer::renderGBuffer(Camera *cam, float interpolation){
    deferred_framebuffer.bind();
    glViewport(0,0,width,height);
    glClear( GL_COLOR_BUFFER_BIT );
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    if(wireframe){
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        glLineWidth(wireframeLineSize);
    }
    render(cam, interpolation);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );



    deferred_framebuffer.unbind();


    Error::quitWhenError("Deferred_Renderer::renderGBuffer");

}

void Deferred_Renderer::renderDepthMaps(Camera *cam){
    lighting.renderDepthMaps(this);

     Error::quitWhenError("Deferred_Renderer::renderDepthMaps");

}

void Deferred_Renderer::renderLighting(Camera *cam){
    glDepthMask(GL_FALSE);

    mat4 model;
    cam->getModelMatrix(model);
    lighting.setViewProj(model,cam->view,cam->proj);
    lighting.render(cam);
    glDisable(GL_BLEND);
    Error::quitWhenError("Deferred_Renderer::renderLighting");
}

void Deferred_Renderer::postProcess(){

    //shader post process + gamma correction
    glEnable(GL_FRAMEBUFFER_SRGB);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    postProcess_framebuffer.bind();
//    glClear( GL_COLOR_BUFFER_BIT );
    postProcessingShader->bind();

    vec4 screenSize(width,height,1.0/width,1.0/height);
    postProcessingShader->uploadScreenSize(screenSize);
    postProcessingShader->uploadTexture(mix_framebuffer.colorBuffers[0]);
    quadMesh.bindAndDraw();
    postProcessingShader->unbind();

//    postProcess_framebuffer.unbind();

    glDisable(GL_FRAMEBUFFER_SRGB);

//    postProcess_framebuffer.blitColor(0);

//    cudaPostProcessing();
//    postProcess_framebuffer.blitColor(0);

//     Error::quitWhenError("Deferred_Renderer::postProcess");
}

