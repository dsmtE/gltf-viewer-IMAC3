#pragma once

#include "FrameBuffer.hpp"
#include "Texture.hpp"

class TextureFB : public FrameBuffer {

    public:
        TextureFB(const glm::ivec2& size, const GLint internalformat);
        ~TextureFB() = default;
        
        inline void bindTexture(const int slot) const { texture_.attachToSlot(slot); }

    protected:

        Texture texture_;
};
