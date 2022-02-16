#pragma once

#include <memory>
#include "common.hpp"

#include "scene/camera.hpp"
#include "scene/node.hpp"

namespace vre {

namespace rendering {
struct RenderContext;
class RenderCore;
}  // namespace rendering

namespace scene {

class Scene {
 public:
  void LoadFromFile();
  void CreateCamera();
  void InitializeVulkan(rendering::RenderCore &renderer);

  void Update();
  void Render(rendering::RenderContext &context);

  void Cleanup();

  Node &GetRootNode();
  Camera &GetMainCamera();
  Node &GetMainCameraNode();

 private:
  Node *main_camera_node_ = nullptr;
  Camera *main_camera_ = nullptr;

  std::unique_ptr<Node> root_node_;
};

}  // namespace scene
}  // namespace vre