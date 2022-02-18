#pragma once

#include <vulkan/vulkan_core.h>
#include <memory>
#include <vector>
#include "common.hpp"

#include "rendering/descriptor_set_allocator.hpp"

namespace vre::rendering {

struct DescriptorSetLayout {
  struct UniformBuffer {
    uint32_t binding;
    std::string name;
  };
  std::vector<UniformBuffer> uniform_buffers;
};
struct ResourceLayout {
  struct Input {
    uint32_t location;
    uint32_t offset;
    uint32_t width;
    std::string name;
  };

  std::vector<Input> inputs;
  std::unordered_map<uint8_t, DescriptorSetLayout> descriptor_set_layouts;
};

struct ResourceBinding {
  VkDescriptorBufferInfo buffer_info;
  uint32_t dynamic_offset = 0;
};

using SetResourceBindings = std::vector<ResourceBinding>;
using ResourceBindings = std::unordered_map<uint8_t, SetResourceBindings>;

class Shader {
 public:
  enum Type {
    kVertex,
    kFragment,
  };

  Shader(VkDevice device, Type type, const std::string &path);
  ~Shader();

  Shader(Shader &) = delete;
  Shader(Shader &&) = delete;

  [[nodiscard]] VkShaderModule GetShaderModule() const { return shader_module_; }
  [[nodiscard]] const ResourceLayout &GetResourceLayout() const { return resource_layout_; }

 private:
  VkDevice device_;

  const std::string path_;
  const Type type_;

  ResourceLayout resource_layout_;
  VkShaderModule shader_module_;
};

struct CombinedResourceLayout {
  std::unordered_map<uint8_t, DescriptorSetLayout> descriptor_set_layouts;
};

class PipelineLayout {
 public:
  PipelineLayout(VkDevice device, const CombinedResourceLayout &resource_layout);
  ~PipelineLayout();

  PipelineLayout(PipelineLayout &) = delete;
  PipelineLayout(PipelineLayout &&) = delete;

  [[nodiscard]] DescriptorSetAllocator &GetDescriptorSetAllocator(uint32_t set) {
    VR_ASSERT(set < descriptor_set_allocators_.size());
    return *descriptor_set_allocators_[set];
  }

  [[nodiscard]] VkDescriptorUpdateTemplateKHR &GetUpdateTemplate(uint32_t set) {
    VR_ASSERT(set < descriptor_update_template_.size());
    return descriptor_update_template_[set];
  }

  [[nodiscard]] VkPipelineLayout GetPipelineLayout() const { return pipeline_layout_; }

 private:
  VkDevice device_;

  std::vector<std::unique_ptr<DescriptorSetAllocator>> descriptor_set_allocators_;
  std::vector<VkDescriptorUpdateTemplateKHR> descriptor_update_template_;

  VkPipelineLayout pipeline_layout_;
  CombinedResourceLayout resource_layout_;
};

class Material {
 public:
  Material(VkDevice device, std::shared_ptr<Shader> fragment, std::shared_ptr<Shader> vertex);
  ~Material();

  Material(Material &) = delete;
  Material(Material &&) = delete;

  [[nodiscard]] std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() const;
  [[nodiscard]] std::tuple<std::vector<VkVertexInputBindingDescription>,
                           std::vector<VkVertexInputAttributeDescription>>
  GetInputBindings() const;

  [[nodiscard]] PipelineLayout &GetPipelineLayout();

  [[nodiscard]] VkPipeline GetPipeline() { return pipeline_; }
  void SetPipeline(VkPipeline pipeline) {
    VR_ASSERT(pipeline_ == VK_NULL_HANDLE);
    pipeline_ = pipeline;
  }

 private:
  VkDevice device_;

  std::shared_ptr<Shader> fragment_;
  std::shared_ptr<Shader> vertex_;

  CombinedResourceLayout combined_resource_layout_;
  std::shared_ptr<PipelineLayout> pipeline_layout_;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

}  // namespace vre::rendering