#pragma once

#include "common.hpp"

#include "rendering/buffers.hpp"
#include "rendering/render_core.hpp"
#include "scene/scene.hpp"

namespace vre {

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities_{};
  std::vector<VkSurfaceFormatKHR> formats_;
  std::vector<VkPresentModeKHR> present_modes_;
};

struct Vertex {
  glm::vec3 pos_;

  static VkVertexInputBindingDescription GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

class Application {
 public:
  void Run();

  virtual bool ProcessInput(GLFWindow *window, int key, int scancode, int action, int mods);

 protected:
  GLFWindow *window_ = nullptr;

  rendering::RenderCore render_core_;

  scene::Scene main_scene_;

  virtual void PreInit() {}
  virtual void Cleanup();
  virtual void DrawRenderPass(VkCommandBuffer command_buffers) {}
  virtual void PreDrawFrame(uint32_t image_index) {}
  virtual void CrateBuffers() {}

 private:
  void InitWindow();
  void MainLoop();
};

}  // namespace vre