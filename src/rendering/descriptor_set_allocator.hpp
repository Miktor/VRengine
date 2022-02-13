#pragma once

#include <vector>
#include "common.hpp"

namespace vre::rendering {

struct DescriptorSetLayout;

class DescriptorSetAllocator {
 public:
  DescriptorSetAllocator(VkDevice device, const DescriptorSetLayout &layout);

  VkDescriptorSetLayout GetLayout() const { return descriptor_set_layout_; }

  VkDescriptorSet GetSet();

 private:
  VkDevice device_ = VK_NULL_HANDLE;

  std::vector<VkDescriptorPoolSize> pool_size_;

  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

  uint32_t index = 0;
  VkDescriptorPool pool_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptor_sets_;
};

}  // namespace vre::rendering