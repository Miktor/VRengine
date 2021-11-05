#pragma once

#include "common.hpp"

namespace vre {

class Application;

namespace scene {

struct Node;

class Scene {
 public:
  void LoadFromFile();
  void Initialize();
  void InitializeVulkan(Application &app);

  void Update();
  void Render(VkCommandBuffer command_buffers);

 private:
  std::vector<std::shared_ptr<Node>> root_nodes_;
};

}  // namespace scene
}  // namespace vre