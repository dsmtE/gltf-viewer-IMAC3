#include "SSAOFrameBuffer.hpp"

#include "glDebug.hpp"

#include <imgui.h>

#include <random>
#include <functional>

SSAOFrameBuffer::SSAOFrameBuffer(const glm::ivec2& size, const int& samplesSize) : TextureFB(size, GL_R16F), samples_(samplesSize), samplesSize_(samplesSize), bias_(0.5f), radius_(1.0f) {
    generateKernel();
}

void SSAOFrameBuffer::generateKernel() {

    static auto lerp = [](float a, float b, float f) { return a + f * (b - a); };

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine generator;
    auto rand = std::bind(dist, std::ref(generator));

    samples_ = std::vector<glm::vec3>(samplesSize_);

    for (size_t i = 0; i < samplesSize_; ++i) {
        glm::vec3 sample(rand() * 2.0 - 1.0, rand() * 2.0 - 1.0, rand());
        sample  = glm::normalize(sample);
        sample *= rand();

        // more sample close to origin pos
        const float scale = static_cast<float>(i) / static_cast<float>(samplesSize_); 
        sample *= lerp(0.1f, 1.0f, scale * scale);
        samples_[i] = sample;  
    }
}

void SSAOFrameBuffer::sendUniforms(GLProgram& shaderProgram) const {
    shaderProgram.setVec3f("samples", samples_.data(), samples_.size());
    shaderProgram.setInt("samplesSize", static_cast<int>(samples_.size()));
    shaderProgram.setFloat("bias", bias_);
    shaderProgram.setFloat("radius", radius_);
}

void SSAOFrameBuffer::imguiMenu() {
    if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderInt("samplesSize", &samplesSize_, 1, 128)) {
            generateKernel();
        }

        ImGui::SliderFloat("bias", &bias_, 0.f, 1.f);
        ImGui::SliderFloat("radius", &radius_, 0.0f, 10.f);
    }
}
