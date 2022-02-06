#pragma once

#include "common.hpp"

#include "rendering/pipeline.hpp"

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

class Shader {
 public:
  enum Type {
    kVertex,
    kFragment,
  };

  Shader(VkDevice device, Type type, const std::string &path);

  VkShaderModule GetShaderModule() const { return shader_module_; }

  const ResourceLayout &GetResourceLayout() const { return resource_layout_; }

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

  const std::vector<VkDescriptorSetLayout> &GetDescriptorSetLayouts() const {
    return descriptor_set_layouts_;
  }
  VkPipelineLayout GetPipelineLayout() const { return pipeline_layout_; }

 private:
  VkDevice device_;
  std::vector<VkDescriptorSetLayout> descriptor_set_layouts_;
  VkPipelineLayout pipeline_layout_;
  CombinedResourceLayout resource_layout_;
};

class Material {
 public:
  Material(VkDevice device, Shader &&fragment, Shader &&vertex);

  std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() const;
  std::tuple<std::vector<VkVertexInputBindingDescription>, std::vector<VkVertexInputAttributeDescription>>
  GetInputBindings() const;

  PipelineLayout &GetPipelineLayout() const;

 private:
  VkDevice device_;

  Shader fragment_;
  Shader vertex_;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

  CombinedResourceLayout combined_resource_layout_;
  std::shared_ptr<PipelineLayout> pipeline_layout_;

 private:
  VkDescriptorSetLayout GetDescriptorSetLayout();
};

}  // namespace vre::rendering