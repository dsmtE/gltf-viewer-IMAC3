#pragma once

#include "glfw.hpp"
#include <array>

// forward declaration of tinygltf sampler
namespace tinygltf {
    struct Sampler;
}

class Texture {
private:
	GLuint textureId_ = -1;

#ifndef NDEBUG
	bool initialized_ = false;
#endif

public:
	Texture() {
		glGenTextures(1, &textureId_);
	}
	~Texture() {
		glDeleteTextures(1, &textureId_);
	}
	Texture(const Texture&) = delete;
	Texture(Texture&&) = delete;
	Texture& operator=(const Texture&) = delete;

	template <typename T>
	void init(const int width, const int height, const GLint internalformat, const GLenum format, const GLenum type, const T* data, 
	const GLint minFilter = GL_LINEAR, const GLint magFilter = GL_LINEAR, const std::array<GLint, 3>& wrapsMode = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}) {
		
		glBindTexture(GL_TEXTURE_2D, textureId_);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapsMode[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapsMode[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapsMode[2]);

		if (minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			minFilter == GL_LINEAR_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_LINEAR) {
			glGenerateMipmap(GL_TEXTURE_2D);
		}

#ifndef NDEBUG
	bool initialized_ = true;
#endif

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	template <typename T>
	inline void init(const int width, const int height, const GLint internalformat, const GLenum format, const GLenum type, const T* data, const tinygltf::Sampler& sampler) {
		init(width, height, internalformat, format, type, data, sampler.minFilter, sampler.magFilter, {sampler.wrapS, sampler.wrapT, sampler.wrapR});
	}

	// Attaches your texture to a slot, so that it is ready to be read by a shader.
	// This should match the "uniform sampler2D u_TextureSlot" in your shader that is set through setUniform1i(slot)
	void attachToSlot(const int slot = 0) const {
#ifndef NDEBUG
		if (!initialized_)
			std::cerr << "[Texture::attachToSlot] You must initialize your texture before using it." << std::endl;
#endif
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, textureId_);
	}


	inline GLuint ID() const { return textureId_; }
};


