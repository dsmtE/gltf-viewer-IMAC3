#include "BloomFB.hpp"

#include "glDebug.hpp"

#include <imgui.h>

BloomFB::BloomFB(const glm::ivec2& size) : FrameBuffer(size), sceneTex_(size, GL_RGBA16F), lightsTex_(size, GL_RGBA16F), use_(true), quality_(5), intensity_(1.f), tint_(1.f), threshold_(0.66f) {
    
    bind();

    sceneTex_.setup();
    lightsTex_.setup();
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex_.ID(), 0));
    GLCALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightsTex_.ID(), 0));
    
    // finally check if framebuffer is complete
    const GLenum status = GLCALL(glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "Framebuffer is not complete : " << glEnumToStr::find(glEnumToStr::framebufferStatus, status) << std::endl;

    unbind();
}

void BloomFB::bindScene(const GLenum target) {
    bind(target);
    GLCALL(glDrawBuffer(GL_COLOR_ATTACHMENT0));
}

void BloomFB::bindLights(const GLenum target) {
    bind(target);
    GLCALL(glDrawBuffer(GL_COLOR_ATTACHMENT1));
}

void BloomFB::bindSceneTexture(const int slot) const {
    sceneTex_.attachToSlot(slot);
}

void BloomFB::setUniforms(GLProgram& shaderProgram) const {
    shaderProgram.setInt("uUseBloom", use_);
    shaderProgram.setVec3f("uBloomTint", tint_);
    shaderProgram.setFloat("uBloomIntensity",intensity_);
}

void BloomFB::imguiMenu() {
    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable", &use_);
        if (use_) {
            // ImGui::ColorEdit3("color", (float *)tint_);
            ImGui::SliderInt("Quality", &quality_, 2, 50);
            ImGui::SliderFloat("Bloom Intensity", &intensity_, 0.f, 10.f);
            ImGui::SliderFloat("Threshold", &threshold_, 0.f, 1.f);

        }
    }
}
