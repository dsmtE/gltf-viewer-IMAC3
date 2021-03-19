#pragma once

#include "glDebug.hpp"
#include "shaders.hpp"
#include "glfw.hpp"

#include <array>
#include <iostream>

// forward declaration of tinygltf sampler
namespace tinygltf {
    struct Sampler;
}

class Texture {

public:
	Texture(const glm::ivec2& size, const GLint internalformat) : size_(size) {

		const int32_t& width = size_.x;
    	const int32_t& height = size_.y;

		GLCALL(glGenTextures(1, &textureId_));
		GLCALL(glBindTexture(GL_TEXTURE_2D, textureId_));
		GLCALL(glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, width, height));
		GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
	}

	~Texture() { glDeleteTextures(1, &textureId_); }
	Texture(const Texture&) = delete;
	Texture(Texture&& o) {
		textureId_ = o.textureId_;
		o.textureId_ = -1;

		size_ = o.size_;
		o.size_ = {0, 0};
	}

	Texture& operator=(const Texture&) = delete;

	template <typename T>
	void upload(const GLenum format, const GLenum type, const T* data) {
		GLCALL(glBindTexture(GL_TEXTURE_2D, textureId_));
		GLCALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size_.x, size_.y, format, type, data));
		GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
	}

	template <typename T>
	void upload(const GLenum format, const GLenum type, const T* data, 
	GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, const std::array<GLint, 3>& wrapsMode = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}) {
		
		setup(minFilter, magFilter, wrapsMode);
		
		minFilter = minFilter != -1 ? minFilter : GL_LINEAR;
		magFilter = magFilter != -1 ? magFilter : GL_LINEAR;

		GLCALL(glBindTexture(GL_TEXTURE_2D, textureId_));

		GLCALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size_.x, size_.y, format, type, data));

		if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_LINEAR) {
			GLCALL(glGenerateMipmap(GL_TEXTURE_2D));
		}

		GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
	}

	void setup(GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR, const std::array<GLint, 3>& wrapsMode = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}) {
		minFilter = minFilter != -1 ? minFilter : GL_LINEAR;
		magFilter = magFilter != -1 ? magFilter : GL_LINEAR;

		GLCALL(glBindTexture(GL_TEXTURE_2D, textureId_));

		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapsMode[0]));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapsMode[1]));
		GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapsMode[2]));

		GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
	}


	template <typename T>
	inline void upload(const GLenum format, const GLenum type, const T* data, const tinygltf::Sampler& sampler) {
		upload(format, type, data, sampler.minFilter, sampler.magFilter, {sampler.wrapS, sampler.wrapT, sampler.wrapR});
	}

	// Attaches your texture to a slot, so that it is ready to be read by a shader.
	// This should match the "uniform sampler2D u_TextureSlot" in your shader that is set through setUniform1i(slot)
	void attachToSlot(const int slot = 0) const {
		GLCALL(glActiveTexture(GL_TEXTURE0 + slot));
		GLCALL(glBindTexture(GL_TEXTURE_2D, textureId_));
	}

	void attachToShaderSlot(GLProgram& shaderProgram, const char* name, const int slot = 0) const {
		GLCALL(glActiveTexture(GL_TEXTURE0 + slot));
		GLCALL(glBindTexture(GL_TEXTURE_2D, textureId_));
		shaderProgram.setInt(name, slot);
	}

	inline GLuint ID() const { return textureId_; }

private:
	GLuint textureId_ = -1;
	glm::ivec2 size_ = {0, 0};
};


