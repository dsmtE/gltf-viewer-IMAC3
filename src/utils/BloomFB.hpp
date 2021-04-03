#pragma once

#include "FrameBuffer.hpp"
#include "Texture.hpp"

#include <functional>

class BloomFB : public FrameBuffer {

    public:
        BloomFB(const glm::ivec2& size);
        ~BloomFB() = default;
        
        void bindScene(const GLenum target = GL_FRAMEBUFFER);
        void bindLights(const GLenum target = GL_FRAMEBUFFER);
        
        void bindSceneTexture(const int slot) const;

        void setUniforms(GLProgram& shaderProgram) const;
        void imguiMenu();

        inline int quality() const { return quality_; }
        inline float threshold() const { return threshold_; }
    private:

        Texture sceneTex_;
        Texture lightsTex_;
        
        bool use_;
        int quality_;
        glm::vec3 tint_;
        float intensity_;
        float threshold_;

};
