#pragma once

#include "common.hpp"

#include "rendering/shader.hpp"

namespace vre::rendering {

class DescriptorSetAllocator {
 public:
  DescriptorSetAllocator(VkDevice device, const DescriptorSetLayout &layout) : device_(device) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto &ubo : layout.uniform_buffers) {
      VkDescriptorSetLayoutBinding binding{};
      binding.binding = ubo.binding;
      binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      binding.descriptorCount = 1;
      binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      binding.pImmutableSamplers = nullptr;
      bindings.push_back(std::move(binding));
    }

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = bindings.size();
    layout_info.pBindings = bindings.data();
    CHECK_VK_SUCCESS(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout_));
  }

 private:
  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
};

}  // namespace vre::rendering