#include "Texture.hpp"

#include <iostream>
#include <algorithm>

Texture::~Texture() {
	glDeleteTextures(1, &textureId_);
}

GLuint Texture::CreateTextureID(const GLint minFilter, const GLint magFilter, const std::array<GLint, 3>& wrapsMode) {
    GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapsMode[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapsMode[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, wrapsMode[2]);

    const std::array<GLint, 4> mipmapMode = {
        GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, 
        GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR
    };
    if (std::any_of(mipmapMode.begin(), mipmapMode.end(), [&](GLint i){return i == minFilter;})) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
	glBindTexture(GL_TEXTURE_2D, 0);
	return textureId;
}

void Texture::genTexture(const GLint minFilter, const GLint magFilter, const std::array<GLint, 3>&  wrapsMode) {
#ifndef NDEBUG
	if (textureId_ != -1){
		std::cerr << "[Texture::genTexture] You have already generated that texture !" << std::endl;
        return;
    }
#endif
    textureId_ = CreateTextureID(minFilter, magFilter, wrapsMode);
}

void Texture::attachToSlot(const int slot) const {
#ifndef NDEBUG
	if (textureId_ == -1)
		std::cerr << "[Texture::attachToSlot] You haven't generated that texture yet !" << std::endl;
	if (!dataUploaded_)
		std::cerr << "[Texture::attachToSlot] You must upload some data (at least a width and height) before using the texture." << std::endl;
#endif
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureId_);
}
