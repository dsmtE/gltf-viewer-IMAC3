#include "BlurFB.hpp"

#include "glDebug.hpp"

BlurFB::BlurFB(const glm::ivec2& size, const GLint internalformat) : FrameBuffer(size), pingTex_(size, internalformat), pongTex_(size, internalformat) {
    bind();
    
    pingTex_.setup();
    pongTex_.setup();
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, pongTex_.ID(), 0));

    // finally check if framebuffer is complete
    const GLenum status = GLCALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "Framebuffer is not complete : " << glEnumToStr::find(glEnumToStr::framebufferStatus, status) << std::endl;

    unbind();
}


void BlurFB::bindPing(const GLenum target) {
    bind(target);
    GLCALL(glDrawBuffer(GL_COLOR_ATTACHMENT0));
}

void BlurFB::bindPong(const GLenum target) {
    bind(target);
    GLCALL(glDrawBuffer(GL_COLOR_ATTACHMENT1));
}

void BlurFB::bindPingTexToSlot(const int slot) const {
    pingTex_.attachToSlot(slot);
}

void BlurFB::bindPongTexToSlot(const int slot) const {
    pongTex_.attachToSlot(slot);
}