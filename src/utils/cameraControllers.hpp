#pragma once

#include "camera.hpp"

// Good reference here to map camera movements to lookAt calls
// http://learnwebgl.brown37.net/07_cameras/camera_movement.html

// Forward Declaration
struct GLFWwindow;

class CameraController {
protected:
  GLFWwindow *pWindow_ = nullptr;
  float fSpeed_ = 0.f;
  float sensitivity_ = 0.01f;
  glm::vec3 worldUpAxis_;

  // Input event state
  bool middleButtonPressed_ = false;
  glm::dvec2 lastCursorPosition_;

  Camera camera_;

public:
  CameraController(GLFWwindow *window, const float speed = 0.1f, const float sensitivity = 0.01f, const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0));
  virtual ~CameraController() {};

  // Setters
  inline void setSpeed(const float speed) { fSpeed_ = speed; }
  inline void setWorldUpAxis(const glm::vec3 &worldUpAxis) { worldUpAxis_ = worldUpAxis; }
  inline void setCamera(const Camera &camera) { camera_ = camera; }

  // Getters 
  inline float getSpeed() const { return fSpeed_; }
  const glm::vec3 &getWorldUpAxis() const { return worldUpAxis_; }
  inline const Camera &getCamera() const { return camera_; }

  void increaseSpeed(float delta) {
    fSpeed_ += delta;
    fSpeed_ = glm::max(fSpeed_, 0.f);
  }

  glm::dvec2 getCursorDelta();

  // Update the view matrix based on input events and elapsed time
  // Return true if the view matrix has been modified
  virtual bool update(const float elapsedTime) = 0;
};

class FirstPersonCameraController : public CameraController {
public:
  FirstPersonCameraController(GLFWwindow *window, const float speed = 0.1f, const float sensitivity = 0.01f, const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0));

  bool update(const float elapsedTime) override;
};

// todo Blender like camera
class TrackballCameraController: public CameraController {
public:

  TrackballCameraController(GLFWwindow *window, const float speed = 0.1f, const float sensitivity = 0.01f, const glm::vec3 &worldUpAxis = glm::vec3(0, 1, 0));

  bool update(const float elapsedTime) override;
};