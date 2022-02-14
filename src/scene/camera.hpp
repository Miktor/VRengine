#pragma once

#include "common.hpp"

#include "scene/attachable.hpp"

namespace vre::scene {

class Camera : public Attachable {
 public:
  Camera();

  // TODO: use quats for this
  void AddYaw(float yaw);
  void AddPitch(float pitch);

  [[nodiscard]] glm::mat4 GetView() const;
  [[nodiscard]] glm::mat4 GetProjection() const;

  [[nodiscard]] glm::vec3 GetRelativeForward() const;
  [[nodiscard]] glm::vec3 GetRelativeLeft() const;

  [[nodiscard]] glm::vec3 GetForward() const;
  [[nodiscard]] glm::vec3 GetLeft() const;
  [[nodiscard]] glm::vec3 GetUp() const;

 private:
  float fow_ = 45.0F;
  float yaw_ = .0F;
  float pitch_ = .0F;
};

}  // namespace vre::scene