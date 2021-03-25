#include "FrameBuffer.hpp"

#include "glDebug.hpp"

#include <iostream>

FrameBuffer::FrameBuffer(const glm::ivec2& size) : size_{size} {
    GLCALL(glGenFramebuffers(1, &id_));
}

FrameBuffer::~FrameBuffer() {
	GLCALL(glDeleteFramebuffers(1, &id_));
}

void FrameBuffer::bind(const GLenum target) {
	GLCALL(glBindFramebuffer(target, id_));
	GLCALL(glGetIntegerv(GL_VIEWPORT, saveViewportDim_)); // Store viewport settings to restore them when unbinding
	GLCALL(glViewport(0, 0, width(), height())); // Only usefull if we plan on using this frame buffer at a different resolution than the screen's
}

void FrameBuffer::unbind(const GLenum target) {
	GLCALL(glBindFramebuffer(target, 0));
	GLCALL(glViewport(saveViewportDim_[0], saveViewportDim_[1], saveViewportDim_[2], saveViewportDim_[3]));
}

void FrameBuffer::copyTo(const glm::ivec2& botLeft, const glm::ivec2& topRight, const GLbitfield& mask, const GLuint dstFrameBufferID, const GLint interpolationMode) {
	bind(GL_READ_FRAMEBUFFER);
	GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBufferID));
	GLCALL(glBlitFramebuffer(0, 0, width(), height(), botLeft.x, botLeft.y, topRight.x, topRight.y, mask, interpolationMode));
	unbind(GL_READ_FRAMEBUFFER);
}

void FrameBuffer::copyToFromSlot(const glm::ivec2& botLeft, const glm::ivec2& topRight, const GLbitfield& mask, const int slot, const GLuint dstFrameBufferID, const GLint interpolationMode) {
	bind(GL_READ_FRAMEBUFFER);
	GLCALL(glReadBuffer(GL_COLOR_ATTACHMENT0 + slot));
	GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBufferID));
	GLCALL(glBlitFramebuffer(0, 0, width(), height(), botLeft.x, botLeft.y, topRight.x, topRight.y, mask, interpolationMode));
	unbind(GL_READ_FRAMEBUFFER);
}
