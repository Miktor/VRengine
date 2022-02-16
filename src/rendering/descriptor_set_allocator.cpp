#include "descriptor_set_allocator.hpp"

#include "rendering/shader.hpp"

namespace vre::rendering {

constexpr uint32_t kSetCount = 3;

DescriptorSetAllocator::DescriptorSetAllocator(VkDevice device, const DescriptorSetLayout &layout)
    : device_(device) {
  // TODO: calculate count
  constexpr uint32_t kDescriptorCount = 1;

  std::vector<VkDescriptorSetLayoutBinding> bindings;

  for (const auto &ubo : layout.uniform_buffers) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = ubo.binding;
    binding.descriptorCount = kDescriptorCount;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    binding.pImmutableSamplers = nullptr;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings.push_back(std::move(binding));

    pool_size_.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kDescriptorCount * kSetCount});
  }

  VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layout_info.bindingCount = bindings.size();
  layout_info.pBindings = bindings.data();
  CHECK_VK_SUCCESS(vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_));
}

DescriptorSetAllocator::~DescriptorSetAllocator() {
  vkDestroyDescriptorPool(device_, pool_, nullptr);
  vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
}

VkDescriptorSet DescriptorSetAllocator::GetSet() {
  if (descriptor_sets_.empty()) {
    VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    info.maxSets = kSetCount;
    if (!pool_size_.empty()) {
      info.poolSizeCount = pool_size_.size();
      info.pPoolSizes = pool_size_.data();
    }

    CHECK_VK_SUCCESS(vkCreateDescriptorPool(device_, &info, nullptr, &pool_));

    VkDescriptorSetLayout layouts[kSetCount];
    std::fill(std::begin(layouts), std::end(layouts), descriptor_set_layout_);

    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = pool_;
    alloc.descriptorSetCount = kSetCount;
    alloc.pSetLayouts = layouts;

    descriptor_sets_.resize(kSetCount);
    CHECK_VK_SUCCESS(vkAllocateDescriptorSets(device_, &alloc, descriptor_sets_.data()));
  }

  auto set = descriptor_sets_[index];
  index = (index + 1) & (kSetCount - 1);
  return set;
}

}  // namespace vre::rendering