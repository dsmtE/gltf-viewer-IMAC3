#pragma once

#include "Texture.hpp"

#include <glm/glm.hpp>
#include <tiny_gltf.h>

#include <glad/glad.h>

#include <filesystem>
#include <vector>

namespace gltfUtils {
    struct VaoRange {
        GLsizei begin; // Index of first element in vertexArrayObjects
        GLsizei count; // Number of elements in range
    };

    bool loadGltfFile(tinygltf::Model &model, const std::filesystem::path& path);
    
    bool attributAvailable(const tinygltf::Model& model, std::string attributName);
    bool attributAvailable(const tinygltf::Mesh& mesh, std::string attributName);

    std::vector<Texture> createTextureObjects(const tinygltf::Model& model);
    std::vector<GLuint> createBufferObjects(const tinygltf::Model& model);
    std::vector<GLuint> createVertexArrayObjects(const tinygltf::Model& model, const std::vector<GLuint>& bufferObjects, 
    const std::vector<std::pair<std::string, GLuint>>& attributesNamesAndIndex, std::vector<VaoRange>& meshVAOInfos, bool& normalEnable, bool& tangentAvailable);

    glm::mat4 getLocalToWorldMatrix(const tinygltf::Node& node, const glm::mat4& parentMatrix);
    void computeSceneBounds(const tinygltf::Model& model, glm::vec3& bboxMin, glm::vec3& bboxMax);
    std::vector<std::vector<glm::vec4>> computeTangentData(const tinygltf::Model& model);
}