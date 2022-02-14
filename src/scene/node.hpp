#pragma once

#include <memory>
#include <utility>

#include "common.hpp"
#include "scene/attachable.hpp"

namespace vre::rendering {
class Mesh;
}  // namespace vre::rendering

namespace vre::scene {

struct Transform {
  glm::vec3 position = glm::vec3(0.0f);
  glm::quat rotation = glm::identity<glm::quat>();
  glm::vec3 scale = glm::vec3(1.0f);
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

  [[nodiscard]] glm::mat4 GetTransform() {
    auto transform = glm::translate(glm::mat4(1.0f), transform_.position);
    transform = glm::scale(transform, transform_.scale);
    transform *= glm::toMat4(transform_.rotation);
    return transform;
  }

 public:  // TODO: make private
  Node *parent;

  std::string name_;

  Transform transform_;

  std::vector<std::shared_ptr<Node>> childrens_;

  std::vector<std::shared_ptr<Attachable>> attachables_;
  std::shared_ptr<rendering::Mesh> mesh_;
};

}  // namespace vre::scene