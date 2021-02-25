#include "cameraControllers.hpp"
#include "glfw.hpp"

// #include <iostream>

struct ViewFrame{
  glm::vec3 left;
  glm::vec3 up;
  glm::vec3 front;
  glm::vec3 eye;

  ViewFrame(glm::vec3 l, glm::vec3 u, glm::vec3 f, glm::vec3 e) : left(l), up(u), front(f), eye(e) {}
};

ViewFrame fromViewToWorldMatrix(const glm::mat4 &viewToWorldMatrix) {
  return ViewFrame{-glm::vec3(viewToWorldMatrix[0]), glm::vec3(viewToWorldMatrix[1]),
      -glm::vec3(viewToWorldMatrix[2]), glm::vec3(viewToWorldMatrix[3])};
}

CameraController::CameraController(GLFWwindow* window, const float speed, const float sensitivity, const glm::vec3 &worldUpAxis) :
  pWindow_(window), fSpeed_(speed), sensitivity_(sensitivity), worldUpAxis_(worldUpAxis), camera_{glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)}{}

glm::dvec2 CameraController::getCursorDelta() {
  if (middleButtonPressed_) {
    glm::dvec2 cursorPosition;
    glfwGetCursorPos(pWindow_, &cursorPosition.x, &cursorPosition.y);
    const glm::dvec2 delta = cursorPosition - lastCursorPosition_;
    lastCursorPosition_ = cursorPosition;
    return delta;
  }
  return glm::dvec2(0);
};

FirstPersonCameraController::FirstPersonCameraController(GLFWwindow* window, const float speed, const float sensitivity, const glm::vec3 &worldUpAxis) :
  CameraController(window, speed, sensitivity, worldUpAxis)
  {}

TrackballCameraController::TrackballCameraController(GLFWwindow* window, const float speed, const float sensitivity, const glm::vec3 &worldUpAxis) :
  CameraController(window, speed, sensitivity, worldUpAxis)
  {}

bool FirstPersonCameraController::update(const float elapsedTime) {

  if (glfwGetMouseButton(pWindow_, GLFW_MOUSE_BUTTON_MIDDLE) && !middleButtonPressed_) {
    middleButtonPressed_ = true;
    glfwGetCursorPos(pWindow_, &lastCursorPosition_.x, &lastCursorPosition_.y);
  } else if (!glfwGetMouseButton(pWindow_, GLFW_MOUSE_BUTTON_MIDDLE) && middleButtonPressed_) {
    middleButtonPressed_ = false;
  }

  const glm::dvec2 cursorDelta = getCursorDelta();

  float truckLeft = 0.f;
  float pedestalUp = 0.f;
  float dollyIn = 0.f;
  float rollRightAngle = 0.f;

  if (glfwGetKey(pWindow_, GLFW_KEY_W)) { dollyIn += fSpeed_ * elapsedTime; } // Dolly in
  if (glfwGetKey(pWindow_, GLFW_KEY_S)) { dollyIn -= fSpeed_ * elapsedTime; } // Dolly out
  
  if (glfwGetKey(pWindow_, GLFW_KEY_A)) { truckLeft += fSpeed_ * elapsedTime; } // Truck left
  if (glfwGetKey(pWindow_, GLFW_KEY_D)) { truckLeft -= fSpeed_ * elapsedTime; } // Truck right
  
  if (glfwGetKey(pWindow_, GLFW_KEY_SPACE)) { pedestalUp += fSpeed_ * elapsedTime; } // Pedestal up
  if (glfwGetKey(pWindow_, GLFW_KEY_LEFT_SHIFT)) { pedestalUp -= fSpeed_ * elapsedTime; } // Pedestal down
  
  if (glfwGetKey(pWindow_, GLFW_KEY_Q)) { rollRightAngle -= 0.001f; }
  if (glfwGetKey(pWindow_, GLFW_KEY_E)) { rollRightAngle += 0.001f; }

  // cursor going right, so minus because we want pan left angle:
  const float panLeftAngle = -0.01f * float(cursorDelta.x);
  const float tiltDownAngle = 0.01f * float(cursorDelta.y);

  const bool hasMoved = truckLeft || pedestalUp || dollyIn || panLeftAngle || tiltDownAngle || rollRightAngle;
  if (!hasMoved) { return false; }

  camera_.moveLocal(truckLeft, pedestalUp, dollyIn);
  camera_.rotateLocal(rollRightAngle, tiltDownAngle, 0.f);
  camera_.rotateWorld(panLeftAngle, worldUpAxis_);

  return true;
}

bool TrackballCameraController::update(float elapsedTime) {

  if (glfwGetMouseButton(pWindow_, GLFW_MOUSE_BUTTON_MIDDLE) && !middleButtonPressed_) {
    middleButtonPressed_ = true;
    glfwGetCursorPos(pWindow_, &lastCursorPosition_.x, &lastCursorPosition_.y);
  } else if (!glfwGetMouseButton(pWindow_, GLFW_MOUSE_BUTTON_MIDDLE) && middleButtonPressed_) {
    middleButtonPressed_ = false;
  }

  const glm::vec2 cursorDelta = getCursorDelta();

  // Pan
  if (glfwGetKey(pWindow_, GLFW_KEY_LEFT_SHIFT)) {
    const float truckLeft = sensitivity_ * cursorDelta.x;
    const float pedestalUp = sensitivity_ * cursorDelta.y;
    const bool hasMoved = truckLeft || pedestalUp;
    if (!hasMoved) { return false; }

    camera_.moveLocal(truckLeft, pedestalUp, 0.f);

    return true;
  }

  // Zoom
  if (glfwGetKey(pWindow_, GLFW_KEY_LEFT_CONTROL)) {
    const float mouseOffset = sensitivity_ * cursorDelta.x;
    if (mouseOffset == 0.f) { return false; }

    camera_.moveLocal(0.f, 0.f, mouseOffset);

    return true;
  }

  // Rotate around target
  const float longitudeAngle = sensitivity_ * float(cursorDelta.y); // Vertical angle
  const float latitudeAngle = -sensitivity_ * float(cursorDelta.x); // Horizontal angle
  const bool hasMoved = longitudeAngle || latitudeAngle;
  if (!hasMoved) { return false; }

  // We need to rotate eye around center, for that we rotate 
  // the vector [center, eye] (= depthAxis) in order to compute a new eye position
  const glm::vec3 centerToEye = camera_.eye() - camera_.center();

  const glm::mat4 latitudeRotMat = glm::rotate(glm::mat4(1), latitudeAngle, worldUpAxis_);

  const glm::mat4 longitudeRotMat = glm::rotate(glm::mat4(1), longitudeAngle, camera_.left());
  const glm::vec3 rotatedCenterToEye = glm::vec3(longitudeRotMat * latitudeRotMat * glm::vec4(centerToEye, 0));

  camera_.eye() = camera_.center() + rotatedCenterToEye;// Update camera eye
  return true;
}
