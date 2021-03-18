#include "FrameBuffer.hpp"

#include "glDebug.hpp"

#include <iostream>

FrameBuffer::FrameBuffer(const glm::ivec2& size) : size_{size} {
    GLCALL(glGenFramebuffers(1, &id_));
}

FrameBuffer::~FrameBuffer() {
	GLCALL(glDeleteFramebuffers(1, &id_));
}

void FrameBuffer::bind() {
	GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, id_));
	GLCALL(glGetIntegerv(GL_VIEWPORT, saveViewportDim_)); // Store viewport settings to restore them when unbinding
	GLCALL(glViewport(0, 0, width(), height())); // Only usefull if we plan on using this frame buffer at a different resolution than the screen's
}

void FrameBuffer::unbind() {
	GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GLCALL(glViewport(saveViewportDim_[0], saveViewportDim_[1], saveViewportDim_[2], saveViewportDim_[3]));
}

void FrameBuffer::copyTo(const glm::ivec2& botLeft, const glm::ivec2& topRight, const GLbitfield& mask, const GLuint dstFrameBufferID, const GLint interpolationMode) {
	GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBufferID));
	GLCALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, id_));
	GLCALL(glBlitFramebuffer(0, 0, width(), height(), botLeft.x, botLeft.y, topRight.x, topRight.y, mask, interpolationMode));
	GLCALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}