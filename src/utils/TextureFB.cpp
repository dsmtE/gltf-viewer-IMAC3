#include "TextureFB.hpp"

#include "glDebug.hpp"

TextureFB::TextureFB(const glm::ivec2& size, const GLint internalformat) : FrameBuffer(size), texture_(size, internalformat) {
    bind();
    
    texture_.setup();
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_.ID(), 0));

    const GLenum status = GLCALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "SSAO Framebuffer is not complete : " << glEnumToStr::find(glEnumToStr::framebufferStatus, status) << std::endl;

    unbind();
}

