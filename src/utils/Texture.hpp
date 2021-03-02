#pragma once

#include "glfw.hpp"
#include <array>

class Texture {
private:
	GLuint textureId_ = -1;

#ifndef NDEBUG
	bool dataUploaded_ = false;
#endif

public:
	Texture() = default;
	~Texture();
	Texture(const Texture&) = delete;
	Texture(Texture&&) = delete;
	Texture& operator=(const Texture&) = delete;

	// Creates an OpenGL texture. You are then responsible for destroying it.
    // The OpenGL ID of the texture.
    static GLuint CreateTextureID(const GLint minFilter  = GL_LINEAR, const GLint magFilter = GL_LINEAR, const std::array<GLint, 3>& wrapsMode = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE});
    static inline GLuint CreateTextureID(const GLint minFilter  = GL_LINEAR, const GLint magFilter = GL_LINEAR, GLint wrapMode = GL_CLAMP_TO_EDGE) {
        return CreateTextureID(minFilter, magFilter, {wrapMode, wrapMode, wrapMode});
    }
        
	// Actually constructs the OpenGL texture.
	// We don't do this in the constructor to allow the use of static textures (those would fail because OpenGL is only initialized once AppManager has been constructed)
    void genTexture(const GLint minFilter  = GL_LINEAR, const GLint magFilter = GL_LINEAR, const std::array<GLint, 3>& wrapSMode = {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE});

    inline void genTexture(const GLint minFilter  = GL_LINEAR, const GLint magFilter = GL_LINEAR, const GLint wrapMode = GL_CLAMP_TO_EDGE) {
        genTexture(minFilter, magFilter, {wrapMode, wrapMode, wrapMode});
    }

	// Upload the image data on the GPU (or just the size if you plan on writting on the texture through shaders).
	// An array of unsigned chars. You can leave as nullptr if you just want to set the size of the texture. 3 consecutive unsigned chars make a pixel (Red Green Blue).
	// The OpenGL convention is that the first pixel in the array is the bottom-left of the image, then the second is on the first row second column and so on.
	template <typename T>
	void upload(const int width, const int height, const GLint internalformat, const GLenum format, const GLenum type, const T* data) {
#ifndef NDEBUG
		if (textureId_ == -1) {
			std::cerr << "[Texture::upload] You haven't generated that texture yet !" << std::endl;
			return;
		}
		dataUploaded_ = true;
#endif
		glBindTexture(GL_TEXTURE_2D, textureId_);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Attaches your texture to a slot, so that it is ready to be read by a shader.
	// This should match the "uniform sampler2D u_TextureSlot" in your shader that is set through setUniform1i(slot)
	void attachToSlot(const int slot = 0) const;

	inline GLuint ID() const { return textureId_; }
};


