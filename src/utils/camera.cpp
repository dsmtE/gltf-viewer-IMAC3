#include "camera.hpp"

Camera::Camera(const glm::vec3& e, const glm::vec3& c, const glm::vec3& u) : eye_(e), center_(c), up_(u) { 
    const glm::vec3 front = center_ - eye_;
    const glm::vec3 left = cross(up_, front);
    assert(left != glm::vec3(0));
    up_ = normalize(cross(front, left));
  };

void Camera::truckLeft(float offset) {
    const glm::vec3 front = center_ - eye_;
    const glm::vec3 left = normalize(cross(up_, front));
    const glm::vec3 translationVector = offset * left;
    eye_ += translationVector;
    center_ += translationVector;
}

void Camera::pedestalUp(float offset) {
    const glm::vec3 translationVector = offset * up_;
    eye_ += translationVector;
    center_ += translationVector;
}

void Camera::dollyIn(float offset) {
    const glm::vec3 front = normalize(center_ - eye_);
    const glm::vec3 translationVector = offset * front;
    eye_ += translationVector;
    center_ += translationVector;
}

void Camera::moveLocal(float truckLeftOffset, float pedestalUpOffset, float dollyIn) {
    const glm::vec3 front = normalize(center_ - eye_);
    const glm::vec3 left = normalize(cross(up_, front));
    const glm::vec3 translationVector =
        truckLeftOffset * left + pedestalUpOffset * up_ + dollyIn * front;
    eye_ += translationVector;
    center_ += translationVector;
}

void Camera::rollRight(float radians) {
    const glm::vec3 front = center_ - eye_;
    const glm::mat4 rollMatrix = glm::rotate(glm::mat4(1), radians, front);

    up_ = glm::vec3(rollMatrix * glm::vec4(up_, 0.f));
}

void Camera::tiltDown(float radians) {
    const glm::vec3 front = center_ - eye_;
    const glm::vec3 left = cross(up_, front);
    const glm::mat4 tiltMatrix = glm::rotate(glm::mat4(1), radians, left);

    const glm::vec3 newFront = glm::vec3(tiltMatrix * glm::vec4(front, 0.f));
    center_ = eye_ + newFront;
    up_ = glm::vec3(tiltMatrix * glm::vec4(up_, 0.f));
}

void Camera::panLeft(float radians) {
    const glm::vec3 front = center_ - eye_;
    const glm::mat4 panMatrix = glm::rotate(glm::mat4(1), radians, up_);

    const auto newFront = glm::vec3(panMatrix * glm::vec4(front, 0.f));
    center_ = eye_ + newFront;
}

void Camera::rotateLocal(float rollRight, float tiltDown, float panLeft) {
    const glm::vec3 front = center_ - eye_;
    const glm::mat4 rollMatrix = glm::rotate(glm::mat4(1), rollRight, front);

    up_ = glm::vec3(rollMatrix * glm::vec4(up_, 0.f));

    const glm::vec3 left = cross(up_, front);

    const glm::mat4 tiltMatrix = glm::rotate(glm::mat4(1), tiltDown, left);

    const auto newFront = glm::vec3(tiltMatrix * glm::vec4(front, 0.f));
    center_ = eye_ + newFront;

    up_ = glm::vec3(tiltMatrix * glm::vec4(up_, 0.f));

    const glm::mat4 panMatrix = glm::rotate(glm::mat4(1), panLeft, up_);

    const glm::vec3 newNewFront = glm::vec3(panMatrix * glm::vec4(newFront, 0.f));
    center_ = eye_ + newNewFront;
}

void Camera::rotateWorld(float radians, const glm::vec3 &axis) {
    const glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1), radians, axis);

    const glm::vec3 front = center_ - eye_;
    const glm::vec3 newFront = glm::vec3(rotationMatrix * glm::vec4(front, 0));
    center_ = eye_ + newFront;
    up_ = glm::vec3(rotationMatrix * glm::vec4(up_, 0));
}

const glm::vec3 Camera::front(const bool normalize) const {
    const auto f = center_ - eye_;
    return normalize ? glm::normalize(f) : f;
}

const glm::vec3 Camera::left(const bool normalize) const {
    const glm::vec3 f = front(false);
    const glm::vec3 l = glm::cross(up_, f);
    return normalize ? glm::normalize(l) : l;
}
