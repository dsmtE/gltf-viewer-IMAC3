#include "FrameBuffer.hpp"

#include <iostream>

FrameBuffer::FrameBuffer(const glm::ivec2& size) : size_{size_} {
    glGenFramebuffers(1, &id_);
}

FrameBuffer::~FrameBuffer() {
	glDeleteFramebuffers(1, &id_);
}

char* FrameBuffer::GLFramebufferStatusToString(GLenum status) {
	char* statusStr = nullptr;
	switch (status) {
	case GL_FRAMEBUFFER_UNDEFINED:
		statusStr = "GL_FRAMEBUFFER_UNDEFINED";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		statusStr = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		statusStr = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		statusStr = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER ";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		statusStr = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER ";
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		statusStr = "GL_FRAMEBUFFER_UNSUPPORTED ";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		statusStr = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE ";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		statusStr = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS ";
		break;
	default:
		statusStr = "UNKNOWN_ERROR";
		break;
	}
	return statusStr;
}

void FrameBuffer::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, id_);
	//glGetIntegerv(GL_VIEWPORT, prevViewport_); // Store viewport settings to restore them when unbinding
	//glViewport(0, 0, width(), height()); // Only usefull if we plan on using this frame buffer at a different resolution than the screen's
}

void FrameBuffer::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glViewport(prevViewport_[0], prevViewport_[1], prevViewport_[2], prevViewport_[3]);
}

void FrameBuffer::copyTo(const glm::ivec2& botLeft, const glm::ivec2& topRight, const GLbitfield& mask, const GLuint dstFrameBufferID, const GLint interpolationMode) {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBufferID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, id_);
	glBlitFramebuffer(0, 0, width(), height(), botLeft.x, botLeft.y, topRight.x, topRight.y, mask, interpolationMode);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}