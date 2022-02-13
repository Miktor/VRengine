#include "camera.hpp"

#include "scene/node.hpp"

namespace vre::scene {

Camera::Camera() {}

glm::mat4 Camera::GetView() const {
  const auto &transform = GetParrent()->transform_;

  return glm::lookAt(transform.position, transform.position + GetForward(), GetUp());
}

glm::mat4 Camera::GetProjection() const {
  auto proj = glm::perspective(glm::radians(fow_), 1.0F, 0.01F, 100.0F);
  proj[1][1] *= -1;

  return proj;
}

[[nodiscard]] glm::vec3 Camera::GetForward() const {
  SPDLOG_INFO("fwd: {}", glm::to_string(GetParrent()->transform_.rotation * glm::vec3(0.0F, 0.0F, -1.0F)));
  return GetParrent()->transform_.rotation * glm::vec3(0.0F, 0.0F, -1.0F);
}

[[nodiscard]] glm::vec3 Camera::GetLeft() const {
  return GetParrent()->transform_.rotation * glm::vec3(-1.0F, 0.0F, 0.0F);
}

[[nodiscard]] glm::vec3 Camera::GetUp() const { return glm::vec3(0.0F, 0.0F, 1.0F); }

}  // namespace vre::scene