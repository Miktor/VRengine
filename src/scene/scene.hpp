#pragma once

#include "common.hpp"

namespace vre {

namespace rendering {
struct RenderContext;
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
  void Render(rendering::RenderContext& context);

 private:
  std::vector<std::shared_ptr<Node>> root_nodes_;
};

}  // namespace scene
}  // namespace vre