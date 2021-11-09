#pragma once

#include "common.hpp"

namespace vre {

namespace rendering {
class RenderCore;
}

namespace scene {

struct Node;

class Scene {
 public:
  void LoadFromFile();
  void Initialize();
  void InitializeVulkan(rendering::RenderCore &renderer);

  void Update();
  void Render(VkCommandBuffer command_buffers, VkPipelineLayout pipeline_layout, VkDescriptorSet descriptor_set);

 private:
  std::vector<std::shared_ptr<Node>> root_nodes_;
};

}  // namespace scene
}  // namespace vre