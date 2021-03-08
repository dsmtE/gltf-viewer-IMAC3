#include "gltf.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

glm::mat4 getLocalToWorldMatrix(const tinygltf::Node& node, const glm::mat4& parentMatrix) {
  // Extract model matrix
  // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#transformations
  if (!node.matrix.empty()) {
    return parentMatrix * glm::mat4(node.matrix[0], node.matrix[1],
                              node.matrix[2], node.matrix[3], node.matrix[4],
                              node.matrix[5], node.matrix[6], node.matrix[7],
                              node.matrix[8], node.matrix[9], node.matrix[10],
                              node.matrix[11], node.matrix[12], node.matrix[13],
                              node.matrix[14], node.matrix[15]);
  }
  const glm::mat4 T = node.translation.empty() ? parentMatrix : 
    glm::translate(parentMatrix, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
  const glm::quat rotationQuat = node.rotation.empty() ? glm::quat(1, 0, 0, 0)
          : glm::quat(float(node.rotation[3]), float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]));
  const glm::mat4 TR = T * glm::mat4_cast(rotationQuat);
  return node.scale.empty() ? TR : glm::scale(TR, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
};

void computeSceneBounds(const tinygltf::Model &model, glm::vec3 &bboxMin, glm::vec3 &bboxMax) {
  // Compute scene bounding box
  // todo refactor with scene drawing
  // todo need a visitScene generic function that takes a accept() functor
  bboxMin = glm::vec3(std::numeric_limits<float>::max());
  bboxMax = glm::vec3(std::numeric_limits<float>::lowest());
  if (model.defaultScene >= 0) {
    const std::function<void(int, const glm::mat4 &)> updateBounds = [&](int nodeIdx, const glm::mat4 &parentMatrix) {
      const tinygltf::Node &node = model.nodes[nodeIdx];
      const glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);
      if (node.mesh >= 0) {
        const tinygltf::Mesh &mesh = model.meshes[node.mesh];
        for (size_t pIdx = 0; pIdx < mesh.primitives.size(); ++pIdx) {
          const tinygltf::Primitive& primitive = mesh.primitives[pIdx];
          const auto positionAttrIdxIt = primitive.attributes.find("POSITION");
          if (positionAttrIdxIt == end(primitive.attributes)) {
            continue;
          }
          const tinygltf::Accessor &positionAccessor = model.accessors[(*positionAttrIdxIt).second];
          if (positionAccessor.type != 3) {
            std::cerr << "Position accessor with type != VEC3, skipping" << std::endl;
            continue;
          }
          const tinygltf::BufferView &positionBufferView = model.bufferViews[positionAccessor.bufferView];
          const size_t byteOffset = positionAccessor.byteOffset + positionBufferView.byteOffset;
          const tinygltf::Buffer &positionBuffer = model.buffers[positionBufferView.buffer];
          const size_t positionByteStride = positionBufferView.byteStride ? positionBufferView.byteStride : 3 * sizeof(float);

          if (primitive.indices >= 0) {
            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const size_t indexByteOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
            const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];
            size_t indexByteStride = indexBufferView.byteStride;

            switch (indexAccessor.componentType) {
              default:
                std::cerr << "Primitive index accessor with bad componentType "
                          << indexAccessor.componentType << ", skipping it."
                          << std::endl;
                continue;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                indexByteStride = indexByteStride ? indexByteStride : sizeof(uint8_t);
                break;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                indexByteStride = indexByteStride ? indexByteStride : sizeof(uint16_t);
                break;
              case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                indexByteStride = indexByteStride ? indexByteStride : sizeof(uint32_t);
                break;
            }

            for (size_t i = 0; i < indexAccessor.count; ++i) {
              uint32_t index = 0;
              switch (indexAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                  index = reinterpret_cast<const uint8_t&>(indexBuffer.data[indexByteOffset + indexByteStride * i]);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                  index = reinterpret_cast<const uint16_t&>(indexBuffer.data[indexByteOffset + indexByteStride * i]);
                  break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                  index = reinterpret_cast<const uint32_t&>(indexBuffer.data[indexByteOffset + indexByteStride * i]);
                  break;
              }
              const glm::vec3& localPosition = reinterpret_cast<const glm::vec3&>(positionBuffer.data[byteOffset + positionByteStride * index]);
              const glm::vec3 worldPosition = glm::vec3(modelMatrix * glm::vec4(localPosition, 1.f));
              bboxMin = glm::min(bboxMin, worldPosition);
              bboxMax = glm::max(bboxMax, worldPosition);
            }
          } else {
            for (size_t i = 0; i < positionAccessor.count; ++i) {
              const glm::vec3& localPosition = reinterpret_cast<const glm::vec3&>(positionBuffer.data[byteOffset + positionByteStride * i]);
              const glm::vec3 worldPosition = glm::vec3(modelMatrix * glm::vec4(localPosition, 1.f));
              bboxMin = glm::min(bboxMin, worldPosition);
              bboxMax = glm::max(bboxMax, worldPosition);
            }
          }
        }
      }
      for (const int childNodeIdx : node.children) {
        updateBounds(childNodeIdx, modelMatrix);
      }
    };
    
    for (const int nodeIdx : model.scenes[model.defaultScene].nodes) {
      updateBounds(nodeIdx, glm::mat4(1));
    }
  }
}