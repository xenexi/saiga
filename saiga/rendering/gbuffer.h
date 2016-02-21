#pragma once

#include <memory>

#include "saiga/util/quality.h"
#include "saiga/opengl/framebuffer.h"


struct SAIGA_GLOBAL GBufferParameters{
    bool srgb = false; //colors stored in srgb. saves memory bandwith but adds conversion operations.
    Quality colorQuality = Quality::MEDIUM;
    Quality normalQuality = Quality::MEDIUM;
    Quality dataQuality = Quality::LOW;
    Quality depthQuality = Quality::HIGH;
};

class SAIGA_GLOBAL GBuffer : public Framebuffer{
protected:
    GBufferParameters params;
public:
    GBuffer();
    GBuffer(int w, int h, GBufferParameters params);
    void init(int w, int h, GBufferParameters params);

    Texture* getTextureColor(){return this->colorBuffers[0];}
    Texture* getTextureNormal(){return this->colorBuffers[1];}
    Texture* getTextureData(){return this->colorBuffers[2];}
};
