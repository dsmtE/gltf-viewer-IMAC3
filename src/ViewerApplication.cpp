#include "ViewerApplication.hpp"

#include "utils/gltfUtils.hpp"
#include "utils/images.hpp"
#include "utils/Texture.hpp"
#include "utils/GBuffer.hpp"
#include "utils/SSAOFrameBuffer.hpp"
#include "utils/TextureFB.hpp"
#include "utils/CubeMap.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include <stb_image_write.h>
#include <tiny_gltf.h>

#include <iostream>
#include <numeric>

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, 1);
  }
}

int ViewerApplication::run() {
  // Loader shaders
  GLProgram geometryPassProgram = compileProgram({m_ShadersRootPath / "geometryPass.vs.glsl", m_ShadersRootPath / "geometryPass.fs.glsl"});
  GLProgram shadingPassProgram = compileProgram({m_ShadersRootPath / "shadingPass.vs.glsl", m_ShadersRootPath / "shadingPass.fs.glsl"});
  GLProgram ssaoPassProgramm = compileProgram({m_ShadersRootPath / "shadingPass.vs.glsl", m_ShadersRootPath / "ssaoPass.fs.glsl"});
  GLProgram ssaoBlurPassProgram = compileProgram({m_ShadersRootPath / "shadingPass.vs.glsl", m_ShadersRootPath / "ssaoBlurPass.fs.glsl"});

  GLProgram cubeMapPassProgram = compileProgram({m_ShadersRootPath / "cubeMap.vs.glsl", m_ShadersRootPath / "cubeMap.fs.glsl"});
  

  GBuffer gBuffer({m_nWindowWidth, m_nWindowHeight});

  SSAOFrameBuffer ssaoFB({m_nWindowWidth, m_nWindowHeight});

  TextureFB SSAOBlurFB({m_nWindowWidth, m_nWindowHeight}, GL_R16F);

  CubeMap skyBox;
  
  const std::array<std::string, 6> facesPaths = {
    (m_AssetsRootPath / "skybox" / "right.jpg").string(),
    (m_AssetsRootPath / "skybox" / "left.jpg").string(), 
    (m_AssetsRootPath / "skybox" / "top.jpg").string(), 
    (m_AssetsRootPath / "skybox" / "bottom.jpg").string(), 
    (m_AssetsRootPath / "skybox" / "front.jpg").string(), 
    (m_AssetsRootPath / "skybox" / "back.jpg").string()
  };
  
  skyBox.upload(facesPaths);

  // load model
  tinygltf::Model model;
  const bool loadModel = gltfUtils::loadGltfFile(model, m_gltfFilePath);
  if (!loadModel) { return -1; }

  // Compute model bbox
  glm::vec3 bboxMin, bboxMax;
  gltfUtils::computeSceneBounds(model, bboxMin, bboxMax);
  const glm::vec3 diag = bboxMax - bboxMin;
  const float maxDistance = glm::length(diag);

  // Build projection matrix
  const glm::mat4 projMatrix = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight,0.001f * maxDistance, 1.5f * maxDistance);
  
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
  bool SSAOEnable = true;
  bool IBLEnable = true;
  float occlusionStrength = 0.5f;
  bool normalEnable = true;
  bool tangentAvailable = false;

  int deferredShadingDisplayId = 0;

  // used with imgui to recompute LightDir
  float lightTheta = 0.f;
  float lightPhi = 0.f;

  // std::vector<std::vector<glm::vec4>> meshesTangentBuffer;
  // if(!gltfUtils::attributAvailable(model, "TANGENT")) {
  //   try {
  //     meshesTangentBuffer = gltfUtils::computeTangentData(model);
  //   } catch (const std::runtime_error e) {
  //     std::cout << e.what() << std::endl;
  //   }
  // }
  
  // gltf loading
  const std::vector<GLuint> bufferObjects = gltfUtils::createBufferObjects(model);

  const std::vector<std::pair<std::string, GLuint>> attributesNamesAndIndex = {{
    {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORD_0", 2}, {"TANGENT", 3}
  }};

  std::vector<gltfUtils::VaoRange> meshVAOInfos;
  std::vector<GLuint> vertexArrayObjects = gltfUtils::createVertexArrayObjects(model, bufferObjects, attributesNamesAndIndex, meshVAOInfos, normalEnable, tangentAvailable);

  std::vector<Texture> textureObjects = gltfUtils::createTextureObjects(model);

  // Create white texture for object with no base color texture
  Texture whiteTexture({1, 1}, GL_RGBA16F);
  const float white[] = {1, 1, 1, 1};
  whiteTexture.uploadAndSetup(GL_RGBA, GL_FLOAT, white, GL_LINEAR, GL_LINEAR, {GL_REPEAT, GL_REPEAT, GL_REPEAT});

  Texture blackTexture({1, 1}, GL_RGBA16F);
  const float black[] = {0, 0, 0, 1};
  blackTexture.uploadAndSetup(GL_RGBA, GL_FLOAT, black, GL_LINEAR, GL_LINEAR, {GL_REPEAT, GL_REPEAT, GL_REPEAT});

  Texture bumpTexture({1, 1}, GL_RGBA16F);
  const float bump[] = {0, 0, 1, 1};
  bumpTexture.uploadAndSetup(GL_RGBA, GL_FLOAT, bump, GL_LINEAR, GL_LINEAR, {GL_REPEAT, GL_REPEAT, GL_REPEAT});

  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);

  const std::function<void(GLProgram&, const int&)> bindMaterial = [&](GLProgram& shaderprogram, const int materialIndex) {
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

      const std::array<std::pair<int, std::string>, 5> texturesIdxAndNames = {{
        {pbrMetallicRoughness.baseColorTexture.index, "uBaseColorTexture"},
        {material.normalTexture.index, "uNormalTexture"},
        {pbrMetallicRoughness.metallicRoughnessTexture.index, "uMetallicRoughnessTexture"},
        {material.occlusionTexture.index, "uOcclusionTexture"},
        {material.emissiveTexture.index, "uEmissiveTexture"}
      }};

      for (int i = 0; i < texturesIdxAndNames.size(); ++i){
        const int id = texturesIdxAndNames[i].first;
        const std::string& uName = texturesIdxAndNames[i].second;

        if(id >= 0) {
          const tinygltf::Texture& texture = model.textures[id];
          if (texture.source >= 0) {
            const Texture& textureObject = textureObjects[texture.source];
            textureObject.attachToShaderSlot(shaderprogram, uName.c_str(), i);
          }
        }
      }

    }else {
      whiteTexture.attachToShaderSlot(shaderprogram, "uBaseColorTexture", 0);
      bumpTexture.attachToShaderSlot(shaderprogram, "uNormalTexture", 1);
      blackTexture.attachToShaderSlot(shaderprogram, "uMetallicRoughnessTexture", 2);
      whiteTexture.attachToShaderSlot(shaderprogram, "uOcclusionTexture", 3);
      blackTexture.attachToShaderSlot(shaderprogram, "uEmissiveTexture", 4);
    }

    shaderprogram.setVec4f("uBaseColorFactor", baseColor);
    shaderprogram.setFloat("uMetallicFactor", metallicFactor);
    shaderprogram.setFloat("uRoughnessFactor", roughnessFactor);
    shaderprogram.setVec3f("uEmissiveFactor", emissiveFactor);
    shaderprogram.setFloat("uOcclusionStrength", occlusionStrength);
    shaderprogram.setInt("uNormalEnable", tangentAvailable && normalEnable ? 1 : 0);
  };

  // Lambda function to draw the scene
  const std::function<void(GLProgram&, const Camera &)> drawScene = [&](GLProgram& shaderprogram, const Camera &camera) {    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    shaderprogram.use();
    const glm::mat4 viewMatrix = camera.getViewMatrix();

    // send globa lighting uniforms
    shaderprogram.setInt("uNormalEnable", tangentAvailable && normalEnable ? 1 : 0);
    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode = [&](int nodeIdx, const glm::mat4 &parentMatrix) {
      const tinygltf::Node& node = model.nodes[nodeIdx];
      const glm::mat4 modelMatrix = gltfUtils::getLocalToWorldMatrix(node, parentMatrix);

      // If is actually a mesh (not a camera or a light)
      if (node.mesh >= 0) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        const gltfUtils::VaoRange& meshVAOInfo = meshVAOInfos[node.mesh];

        // compute matrix
        const glm::mat4 mvMatrix = viewMatrix * modelMatrix;
        const glm::mat4 mvpMatrix = projMatrix * mvMatrix;
        const glm::mat4 normalMatrix = glm::transpose(glm::inverse(mvMatrix));

        // send matrix as uniforms
        shaderprogram.setMat4("uModelViewProjMatrix", mvpMatrix);
        shaderprogram.setMat4("uModelViewMatrix", mvMatrix);
        shaderprogram.setMat4("uNormalMatrix", normalMatrix);

        // iterate over primitives
        for (size_t i = 0; i < mesh.primitives.size(); ++i) {
          // bind primitives vao
          const GLuint vao = vertexArrayObjects[meshVAOInfo.begin + i];
          glBindVertexArray(vao);

          const tinygltf::Primitive& primitive = mesh.primitives[i];
          
          bindMaterial(shaderprogram, primitive.material);

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
    // const GLsizei numComponents = 3;
    // std::vector<unsigned char> pixels(m_nWindowWidth * m_nWindowHeight * numComponents);
    // renderToImage(m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data(), [&]() {
    //   drawScene(glslProgram, cameraController->getCamera());
    // });

    // // flip the Y axis for image formats convention
    // flipImageYAxis(m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data());
    // stbi_write_png(m_OutputPath.string().c_str(), m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data(), 0);

    // std::cout << "Scene render and saved at : " << m_OutputPath.string() << std::endl;

    std::cout << "Scene render and save not working with deferred rendering currently." << std::endl;
    return 0;
  }

  bool shouldDraw = true;
  double secondLastDraw = 0;

  // Loop until the user closes the window
  for (size_t iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount) {
    const double seconds = glfwGetTime();

    const Camera& camera = cameraController->getCamera();

    if(shouldDraw) {
      secondLastDraw = seconds;
      shouldDraw = false;

      // Geometry Pass
      gBuffer.bind(GL_DRAW_FRAMEBUFFER);

      geometryPassProgram.use();
      geometryPassProgram.setInt("uNormalEnable", tangentAvailable && normalEnable ? 1 : 0);
      drawScene(geometryPassProgram, camera);

      gBuffer.unbind(GL_DRAW_FRAMEBUFFER);

      // SSAO Pass
      ssaoFB.bind(GL_DRAW_FRAMEBUFFER);

      GLCALL(glClear(GL_COLOR_BUFFER_BIT));
      ssaoPassProgramm.use();
      ssaoPassProgramm.setMat4("uProjMatrix", projMatrix);
      ssaoFB.sendUniforms(ssaoPassProgramm);
      gBuffer.bindTexturesToShader(ssaoPassProgramm);
      gBuffer.render(); // TODO use better location for screenRender VAO trick

      ssaoFB.unbind(GL_DRAW_FRAMEBUFFER);

      // SSAO Blur
      SSAOBlurFB.bind(GL_DRAW_FRAMEBUFFER);

      GLCALL(glClear(GL_COLOR_BUFFER_BIT));
      ssaoBlurPassProgram.use();
      ssaoFB.bindTexture(0);
      ssaoBlurPassProgram.setInt("uSSAO", 0);
      gBuffer.render();

      SSAOBlurFB.unbind(GL_DRAW_FRAMEBUFFER);

      if (deferredShadingDisplayId <= 1) {
        // shading/lighting Pass
        GLCALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        
        shadingPassProgram.use();
        shadingPassProgram.setVec3f("uLightDirection", lightFromCamera ? glm::vec3(0, 0, 1) : glm::normalize(glm::vec3(camera.getViewMatrix() * glm::vec4(lightDirection, 0.))));
        shadingPassProgram.setVec3f("uLightColor", lightColor);
        shadingPassProgram.setFloat("uLightIntensity", lightIntensity);
        shadingPassProgram.setFloat("uOcclusionStrength", occlusionEnable ? occlusionStrength : 0);
        shadingPassProgram.setInt("uDeferredShadingDisplayId", deferredShadingDisplayId);
        shadingPassProgram.setInt("uEnableSSAO", SSAOEnable ? 1 : 0);
        shadingPassProgram.setInt("uEnableIBL",IBLEnable ? 1 : 0);
        gBuffer.bindTexturesToShader(shadingPassProgram);
        SSAOBlurFB.bindTexture(5);
        shadingPassProgram.setInt("uSSAO", 5);

        skyBox.bindTexture(6);
        ssaoBlurPassProgram.setInt("irradianceMap", 6);

        gBuffer.render();

        // draw skybox as last only if we are not in the overlay mode
        if(deferredShadingDisplayId != 1) {
          // copy depth content to screen frameBuffer for additionnal rendering on top of shadingPass
          gBuffer.copyTo({0, 0}, {m_nWindowWidth, m_nWindowHeight}, GL_DEPTH_BUFFER_BIT, 0, GL_NEAREST);

          GLCALL(glDepthFunc(GL_LEQUAL));  // change depth function so depth test passes when values are equal to depth buffer's content
          cubeMapPassProgram.use();
          cubeMapPassProgram.setMat4("ViewMatrix", camera.getViewMatrix());
          cubeMapPassProgram.setMat4("ProjMatrix", projMatrix);
          skyBox.bindTexture(0);
          cubeMapPassProgram.setInt("cubeMap", 0);

          skyBox.draw();
          GLCALL(glDepthMask(GL_LESS)); // set depth function back to default
        }

      }else if (deferredShadingDisplayId <= 6) {
        // copy directly wanted texture without shadingPass
        gBuffer.copyToFromSlot({0, 0}, {m_nWindowWidth, m_nWindowHeight}, GL_COLOR_BUFFER_BIT, deferredShadingDisplayId-2);
      }else {
        // SSAO Occlusion
        SSAOBlurFB.copyToFromSlot({0, 0}, {m_nWindowWidth, m_nWindowHeight}, GL_COLOR_BUFFER_BIT);
      }

      // copy depth content to screen frameBuffer for additionnal rendering on top of shadingPass
      // gBuffer.copyTo({0, 0}, {m_nWindowWidth, m_nWindowHeight}, GL_DEPTH_BUFFER_BIT);
    }

    // GUI code:
    imguiNewFrame();

    {
      ImGui::Begin("GUI");
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::SliderFloat("frameRateLimit", &frameRateLimit, 1.f, 800.f, "%.0f", 3);
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
      }

      if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("theta", &lightTheta, 0, glm::pi<float>()) || ImGui::SliderFloat("phi", &lightPhi, 0, 2.f * glm::pi<float>())) {
          lightDirection = glm::vec3(glm::sin(lightTheta) * glm::cos(lightPhi), glm::cos(lightTheta), glm::sin(lightTheta) * glm::sin(lightPhi));
        }

        ImGui::ColorEdit3("color", (float *)&lightColor);
        ImGui::SliderFloat("intensity", &lightIntensity, 0.f, 10.f);
        ImGui::Checkbox("light from camera", &lightFromCamera);
      }

      if (ImGui::CollapsingHeader("Shading", ImGuiTreeNodeFlags_DefaultOpen)) {
        static const char* DeferredShadingModes[]{"All", "Overlay", "Position", "Normal", "Albedo", "Occlusion/Roughness/Metallic", "Emissive", "SSAO Occlusion"};
        ImGui::Combo("Deferred display textures", &deferredShadingDisplayId, DeferredShadingModes, 8);

        ImGui::Checkbox("enable occlusion", &occlusionEnable);
        ImGui::Checkbox("enable SSAO", &SSAOEnable);
        ImGui::Checkbox("enable IBL", &IBLEnable);
        
        if(tangentAvailable) ImGui::Checkbox("enable normal mapping", &normalEnable);
        if(SSAOEnable) ssaoFB.imguiMenu();
      }
      
      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    const double ellapsedTime = glfwGetTime() - seconds;

    if(glfwGetTime() - secondLastDraw > 1/frameRateLimit) {
      shouldDraw = true;
    }

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
    m_AssetsRootPath{m_AppPath.parent_path() / "assets"},
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

  // At exit, ImGUI will store its windows positions in this file
  ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); 

  glfwSetKeyCallback(m_GLFWHandle.window(), keyCallback);

  printGLVersion();
}