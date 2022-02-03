#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace vre::rendering {

struct ReflectionData {
  struct Input {
    uint32_t location;
    uint32_t offset;
    uint32_t width;
    std::string name;
  };

  struct UniformBuffer {
    uint32_t set;
    uint32_t binding;
    uint32_t size;
    std::string name;
  };

  std::vector<Input> inputs;
  std::vector<UniformBuffer> uniform_buffers;
};

class Shader {
 public:
  enum Type {
    kVertex,
    kFragment,
  };

  Shader(VkDevice device, Type type, const std::string &path);

  VkShaderModule GetShaderModule() const { return shader_module_; }

  const std::vector<ReflectionData::Input> &GetInputs() const { return reflection_data_.inputs; }
  const std::vector<ReflectionData::UniformBuffer> &GetUniformBuffers() const { return reflection_data_.uniform_buffers; }

 private:
  VkDevice device_;

  const std::string path_;
  const Type type_;

  ReflectionData reflection_data_;

  VkShaderModule shader_module_;
};

class Material {
 public:
  Material(VkDevice device,Shader &&fragment, Shader &&vertex);

  std::vector<VkPipelineShaderStageCreateInfo> GetShaderStages() const;
  std::tuple<std::vector<VkVertexInputBindingDescription>, std::vector<VkVertexInputAttributeDescription>> GetInputBindings() const;

  VkPipelineLayout GetPipelineLayout() const;

 private:
  VkDevice device_;

  Shader fragment_;
  Shader vertex_;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;

 private:
  VkDescriptorSetLayout GetDescriptorSetLayout();
};

}  // namespace vre::rendering