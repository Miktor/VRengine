#pragma once

#include "common.hpp"


namespace vre::rendering {
class Mesh;
}

namespace vre::scene {

struct Node {
  std::shared_ptr<Node> parent;

  std::string name_;

  glm::vec3 translation;
  glm::quat rotation;
  glm::vec3 scale;

  std::vector<std::shared_ptr<Node>> childrens_;
  std::shared_ptr<rendering::Mesh> mesh_;
};

}  // namespace vre::scene