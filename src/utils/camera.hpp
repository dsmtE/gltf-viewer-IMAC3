#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {

private:
  glm::vec3 eye_;
  glm::vec3 center_;
  glm::vec3 up_;

public:
  Camera() = default;

  Camera(const glm::vec3& e,  const glm::vec3& c, const glm::vec3& u);

  glm::mat4 getViewMatrix() const { return glm::lookAt(eye_, center_, up_); }

  // Move the camera along its left axis.
  void truckLeft(float offset);

  void pedestalUp(float offset);

  void dollyIn(float offset);

  void moveLocal(float truckLeftOffset, float pedestalUpOffset, float dollyIn);

  void rollRight(float radians);
  void tiltDown(float radians);

  void panLeft(float radians);

  // All angles in radians
  void rotateLocal(float rollRight, float tiltDown, float panLeft);

  // Rotate around a world axis but keep the same position
  void rotateWorld(float radians, const glm::vec3 &axis);

  inline const glm::vec3 eye() const { return eye_; }
  inline const glm::vec3 center() const { return center_; }
  inline const glm::vec3 up() const { return up_; }

  const glm::vec3 front(const bool normalize = true) const;

  const glm::vec3 left(const bool normalize = true) const;
};
