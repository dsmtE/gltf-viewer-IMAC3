#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

class FrameBuffer {
public:
	FrameBuffer(const glm::ivec2& size);
	virtual ~FrameBuffer();

	void bind();
	void unbind();

	// Copies the content of this framebuffer to another framebuffer (dstFrameBufferID = 0 by default for screen)
	void copyTo(const glm::ivec2& botLeft, const glm::ivec2& topRight, const GLbitfield& mask, const GLuint dstFrameBufferID = 0, const GLint interpolationMode = GL_LINEAR);
	inline void copyTo(const FrameBuffer& frameBuffer, const GLbitfield& mask, GLint interpolationMode = GL_LINEAR) {
        copyTo({0, 0}, frameBuffer.size(), mask, frameBuffer.id_, interpolationMode);
    }

	inline int width() const { return size_.x; }
	inline int height() const { return size_.y; }
	inline const glm::ivec2& size() const { return size_; }
	inline float aspectRatio() const { return static_cast<float>(size_.x) / static_cast<float>(size_.y); }
	inline GLuint Id() const { return id_; }
	
protected:
	
	GLuint id_ = -1;
	glm::ivec2 size_ = glm::ivec2(0);
	int saveViewportDim_[4];
};
