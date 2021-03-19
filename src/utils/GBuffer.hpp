#pragma once

#include "FrameBuffer.hpp"
#include "Texture.hpp"

class GBuffer : public FrameBuffer {

    public:
        GBuffer(const glm::ivec2& size);
        ~GBuffer();
        
        void bindTextures() const;
        void bindTexturesToShader(GLProgram& shaderProgram) const;
        void render() const;

    private:

        Texture positionTex_;
        Texture normalTex_;
        Texture albedoTex_;
        Texture occlusionRoughnessMetallicTex_;
        Texture emissiveTex_;
        
        GLuint depthRenderBufferId_;

        // Rendering triangle covering the whole screen, for the shading pass:
        GLuint screenVAO_ = -1;
};
