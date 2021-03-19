#pragma once

#include "utils/GLFWHandle.hpp"
#include "utils/cameraControllers.hpp"
#include "utils/filesystem.hpp"
#include "utils/shaders.hpp"

#include <tiny_gltf.h>

#include "enum.h"

BETTER_ENUM(EControllerType, int, Trackball, FirstPerson)

class ViewerApplication {
public:
  ViewerApplication(const fs::path &appPath, uint32_t width, uint32_t height,
      const fs::path &gltfFile, const std::vector<float> &lookatArgs,
      const std::string &vertexShader, const std::string &fragmentShader,
      const fs::path &output);

  int run();

private:
  // A range of indices in a vector containing Vertex Array Objects
  
  GLsizei m_nWindowWidth = 1280;
  GLsizei m_nWindowHeight = 720;

  float frameRateLimit = 60;
  
  const fs::path m_AppPath;
  const std::string m_AppName;
  const fs::path m_ShadersRootPath;

  fs::path m_gltfFilePath;
  std::string m_vertexShader = "pbr.vs.glsl";
  std::string m_fragmentShader = "pbr.fs.glsl";

  std::string geometryPassVShader = "geometryPass.vs.glsl";
  std::string geometryPassFShader = "geometryPass.fs.glsl";

  std::string shadingPassVShader = "shadingPass.vs.glsl";
  std::string shadingPassFShader = "shadingPass.fs.glsl";

  std::string ssaoPassVShader = "ssaoPass.vs.glsl";
  std::string ssaoPassFShader = "ssaoPass.fs.glsl";
  std::string ssaoBlurPassFShader = "ssaoBlurPass.fs.glsl";

  bool m_hasUserCamera = false;
  Camera m_userCamera;
  int cameraControllerType_ = static_cast<int>(EControllerType::FirstPerson);

  fs::path m_OutputPath;

  // Order is important here, see comment below
  const std::string m_ImGuiIniFilename;
  // Last to be initialized, first to be destroyed:
  // show the window only if m_OutputPath is empty
  GLFWHandle m_GLFWHandle{int(m_nWindowWidth), int(m_nWindowHeight), "glTF Viewer", m_OutputPath.empty()};
  /*
    ! THE ORDER OF DECLARATION OF MEMBER VARIABLES IS IMPORTANT !
    - m_ImGuiIniFilename.c_str() will be used by ImGUI in ImGui::Shutdown, which
    will be called in destructor of m_GLFWHandle. So we must declare
    m_ImGuiIniFilename before m_GLFWHandle so that m_ImGuiIniFilename
    destructor is called after.
    - m_GLFWHandle must be declared before the creation of any object managing
    OpenGL resources (e.g. GLProgram, GLShader) because it is responsible for
    the creation of a GLFW windows and thus a GL context which must exists
    before most of OpenGL function calls.
  */
  
};