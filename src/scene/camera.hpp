#pragma once

#include "common.hpp"

#include "scene/attachable.hpp"

namespace vre::scene {

class Camera : public Attachable {
 public:
  Camera();

  glm::mat4 GetView() const;
  glm::mat4 GetProjection() const;

 private:
  float fow_ = 60.0F;
};

}  // namespace vre::scene