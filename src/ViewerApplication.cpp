#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/gltf.hpp"
#include "utils/images.hpp"
#include "utils/Texture.hpp"

#include <stb_image_write.h>
#include <tiny_gltf.h>

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, 1);
  }
}

int ViewerApplication::run() {
  // Loader shaders
  GLProgram glslProgram = compileProgram({m_ShadersRootPath / m_vertexShader, m_ShadersRootPath / m_fragmentShader});

  // load model
  tinygltf::Model model;
  bool loadModel = loadGltfFile(model);
  if (!loadModel) { return -1; }

  // Compute model bbox
  glm::vec3 bboxMin, bboxMax;
  computeSceneBounds(model, bboxMin, bboxMax);
  const glm::vec3 diag = bboxMax - bboxMin;
  const float maxDistance = glm::length(diag);

  // Build projection matrix
  const auto projMatrix = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight,0.001f * maxDistance, 1.5f * maxDistance);
  
  std::unique_ptr<CameraController> cameraController;

  switch (cameraControllerType_) {
    case static_cast<int>(EControllerType::Trackball) :
      cameraController = std::make_unique<TrackballCameraController>(m_GLFWHandle.window(), 0.3f * maxDistance);
    break;
    case static_cast<int>(EControllerType::FirstPerson) :
      cameraController = std::make_unique<FirstPersonCameraController>(m_GLFWHandle.window(), 0.3f * maxDistance);
    break;
  }

  if (m_hasUserCamera) {
    cameraController->setCamera(m_userCamera);
  } else {
    const glm::vec3 up = glm::vec3(0, 1, 0);
    const glm::vec3 center = 0.5f * (bboxMax + bboxMin);
    const glm::vec3 eye = diag.z > 0 ? center + diag * 0.6f : center + 2.f * glm::cross(diag, up);
    cameraController->setCamera(Camera{eye, center, up});
  }

  // light parameters
  glm::vec3 lightDirection(1, 1, 1);
  bool lightFromCamera = false;
  glm::vec3 lightColor(1.f, 1.f, 1.f);
  float lightIntensity = 1.f;
  
  bool occlusionEnable = true;
  bool normalEnable = true;
  bool tangentAvailable = false;

  // used with imgui to recompute LightDir
  float lightTheta = 0.f;
  float lightPhi = 0.f;

  // gltf loading
  const std::vector<GLuint> bufferObjects = createBufferObjects(model);

  const std::vector<std::pair<std::string, GLuint>> attributesNamesAndIndex = {{
    {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"TANGENT", 3}
  }};

  std::vector<VaoRange> meshVAOInfos;
  std::vector<GLuint> vertexArrayObjects = createVertexArrayObjects(model, bufferObjects, attributesNamesAndIndex, meshVAOInfos, normalEnable, tangentAvailable);

  std::vector<Texture> textureObjects = createTextureObjects(model);

  // Create white texture for object with no base color texture
  Texture whiteTexture;
  const float white[] = {1, 1, 1, 1};
  whiteTexture.init(1, 1, GL_RGBA, GL_RGBA, GL_FLOAT, white, GL_LINEAR, GL_LINEAR, {GL_REPEAT, GL_REPEAT, GL_REPEAT});

  Texture blackTexture;
  const float black[] = {0, 0, 0, 1};
  blackTexture.init(1, 1, GL_RGBA, GL_RGBA, GL_FLOAT, black, GL_LINEAR, GL_LINEAR, {GL_REPEAT, GL_REPEAT, GL_REPEAT});

  Texture bumpTexture;
  const float bump[] = {0, 0, 1, 1};
  bumpTexture.init(1, 1, GL_RGBA, GL_RGBA, GL_FLOAT, bump, GL_LINEAR, GL_LINEAR, {GL_REPEAT, GL_REPEAT, GL_REPEAT});

  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);

  const std::function<void(const int&)> bindMaterial = [&](const int materialIndex) {
    // default uniforms
    glm::vec4 baseColor(1);
    float metallicFactor = 1;
    float roughnessFactor = 1;
    glm::vec3 emissiveFactor(0);
    float occlusionStrength = 0;
    
    if (materialIndex >= 0) {
      const tinygltf::Material& material = model.materials[materialIndex];
      const tinygltf::PbrMetallicRoughness& pbrMetallicRoughness = material.pbrMetallicRoughness;

      metallicFactor = static_cast<float>(pbrMetallicRoughness.metallicFactor);
      roughnessFactor = static_cast<float>(pbrMetallicRoughness.roughnessFactor);

      for (int i = 0; i < 4; i++)
        baseColor[i] = static_cast<float>(pbrMetallicRoughness.baseColorFactor[i]);

      for (int i = 0; i < 3; i++)
        emissiveFactor[i] = static_cast<float>(material.emissiveFactor[i]);

      occlusionStrength = (float)material.occlusionTexture.strength;

      if (pbrMetallicRoughness.baseColorTexture.index >= 0) {
        const tinygltf::Texture& texture = model.textures[pbrMetallicRoughness.baseColorTexture.index];
        if (texture.source >= 0) {
          const Texture& textureObject = textureObjects[texture.source];

          textureObject.attachToSlot(0);
          glslProgram.setInt("uBaseColorTexture", 0);
          }
      }

      if (pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
        const tinygltf::Texture& texture = model.textures[pbrMetallicRoughness.metallicRoughnessTexture.index];
        if (texture.source >= 0) {
          const Texture& textureObject = textureObjects[texture.source];

          textureObject.attachToSlot(1);
          glslProgram.setInt("uMetallicRoughnessTexture", 1);
          }
      }

      if (material.emissiveTexture.index >= 0) {
        const tinygltf::Texture& texture = model.textures[material.emissiveTexture.index];
        if (texture.source >= 0) {
          const Texture& textureObject = textureObjects[texture.source];

          textureObject.attachToSlot(2);
          glslProgram.setInt("uEmissiveTexture", 2);
          }
      }

      if (material.emissiveTexture.index >= 0) {
        const tinygltf::Texture& texture = model.textures[material.occlusionTexture.index];
        if (texture.source >= 0) {
          const Texture& textureObject = textureObjects[texture.source];

          textureObject.attachToSlot(3);
          glslProgram.setInt("uOcclusionTexture", 3);
          }
      }

      if (material.normalTexture.index >= 0) {
        const tinygltf::Texture& texture = model.textures[material.normalTexture.index];
        if (texture.source >= 0) {
          const Texture& textureObject = textureObjects[texture.source];

          textureObject.attachToSlot(4);
          glslProgram.setInt("uNormalTexture", 4);
          }
      }

    }else {
      whiteTexture.attachToSlot(0);
      glslProgram.setInt("uBaseColorTexture", 0);

      blackTexture.attachToSlot(1);
      glslProgram.setInt("uMetallicRoughnessTexture", 1);

      blackTexture.attachToSlot(2);
      glslProgram.setInt("uEmissiveTexture", 2);

      whiteTexture.attachToSlot(3);
      glslProgram.setInt("uOcclusionTexture", 3);

      bumpTexture.attachToSlot(4);
      glslProgram.setInt("uNormalTexture", 4);
    }

    glslProgram.setFloat("uMetallicFactor", metallicFactor);
    glslProgram.setFloat("uRoughnessFactor", roughnessFactor);
    glslProgram.setVec4f("uBaseColorFactor", baseColor);
    glslProgram.setVec3f("uEmissiveFactor", emissiveFactor);
    glslProgram.setFloat("uOcclusionStrength", occlusionEnable ? occlusionStrength : 0);
    glslProgram.setInt("uNormalEnable", tangentAvailable && normalEnable ? 1 : 0);
  };

  // Lambda function to draw the scene
  const std::function<void(const Camera &)> drawScene = [&](const Camera &camera) {
    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glslProgram.use();
    const glm::mat4 viewMatrix = camera.getViewMatrix();

    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode = [&](int nodeIdx, const glm::mat4 &parentMatrix) {
      const tinygltf::Node& node = model.nodes[nodeIdx];
      const glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);

      // If is actually a mesh (not a camera or a light)
      if (node.mesh >= 0) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        const VaoRange& meshVAOInfo = meshVAOInfos[node.mesh];

        // compute matrix
        const glm::mat4 mvMatrix = viewMatrix * modelMatrix;
        const glm::mat4 mvpMatrix = projMatrix * mvMatrix;
        const glm::mat4 normalMatrix = glm::transpose(glm::inverse(mvMatrix));

        // send matrix as uniforms
        glslProgram.setMat4("uModelViewProjMatrix", mvpMatrix);
        glslProgram.setMat4("uModelViewMatrix", mvMatrix);
        glslProgram.setMat4("uNormalMatrix", normalMatrix);

        // view space conversion of lightDirection
        glslProgram.setVec3f("uLightDirection", lightFromCamera ? glm::vec3(0, 0, 1) : glm::normalize(glm::vec3(viewMatrix * glm::vec4(lightDirection, 0.))));
        glslProgram.setVec3f("uLightColor", lightColor);
        glslProgram.setFloat("uLightIntensity", lightIntensity);
        glslProgram.setInt("uNormalEnable", tangentAvailable && normalEnable ? 1 : 0);

        // iterate over primitives
        for (size_t i = 0; i < mesh.primitives.size(); ++i) {
          // bind primitives vao
          const GLuint vao = vertexArrayObjects[meshVAOInfo.begin + i];
          glBindVertexArray(vao);

          const tinygltf::Primitive& primitive = mesh.primitives[i];
          
          bindMaterial(primitive.material);

          if (primitive.indices >= 0) { // if this primitive uses indices
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const size_t byteOffset = accessor.byteOffset + bufferView.byteOffset;

            glDrawElements(primitive.mode, GLsizei(accessor.count), accessor.componentType, (const GLvoid*)byteOffset);

          } else {
            const int accessorIdx = (*std::begin(primitive.attributes)).second;
            const tinygltf::Accessor& accessor = model.accessors[accessorIdx];
            glDrawArrays(primitive.mode, 0, GLsizei(accessor.count));
          }
        }
      }

      for (const int childNodeIdx : node.children) // Draw children nodes
        drawNode(childNodeIdx, modelMatrix);
    };

    if (model.defaultScene >= 0) { // Draw the scene referenced by gltf file
      for (const int nodeIdx : model.scenes[model.defaultScene].nodes) { // iterate over all main nodes
        drawNode(nodeIdx, glm::mat4(1));
      }
    }
    
  };
  
  if (!m_OutputPath.empty()) { // if output path provided
    const GLsizei numComponents = 3;
    std::vector<unsigned char> pixels(m_nWindowWidth * m_nWindowHeight * numComponents);
    renderToImage(m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data(), [&]() {
      drawScene(cameraController->getCamera());
    });

    // flip the Y axis for image formats convention
    flipImageYAxis(m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data());
    stbi_write_png(m_OutputPath.string().c_str(), m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data(), 0);

    std::cout << "Scene render and saved at : " << m_OutputPath.string() << std::endl;
    return 0;
  }

  // Loop until the user closes the window
  for (size_t iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount) {
    const double seconds = glfwGetTime();

    const Camera camera = cameraController->getCamera();
    drawScene(camera);

    // GUI code:
    imguiNewFrame();

    {
      ImGui::Begin("GUI");
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("eye: %.3f %.3f %.3f", camera.eye().x, camera.eye().y, camera.eye().z);
        ImGui::Text("center: %.3f %.3f %.3f", camera.center().x, camera.center().y, camera.center().z);
        ImGui::Text("up: %.3f %.3f %.3f", camera.up().x, camera.up().y, camera.up().z);
        ImGui::Text("front: %.3f %.3f %.3f", camera.front().x, camera.front().y, camera.front().z);
        ImGui::Text("left: %.3f %.3f %.3f", camera.left().x, camera.left().y, camera.left().z);

        if (ImGui::Button("CLI camera args to clipboard")) {
          std::stringstream ss;
          ss << "--lookat " << camera.eye().x << "," << camera.eye().y << ","
             << camera.eye().z << "," << camera.center().x << ","
             << camera.center().y << "," << camera.center().z << ","
             << camera.up().x << "," << camera.up().y << "," << camera.up().z;
          glfwSetClipboardString(m_GLFWHandle.window(), ss.str().c_str());
        }

        bool ImGuiControllerType = false;
        for (EControllerType c : EControllerType::_values())
          ImGuiControllerType |= ImGui::RadioButton(c._to_string(), &cameraControllerType_, c._to_integral());

        if (ImGuiControllerType) {
          const Camera currentCamera = cameraController->getCamera();
          
          switch (cameraControllerType_) {
          case static_cast<int>(EControllerType::Trackball) :
            cameraController = std::make_unique<TrackballCameraController>(m_GLFWHandle.window(), 0.3f * maxDistance);
            break;
          case static_cast<int>(EControllerType::FirstPerson) :
            cameraController = std::make_unique<FirstPersonCameraController>(m_GLFWHandle.window(), 0.3f * maxDistance);
            break;
          
          default:
            break;
          }
          cameraController->setCamera(currentCamera);
        }

        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
          if (ImGui::SliderFloat("theta", &lightTheta, 0, glm::pi<float>()) || ImGui::SliderFloat("phi", &lightPhi, 0, 2.f * glm::pi<float>())) {
            // update lightDirection
            lightDirection = glm::vec3(glm::sin(lightTheta) * glm::cos(lightPhi), glm::cos(lightTheta), glm::sin(lightTheta) * glm::sin(lightPhi));
          }

          ImGui::ColorEdit3("color", (float *)&lightColor);
          ImGui::SliderFloat("intensity", &lightIntensity, 0.f, 10.f);
          ImGui::Checkbox("light from camera", &lightFromCamera);
          ImGui::Checkbox("enable occlusion", &occlusionEnable);
          if(tangentAvailable) {
            ImGui::Checkbox("enable normal mapping", &normalEnable);
          }
      }

      }
      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    const double ellapsedTime = glfwGetTime() - seconds;
    const bool guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    if (!guiHasFocus) { cameraController->update(float(ellapsedTime)); }

    m_GLFWHandle.swapBuffers(); // Swap front and back buffers
  }

  // TODO clean up allocated GL data

  return 0;
}

