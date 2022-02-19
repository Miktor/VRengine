#pragma once

#include <memory>
#include "common.hpp"

#include "rendering/buffers.hpp"
#include "rendering/command_buffer.hpp"
#include "rendering/image.hpp"
#include "rendering/render_pass.hpp"
#include "rendering/uniform_buffer_allocator.hpp"

namespace vre::rendering {

struct PhysicalDeviceContext {
  VkPhysicalDevice device;
  VkSurfaceKHR surface;

  std::set<std::string> required_extensions;

  struct QueueFamilyIndices {
    uint32_t graphics_family;
    uint32_t present_family;
  } indices;

  VkSurfaceCapabilitiesKHR surface_capabilities;
  std::vector<VkSurfaceFormatKHR> surface_formats;
  std::vector<VkPresentModeKHR> present_modes;
};

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

struct RenderData {
  glm::mat4 camera_view;
  glm::mat4 camera_projection;
};

struct RenderContext {
  RenderContext() = default;
  RenderContext(RenderContext &) = delete;
  RenderContext(RenderContext &&) = default;

  std::unique_ptr<CommandBuffer> command_buffer;

  VkSemaphore image_available_semaphore;
  VkSemaphore render_finished_semaphore;

  VkFence in_flight_fence;
  VkFence images_in_flight;

  RenderData render_data;
};

class RenderCore {
 private:
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;

  PhysicalDeviceContext physical_device_;

  VkDevice device_ = VK_NULL_HANDLE;

  VmaAllocator vma_allocator_;

  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;

  VkPipelineCache pipeline_cache_;

  VkSwapchainKHR swap_chain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swap_chain_images_;
  VkFormat swap_chain_image_format_ = VkFormat::VK_FORMAT_UNDEFINED;
  VkExtent2D swap_chain_extent_{};

  std::vector<Image> backbuffers_;
  std::vector<std::shared_ptr<Framebuffer>> framebuffers_;
  std::shared_ptr<RenderPass> render_pass_;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;

  std::vector<VkCommandBuffer> command_buffers_;

  std::vector<VkSemaphore> image_available_semaphores_;
  std::vector<VkSemaphore> render_finished_semaphores_;
  std::vector<VkFence> in_flight_fences_;
  std::vector<VkFence> images_in_flight_;

  size_t current_frame_ = 0;
  uint32_t next_image_index_ = 0;

  std::unique_ptr<UniformBufferPoolAllocator> ubo_allocator_;

 public:
  RenderCore();

  void InitVulkan(GLFWwindow *window);

  VkDevice GetDevice() { return device_; }
  VmaAllocator GetVmaAllocator() { return vma_allocator_; }

  void Cleanup();
  void CleanupSwapChain();

  UniformBufferPoolAllocator &GetUniformBufferPoolAllocator();
  std::shared_ptr<Buffer> CreateBuffer(const CreateBufferInfo &crate_info);

  RenderContext BeginDraw();
  void Present(RenderContext &context);

  void WaitDeviceIdle();

 private:
  void InitPipelineCache();
  void SaveAndDestroyPipelineCache();

  void CreateSwapChain(GLFWwindow *window);
  void CreateImageViews();
  void CreateSyncObjects();
};

}  // namespace vre::rendering