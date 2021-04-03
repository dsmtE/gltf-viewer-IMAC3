#include "GBuffer.hpp"

#include "glEnumToStr.hpp"

GBuffer::GBuffer(const glm::ivec2& size) : FrameBuffer(size), 
positionTex_(size, GL_RGBA16F), normalTex_(size, GL_RGBA16F), albedoTex_(size, GL_RGBA8), occlusionRoughnessMetallicTex_(size, GL_RGBA16F),  emissiveTex_(size, GL_RGBA16F), depthTex_(size, GL_DEPTH_COMPONENT24) {
    bind();
    
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedoTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, occlusionRoughnessMetallicTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, emissiveTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex_.ID(), 0));

    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    const unsigned int attachments[5] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4};
    GLCALL(glDrawBuffers(5, attachments));

    // create and attach depth buffer (renderbuffer)
    // GLCALL(glGenRenderbuffers(1, &depthRenderBufferId_));
	// GLCALL(glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferId_));
	// GLCALL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height()));
    // GLCALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferId_));
	// GLCALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    // finally check if framebuffer is complete
    const GLenum status = GLCALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "Framebuffer is not complete : " << glEnumToStr::find(glEnumToStr::framebufferStatus, status) << std::endl;

    unbind();
    GLCALL(glGenVertexArrays(1, &screenVAO_));
}

GBuffer::~GBuffer() {
    // Textures will be destroyed properly within Texture class
    GLCALL(glDeleteRenderbuffers(1, &depthRenderBufferId_));
    GLCALL(glDeleteVertexArrays(1, &screenVAO_));
}

void GBuffer::bindTextures() const {
    positionTex_.attachToSlot(0);
    normalTex_.attachToSlot(1);
    albedoTex_.attachToSlot(2);
    occlusionRoughnessMetallicTex_.attachToSlot(3);
    emissiveTex_.attachToSlot(4);
}

void GBuffer::bindDepthTextures(const int slot) const {
    depthTex_.attachToSlot(slot);
}

void GBuffer::bindTexturesToShader(GLProgram& shaderProgram) const {
    positionTex_.attachToShaderSlot(shaderProgram, "uGPosition", 0);
    normalTex_.attachToShaderSlot(shaderProgram, "uGNormal", 1);
    albedoTex_.attachToShaderSlot(shaderProgram, "uGAlbedo", 2);
    occlusionRoughnessMetallicTex_.attachToShaderSlot(shaderProgram, "uGOcclusionRoughnessMetallic", 3);
    emissiveTex_.attachToShaderSlot(shaderProgram, "uGEmissive", 4);
}

void GBuffer::render() const {
    GLCALL(glBindVertexArray(screenVAO_));
    GLCALL(glDrawArrays(GL_TRIANGLES, 0, 3));
    GLCALL(glBindVertexArray(0));
}