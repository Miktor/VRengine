#include <vulkan/vulkan_core.h>
#include <unordered_set>
#include "common.hpp"

namespace vre::scene {

class CommandPool {
 public:
  CommandPool(VkCommandPool command_pool);

  CommandPool(CommandPool &) = delete;
  CommandPool(CommandPool &&) = default;

  VkCommandBuffer GetCommandBuffer();

 private:
  VkDevice device_;
  VkCommandPool command_pool_;

  std::unordered_set<VkCommandBuffer> ready_buffers_;
  std::unordered_set<VkCommandBuffer> in_use_buffers_;

 private:
  VkCommandBuffer CreateCommandBuffer();
  
};
}  // namespace vre::scene