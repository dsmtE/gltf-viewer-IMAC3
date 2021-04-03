#pragma once

#include "FrameBuffer.hpp"
#include "Texture.hpp"

class BlurFB : public FrameBuffer {

    public:
        BlurFB(const glm::ivec2& size, const GLint internalformat);
        ~BlurFB() = default;
        
        void bindPing(const GLenum target = GL_FRAMEBUFFER);
        void bindPong(const GLenum target = GL_FRAMEBUFFER);
        void bindPingTexToSlot(const int slot = 0) const;
        void bindPongTexToSlot(const int slot = 0) const;

    private:

        Texture pingTex_;
        Texture pongTex_;

        // fixel kernel size inside the shader
        // int kernelSize;
};
