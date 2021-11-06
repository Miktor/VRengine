#pragma once

#include "common.hpp"

namespace vre::rendering {

class IndexBuffer;
class VertexBuffer;

struct QueueFamilyIndices;

class RenderCore {
 private:
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;

  VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swap_chain_images_;
  VkFormat swap_chain_image_format_ = VkFormat::VK_FORMAT_UNDEFINED;
  VkExtent2D swap_chain_extent_{};
  std::vector<VkImageView> swap_chain_image_views_;
  std::vector<VkFramebuffer> swap_chain_framebuffers_;

  VkRenderPass render_pass_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
  VkPipeline graphics_pipeline_ = VK_NULL_HANDLE;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;

  std::vector<VkCommandBuffer> command_buffers_;

  std::vector<VkSemaphore> image_available_semaphores_;
  std::vector<VkSemaphore> render_finished_semaphores_;
  std::vector<VkFence> in_flight_fences_;
  std::vector<VkFence> images_in_flight_;

  size_t current_frame_ = 0;
  uint32_t next_image_index_ = 0;

 public:
  void InitVulkan(GLFWindow *window);

  void Cleanup();
  void CleanupSwapChain();

  std::shared_ptr<IndexBuffer> CreateIndexBuffer(const std::vector<uint32_t> &indices);
  std::shared_ptr<VertexBuffer> CreateVertexBuffer(const std::vector<glm::vec3> &vertexes);

  VkCommandBuffer BeginDraw();
  void Present(VkCommandBuffer command_buffer);

  void WaitDeviceIdle();

 private:
  void CreateSwapChain(GLFWindow *window, const QueueFamilyIndices &indices);
  void CreateImageViews();
  void CreateRenderPass();
  void CreateFramebuffers();
  void CreateSyncObjects();
};

}  // namespace vre::rendering