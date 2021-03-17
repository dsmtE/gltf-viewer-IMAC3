#include "gltfUtils.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

namespace gltfUtils {

  bool loadGltfFile(tinygltf::Model &model, const std::filesystem::path& path) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    const bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());

    if (!warn.empty()) { std::cerr << warn << std::endl; }
    if (!err.empty()) { std::cerr << err << std::endl; }
    if (!ret) { std::cerr << "Failed to parse glTF file" << std::endl; }

    return ret;
  }

  std::vector<Texture> createTextureObjects(const tinygltf::Model& model) {
    std::vector<Texture> textureObjects;
    textureObjects.reserve(model.textures.size());

    tinygltf::Sampler defaultSampler;
    defaultSampler.minFilter = GL_LINEAR;
    defaultSampler.magFilter = GL_LINEAR;
    defaultSampler.wrapS = GL_REPEAT;
    defaultSampler.wrapT = GL_REPEAT;
    defaultSampler.wrapR = GL_REPEAT;

    for (size_t i = 0; i < model.textures.size(); ++i) { // foreach model texture
      const tinygltf::Texture& modelTexture = model.textures[i];
      assert(modelTexture.source >= 0);
      const tinygltf::Image& image = model.images[modelTexture.source];
      const tinygltf::Sampler& sampler = modelTexture.sampler >= 0 ? model.samplers[modelTexture.sampler] : defaultSampler;

      const glm::ivec2 size = {image.width, image.height};
      textureObjects.emplace_back(size, GL_RGBA16F);
      textureObjects[i].upload(GL_RGBA, image.pixel_type, image.image.data(), sampler);
    }

    return textureObjects;
  }


  bool attributAvailable(const tinygltf::Mesh& mesh, std::string attributName) {
    bool available = true;
    if(mesh.primitives.size() > 0) {
      std::wcerr << "Mesh has multiple primitives. we will check attribut avalability for each primitives." << std::endl;
    }

    for (size_t primitiveIdx = 0; primitiveIdx < mesh.primitives.size(); ++primitiveIdx) { // for each primitives
      const tinygltf::Primitive &primitive = mesh.primitives[primitiveIdx];
      if (primitive.attributes.find(attributName) == end(primitive.attributes)) {
        available = false;
        break;
      }
    }
    return available;
  }

  bool attributAvailable(const tinygltf::Model& model, std::string attributName) {
    bool available = true;
    for (size_t i = 0; i < model.meshes.size(); ++i) {
      if(!attributAvailable(model.meshes[i], attributName)) {
        available = false;
        break;
      }
    }
    return available;
  }

  std::vector<GLuint> createBufferObjects(const tinygltf::Model& model) {
    std::vector<GLuint> bufferObjects(model.buffers.size(), 0);

    glGenBuffers(static_cast<GLsizei>(model.buffers.size()), bufferObjects.data());

    for (size_t i = 0; i < model.buffers.size(); ++i) {
      glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
      glBufferStorage(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(), 0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return bufferObjects;
  }

  std::vector<GLuint> createVertexArrayObjects(const tinygltf::Model& model, const std::vector<GLuint>& bufferObjects, 
    const std::vector<std::pair<std::string, GLuint>>& attributesNamesAndIndex, std::vector<VaoRange>& meshVAOInfos, bool& normalEnable, bool& tangentAvailable) {

    std::vector<GLuint> vertexArrayObjects; // will be stack VAO for each mesh and primitives in this vector

    meshVAOInfos.resize(model.meshes.size()); // one VAO info struct for each meshes

    for (size_t i = 0; i < model.meshes.size(); ++i) {
      const tinygltf::Mesh &mesh = model.meshes[i];
      VaoRange& vaoRange = meshVAOInfos[i];
      vaoRange.begin = static_cast<GLsizei>(vertexArrayObjects.size());
      vaoRange.count = static_cast<GLsizei>(mesh.primitives.size());

      vertexArrayObjects.resize(vertexArrayObjects.size() + mesh.primitives.size()); // extend size for this mesh primitives count

      glGenVertexArrays(vaoRange.count, &vertexArrayObjects[vaoRange.begin]); // gen Vertex arrays for our new mesh primitives

      for (size_t primitiveIdx = 0; primitiveIdx < mesh.primitives.size(); ++primitiveIdx) { // for each primitives
        const GLuint vao = vertexArrayObjects[vaoRange.begin + primitiveIdx];
        const tinygltf::Primitive &primitive = mesh.primitives[primitiveIdx];
        glBindVertexArray(vao);

        for(const auto &attributInfos : attributesNamesAndIndex) { // for each attributs

          const std::string attributName = attributInfos.first;
          const GLuint attributIdx = attributInfos.second;

          const auto iterator = primitive.attributes.find(attributName);
          if (iterator != end(primitive.attributes)) {
            const size_t accessorIdx = (*iterator).second;
            const tinygltf::Accessor &accessor = model.accessors[accessorIdx];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const size_t bufferIdx = bufferView.buffer;

            glEnableVertexAttribArray(attributIdx);
            assert(GL_ARRAY_BUFFER == bufferView.target); // check target
            glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[bufferIdx]); // bind to our previous generated buffer Object
          
            const size_t byteOffset = accessor.byteOffset + bufferView.byteOffset;
            // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
            glVertexAttribPointer(attributIdx, accessor.type, accessor.componentType, GL_FALSE,
            static_cast<GLsizei>(bufferView.byteStride), (const GLvoid *)byteOffset);

            if(attributName == "TANGENT") tangentAvailable = true;
          } else {
            std::cerr << "Unknown attribut \"" << attributName <<  "\" in mesh " << mesh.name << "." << std::endl;
          }
        } // end attributs

        if (primitive.indices >= 0) { // bind indices if needed
          const size_t accessorIdx = primitive.indices;
          const tinygltf::Accessor &accessor = model.accessors[accessorIdx];
          const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
          const size_t bufferIdx = bufferView.buffer;

          assert(GL_ELEMENT_ARRAY_BUFFER == bufferView.target); // check target
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects[bufferIdx]);
        }

      } // end mesh primitives
    } // end meshs

    glBindVertexArray(0); // unbind vertex array safety

    std::cout << "Number of VAOs: " << vertexArrayObjects.size() << std::endl;

    return vertexArrayObjects;
  }


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
    const glm::quat rotationQuat = node.rotation.empty() ? glm::quat(1, 0, 0, 0) :
      glm::quat(float(node.rotation[3]), float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]));
    const glm::mat4 TR = T * glm::mat4_cast(rotationQuat);
    return node.scale.empty() ? TR : glm::scale(TR, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
  };

  void computeSceneBounds(const tinygltf::Model& model, glm::vec3 &bboxMin, glm::vec3 &bboxMax) {
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

  namespace { // scoped to file functions
    glm::vec4 computeTriangleTangent(const std::array<glm::vec3, 3>& pos, const std::array<glm::vec3, 3>& uvs) {

      const glm::vec3 deltaPos1 = pos[1]-pos[0];
      const glm::vec3 deltaPos2 = pos[2]-pos[0];

      const glm::vec2 deltaUV1 = uvs[1]-uvs[0];
      const glm::vec2 deltaUV2 = uvs[2]-uvs[0];

      const glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

      // Note that tangents in GLTF are stored as vec4(Float4), where w is used to indicate handedness 
      return glm::vec4(tangent, 1);
    }
  }

  std::vector<std::vector<glm::vec4>> computeTangentData(const tinygltf::Model& model) {
    // help : https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#geometry
    std::vector<std::vector<glm::vec4>> meshesTangentBuffer(model.meshes.size()); // tengent value will be stacked for each mesh and primitives in this vector

    for (size_t meshIdx = 0; meshIdx < model.meshes.size(); ++meshIdx) {
      const tinygltf::Mesh& mesh = model.meshes[meshIdx];

      for (size_t primitiveIdx = 0; primitiveIdx < mesh.primitives.size(); ++primitiveIdx) { // for each primitives
        const tinygltf::Primitive& primitive = mesh.primitives[primitiveIdx];

        // Extract POSITION and UV attributs
        const auto posAttrIdxIt = primitive.attributes.find("POSITION");

        if (posAttrIdxIt == end(primitive.attributes)) {
          std::cerr << "POSITION attribut can't be found for the primitives " << primitiveIdx << "in the mesh named " << mesh.name << std::endl;
          continue;
        }
        const auto uvAttrIdxIt = primitive.attributes.find("TEXCOORD_0");
        if (uvAttrIdxIt == end(primitive.attributes)) {
          std::cerr << "TEXCOORD_0 attribut can't be found for the primitives " << primitiveIdx << "in the mesh named " << mesh.name << std::endl;
          continue;
        }

        const tinygltf::Accessor& posAccessor = model.accessors[(*posAttrIdxIt).second];
        const tinygltf::Accessor& uvAccessor = model.accessors[(*uvAttrIdxIt).second];

        assert(posAccessor.type == TINYGLTF_TYPE_VEC3 && "computeTangent: expected type of POSTION is VEC3"); // , got" <<  posAccessor.type);
        assert(uvAccessor.type == TINYGLTF_TYPE_VEC2 && "computeTangent: expected type of TEXCOORD_0 is VEC2");

        assert(posAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && "POSTION: expected float componentType");
        assert(uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && "TEXCOORD_0: expected float componentType");

        assert(posAccessor.count == uvAccessor.count && "position an uv accessors count (buffer size) must be equals");

        std::cout << __FUNCTION__ << ": posAccessor.count : " << posAccessor.count << std::endl;

        const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
        const size_t posByteOffset = posAccessor.byteOffset + posBufferView.byteOffset;
        const tinygltf::Buffer &posBuffer = model.buffers[posBufferView.buffer];
        const size_t posByteStride = posBufferView.byteStride ? posBufferView.byteStride : 3 * 4; // sizeof(float) using tinygltf::GetComponentSizeInBytes
        
        const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
        const size_t uvByteOffset = uvAccessor.byteOffset + uvBufferView.byteOffset;
        const tinygltf::Buffer &uvBuffer = model.buffers[uvBufferView.buffer];
        const size_t uvByteStride = uvBufferView.byteStride ? uvBufferView.byteStride : 3 * 4;

        if (primitive.indices >= 0) {
          throw std::runtime_error("indexed mesh: tengant values can't be coputed properly.");
          // const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
          // const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
          // const size_t indexByteOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
          // const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];
          // size_t indexByteStride = indexBufferView.byteStride;

          // if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
          //    indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
          //    indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
          //   indexByteStride = indexByteStride ? indexByteStride : tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
          // }else {
          //   std::cerr << "Index component type" << indexAccessor.componentType << "not supported" << std::endl;
          //   continue;
          // }

          // std::cout << __FUNCTION__ << ": indexAccessor count : " << indexAccessor.count << std::endl;

          // assert(indexAccessor.count % 3 == 0 && "invalid indexAccessor count");

          // for (size_t i = 0; i < indexAccessor.count; i+=3) {
          //   uint32_t idx1 = 0;
          //   uint32_t idx2 = 0;
          //   uint32_t idx3 = 0;

          //   switch (indexAccessor.componentType) {
          //     case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
          //       idx1 = reinterpret_cast<const uint8_t&>(indexBuffer.data[indexByteOffset + indexByteStride * i]);
          //       idx2 = reinterpret_cast<const uint8_t&>(indexBuffer.data[indexByteOffset + indexByteStride * (i+1)]);
          //       idx3 = reinterpret_cast<const uint8_t&>(indexBuffer.data[indexByteOffset + indexByteStride * (i+2)]);
          //       break;
          //     case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          //       idx1 = reinterpret_cast<const uint16_t&>(indexBuffer.data[indexByteOffset + indexByteStride * i]);
          //       idx2 = reinterpret_cast<const uint16_t&>(indexBuffer.data[indexByteOffset + indexByteStride * (i+1)]);
          //       idx3 = reinterpret_cast<const uint16_t&>(indexBuffer.data[indexByteOffset + indexByteStride * (i+2)]);
          //       break;
          //     case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
          //       idx1 = reinterpret_cast<const uint32_t&>(indexBuffer.data[indexByteOffset + indexByteStride * i]);
          //       idx2 = reinterpret_cast<const uint32_t&>(indexBuffer.data[indexByteOffset + indexByteStride * (i+1)]);
          //       idx3 = reinterpret_cast<const uint32_t&>(indexBuffer.data[indexByteOffset + indexByteStride * (i+2)]);
          //       break;
          //   }
          //   const glm::vec3& localPos1 = reinterpret_cast<const glm::vec3&>(posBuffer.data[posByteOffset + posByteStride * idx1]);
          //   const glm::vec3& localPos2 = reinterpret_cast<const glm::vec3&>(posBuffer.data[posByteOffset + posByteStride * idx2]);
          //   const glm::vec3& localPos3 = reinterpret_cast<const glm::vec3&>(posBuffer.data[posByteOffset + posByteStride * idx3]);

          //   const glm::vec3& localUv1 = reinterpret_cast<const glm::vec3&>(uvBuffer.data[uvByteOffset + uvByteStride * idx1]);
          //   const glm::vec3& localUv2 = reinterpret_cast<const glm::vec3&>(uvBuffer.data[uvByteOffset + uvByteStride * idx2]);
          //   const glm::vec3& localUv3 = reinterpret_cast<const glm::vec3&>(uvBuffer.data[uvByteOffset + uvByteStride * idx3]);

          //   const glm::vec4 tangent = computeTriangleTangent({localPos1, localPos2, localPos3}, {localUv1, localUv2, localUv3});
          //   meshesTangentBuffer[meshIdx].push_back(std::move(tangent)); // Same tangent for all three vertices of the triangle
          // }

        } else {
          assert(posAccessor.count % 3 == 0 && "invalid accessor count");
          for (size_t i = 0; i < posAccessor.count; i+=3) {
            const glm::vec3& localPos1 = reinterpret_cast<const glm::vec3&>(posBuffer.data[posByteOffset + posByteStride * i]);
            const glm::vec3& localPos2 = reinterpret_cast<const glm::vec3&>(posBuffer.data[posByteOffset + posByteStride * (i+1)]);
            const glm::vec3& localPos3 = reinterpret_cast<const glm::vec3&>(posBuffer.data[posByteOffset + posByteStride * (i+2)]);
            
            const glm::vec3& localUv1 = reinterpret_cast<const glm::vec3&>(uvBuffer.data[uvByteOffset + uvByteStride * i]);
            const glm::vec3& localUv2 = reinterpret_cast<const glm::vec3&>(uvBuffer.data[uvByteOffset + uvByteStride * (i+1)]);
            const glm::vec3& localUv3 = reinterpret_cast<const glm::vec3&>(uvBuffer.data[uvByteOffset + uvByteStride * (i+2)]);

            const glm::vec4 tangent = computeTriangleTangent({localPos1, localPos2, localPos3}, {localUv1, localUv2, localUv3});
            for (size_t i = 0; i < 3; ++i)
              meshesTangentBuffer[meshIdx].push_back(tangent); // Same tangent for all three vertices of the triangle
          }
        }
      } // end mesh primitives
    } // end meshs
    return meshesTangentBuffer;
  }
}