ViewerApplication::ViewerApplication(const fs::path &appPath, uint32_t width,
    uint32_t height, const fs::path &gltfFile,
    const std::vector<float> &lookatArgs, const std::string &vertexShader,
    const std::string &fragmentShader, const fs::path &output) :
    m_nWindowWidth(width),
    m_nWindowHeight(height),
    m_AppPath{appPath},
    m_AppName{m_AppPath.stem().string()},
    m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
    m_ShadersRootPath{m_AppPath.parent_path() / "shaders"},
    m_gltfFilePath{gltfFile},
    m_OutputPath{output}
{
  if (!lookatArgs.empty()) {
    m_hasUserCamera = true;
    m_userCamera =
        Camera{glm::vec3(lookatArgs[0], lookatArgs[1], lookatArgs[2]),
            glm::vec3(lookatArgs[3], lookatArgs[4], lookatArgs[5]),
            glm::vec3(lookatArgs[6], lookatArgs[7], lookatArgs[8])};
  }

  if (!vertexShader.empty()) {
    m_vertexShader = vertexShader;
  }

  if (!fragmentShader.empty()) {
    m_fragmentShader = fragmentShader;
  }

  // At exit, ImGUI will store its windows positions in this file
  ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); 

  glfwSetKeyCallback(m_GLFWHandle.window(), keyCallback);

  printGLVersion();
}

