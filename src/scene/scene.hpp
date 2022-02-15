#pragma once

#include "common.hpp"

#include "scene/camera.hpp"

namespace vre {

namespace rendering {
struct RenderContext;
class RenderCore;
}  // namespace rendering

namespace scene {

struct Node;

class Scene {
 public:
  void LoadFromFile();
  void CreateCamera();
  void InitializeVulkan(rendering::RenderCore &renderer);

  void Update();
  void Render(rendering::RenderContext &context);

   void Cleanup();

  std::shared_ptr<Node> GetRootNode();

  std::shared_ptr<Node> main_camera_node_;
  std::shared_ptr<Camera> main_camera_;

 private:
  std::vector<std::shared_ptr<Node>> root_nodes_;
};

}  // namespace scene
}  // namespace vre