#include "camera.hpp"

#include "common.hpp"

#include "scene/node.hpp"

namespace vre::scene {

Camera::Camera() {}

void Camera::AddYaw(float yaw) {
  yaw_ += yaw;
  SPDLOG_INFO("yaw_: {}", yaw_);
}

void Camera::AddPitch(float pitch) {
  pitch_ += pitch;
  SPDLOG_INFO("pitch_: {}", pitch_);
}

glm::mat4 Camera::GetView() const {
  const auto &transform = GetParrent()->transform_;

  glm::quat rotation =
      glm::angleAxis(glm::radians(pitch_), GetLeft()) * glm::angleAxis(glm::radians(yaw_), GetUp());

  return glm::toMat4(glm::normalize(rotation)) * glm::translate(glm::mat4(1.0f), transform.position);
}

glm::mat4 Camera::GetProjection() const {
  const glm::mat4 clip(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
                       0.5f, 1.0f);

  return clip * glm::perspective(glm::radians(fow_), 1.0F, 0.01F, 100.0F);
}

[[nodiscard]] glm::vec3 Camera::GetRelativeForward() const {
  return glm::conjugate(glm::angleAxis(glm::radians(pitch_), GetLeft()) *
                        glm::angleAxis(glm::radians(yaw_), GetUp())) *
         GetForward();
}

[[nodiscard]] glm::vec3 Camera::GetRelativeLeft() const {
  return glm::conjugate(glm::angleAxis(glm::radians(pitch_), GetLeft()) *
                        glm::angleAxis(glm::radians(yaw_), GetUp())) *
         GetLeft();
}

[[nodiscard]] glm::vec3 Camera::GetForward() const { return glm::vec3(0.0F, 0.0F, 1.0F); }

[[nodiscard]] glm::vec3 Camera::GetLeft() const { return glm::vec3(1.0F, 0.0F, 0.0F); }

[[nodiscard]] glm::vec3 Camera::GetUp() const { return glm::vec3(0.0F, -1.0F, 0.0F); }

}  // namespace vre::scene