bool ViewerApplication::loadGltfFile(tinygltf::Model &model) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, m_gltfFilePath.string());

  if (!warn.empty()) { std::cerr << warn << std::endl; }

  if (!err.empty()) { std::cerr << err << std::endl; }

  if (!ret) { std::cerr << "Failed to parse glTF file" << std::endl; }

  return ret;
}

std::vector<GLuint> ViewerApplication::createBufferObjects(const tinygltf::Model &model) const {
  std::vector<GLuint> bufferObjects(model.buffers.size(), 0);

  glGenBuffers(static_cast<GLsizei>(model.buffers.size()), bufferObjects.data());

  for (size_t i = 0; i < model.buffers.size(); ++i) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferObjects[i]);
    glBufferStorage(GL_ARRAY_BUFFER, model.buffers[i].data.size(), model.buffers[i].data.data(), 0);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  return bufferObjects;
}

std::vector<Texture> ViewerApplication::createTextureObjects(const tinygltf::Model &model) const {
  std::vector<Texture> textureObjects(model.textures.size());

  tinygltf::Sampler defaultSampler;
  defaultSampler.minFilter = GL_LINEAR;
  defaultSampler.magFilter = GL_LINEAR;
  defaultSampler.wrapS = GL_REPEAT;
  defaultSampler.wrapT = GL_REPEAT;
  defaultSampler.wrapR = GL_REPEAT;

  for (size_t i = 0; i < model.textures.size(); ++i) { // foreach model texture
    Texture& texture = textureObjects[i];
    const tinygltf::Texture& modelTexture = model.textures[i];
    assert(modelTexture.source >= 0);
    const tinygltf::Image& image = model.images[modelTexture.source];
    const tinygltf::Sampler& sampler = modelTexture.sampler >= 0 ? model.samplers[modelTexture.sampler] : defaultSampler;

    texture.init(image.width, image.height, GL_RGBA, GL_RGBA, image.pixel_type, image.image.data(), sampler);
  }

  return textureObjects;
}

std::vector<GLuint> ViewerApplication::createVertexArrayObjects(const tinygltf::Model& model, const std::vector<GLuint>& bufferObjects, 
  const std::vector<std::pair<std::string, GLuint>>& attributesNamesAndIndex, std::vector<VaoRange>& meshVAOInfos, bool& normalEnable, bool& tangentAvailable) const {

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