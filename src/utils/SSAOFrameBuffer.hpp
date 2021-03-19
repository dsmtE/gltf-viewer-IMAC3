#pragma once

#include "TextureFB.hpp"
#include "Texture.hpp"

#include <vector>

class SSAOFrameBuffer : public TextureFB {

    public:
        SSAOFrameBuffer(const glm::ivec2& size, const int& samplesSize = 64);
        ~SSAOFrameBuffer() = default;

        void sendUniforms(GLProgram& shaderProgram) const;

        void imguiMenu();

    private:
        void generateKernel();

        std::vector<glm::vec3> samples_;
        int samplesSize_;
        float bias_;
        float radius_;
};
