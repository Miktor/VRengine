#pragma once

#include <memory>
#include <utility>

#include "common.hpp"
#include "scene/attachable.hpp"

namespace vre::rendering {
class Mesh;
} // namespace vre::rendering

namespace vre::scene {

struct Transform {
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;
};

struct Node {
 public:
  void Update() {}

  std::shared_ptr<Node> CreateChildNode(std::string name) {
    auto node = std::make_shared<Node>();
    node->parent = this;
    node->name_ = std::move(name);

    return node;
  }

  void Attach(const std::shared_ptr<Attachable> &attachable) {
    attachables_.push_back(attachable);
    attachable->SetParrent(this);
  }

 public:  // TODO: make private
  Node *parent;

  std::string name_;

  Transform transform_;
  glm::mat4 cached_transform_;

  std::vector<std::shared_ptr<Node>> childrens_;

  std::vector<std::shared_ptr<Attachable>> attachables_;
  std::shared_ptr<rendering::Mesh> mesh_;
};

}  // namespace vre::scene