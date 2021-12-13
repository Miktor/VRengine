#include "camera.hpp"

#include "scene/node.hpp"

namespace vre::scene {

Camera::Camera() {}

glm::mat4 Camera::GetView() const {
  const auto &transform = GetParrent()->transform_;

  return glm::inverse(glm::translate(glm::toMat4(transform.rotation), transform.position));
}

glm::mat4 Camera::GetProjection() const {
  auto proj = glm::perspective(glm::radians(fow_), 1.0F, 0.01F, 100.0F);
  proj[1][1] *= -1;

  return proj;
}

}  // namespace vre::scene