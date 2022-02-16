#pragma once

#include <memory>
#include <utility>

#include "common.hpp"
#include "rendering/mesh.hpp"
#include "scene/attachable.hpp"

namespace vre::scene {

struct Transform {
  glm::vec3 position = glm::vec3(0.0f);
  glm::quat rotation = glm::identity<glm::quat>();
  glm::vec3 scale = glm::vec3(1.0f);
};

struct Node {
 public:
  void Update() {}

  [[nodiscard]] Node &CreateChildNode(std::string name);

  Attachable &Attach(std::unique_ptr<Attachable> &&attachable);

  [[nodiscard]] glm::mat4 GetTransform();

 public:  // TODO: make private
  Node *parent;

  std::string name_;

  Transform transform_;

  std::vector<std::unique_ptr<Node>> childrens_;

  std::vector<std::unique_ptr<Attachable>> attachables_;
  std::unique_ptr<rendering::Mesh> mesh_;
};

}  // namespace vre::scene