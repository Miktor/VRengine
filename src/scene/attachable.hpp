#pragma once

namespace vre::scene {

class Node;

class Attachable {
 public:
  virtual ~Attachable() = default;

  void SetParrent(Node *parent) { parent_ = parent; }

  [[nodiscard]] Node *GetParrent() { return parent_; }
  [[nodiscard]] const Node *GetParrent() const { return parent_; }

 private:
  Node *parent_ = nullptr;
};

}  // namespace vre::scene