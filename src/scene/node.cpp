#include "node.hpp"

#include "rendering/mesh.hpp"

namespace vre::scene {

Node &Node::CreateChildNode(std::string name) {
  auto node = std::make_unique<Node>();
  node->parent = this;
  node->name_ = std::move(name);
  childrens_.push_back(std::move(node));
  return *childrens_.back();
}

Attachable &Node::Attach(std::unique_ptr<Attachable> &&attachable) {
  attachable->SetParrent(this);
  attachables_.push_back(std::move(attachable));
  return *attachables_.back();
}

glm::mat4 Node::GetTransform() {
  auto transform = glm::translate(glm::mat4(1.0f), transform_.position);
  transform *= glm::scale(transform, transform_.scale);
  transform *= glm::toMat4(transform_.rotation);
  return transform;
}

}  // namespace vre::scene