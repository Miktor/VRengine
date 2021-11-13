#include "shader.hpp"
#include <vulkan/vulkan_core.h>

#include <iostream>
#include <stdexcept>
#include <utility>

#include <shaderc/shaderc.hpp>
#include <spirv-cross/spirv_cross.hpp>

#include "common.hpp"
#include "platform/platform.hpp"

namespace vre::rendering {

namespace {

shaderc_shader_kind ToShadercType(Shader::Type type) {
  switch (type) {
    case Shader::kVertex:
      return shaderc_glsl_vertex_shader;
    case Shader::kFragment:
      return shaderc_glsl_fragment_shader;
  }
}

shaderc::SpvCompilationResult Compile(const std::string &data, const std::string &path, Shader::Type type) {
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  // options.SetOptimizationLevel(shaderc_optimization_level_size);

  auto result = compiler.CompileGlslToSpv(data, ToShadercType(type), path.data(), options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    throw std::runtime_error(result.GetErrorMessage());
  }

  return result;
}

ReflectionData GetReflectionData(std::vector<uint32_t> &&spirv) {
  spirv_cross::Compiler compiler(std::move(spirv));

  auto resources = compiler.get_shader_resources();
  ReflectionData result;

  for (auto &resource : resources.stage_inputs) {
    ReflectionData::Input input{};
    input.name = resource.name;
    input.location = compiler.get_decoration(resource.id, spv::DecorationLocation);

    const auto &type = compiler.get_type(resource.base_type_id);
    input.width = type.width * type.vecsize * type.columns / 8;

    result.inputs.push_back(input);
  }

  for (auto &resource : resources.uniform_buffers) {
    unsigned set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    unsigned binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
    SPDLOG_INFO("UB {} at set = {}, binding = {}", resource.name.c_str(), set, binding);

    ReflectionData::UniformBuffer ub{};
    ub.name = resource.name;
    ub.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    ub.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

    result.uniform_buffers.push_back(ub);
  }

  return result;
}

std::vector<VkVertexInputBindingDescription> GetBindingDescription(const Shader &vertex) {
  std::vector<VkVertexInputBindingDescription> result;

  for (const auto &input : vertex.GetInputs()) {
    VkVertexInputBindingDescription binding_description{};

    binding_description.binding = 0;
    binding_description.stride = input.width;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    result.push_back(binding_description);
  }

  return result;
}

std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(const Shader &vertex) {
  std::vector<VkVertexInputAttributeDescription> result;

  for (const auto &input : vertex.GetInputs()) {
    VkVertexInputAttributeDescription attribute_description{};

    attribute_description.binding = 0;
    attribute_description.location = input.location;
    attribute_description.format = VK_FORMAT_R32G32_SFLOAT;
    attribute_description.offset = input.offset;

    result.push_back(attribute_description);
  }

  return result;
}

std::vector<VkDescriptorSetLayoutBinding> GetLayoutBindings(const Shader &vertex) {
  std::vector<VkDescriptorSetLayoutBinding> result;

  for (const auto &ubo : vertex.GetUniformBuffers()) {
    VkDescriptorSetLayoutBinding layout_binding{};

    layout_binding.binding = ubo.binding;
    layout_binding.descriptorCount = 1;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    result.push_back(layout_binding);
  }

  return result;
}

}  // namespace

Shader::Shader(VkDevice device, Type type, const std::string &path) : device_(device), type_(type), path_(path) {
  const auto data = platform::Platform::ReadFile(path_);
  auto compiled = Compile(data, path_, type_);
  std::vector<uint32_t> spirv(compiled.cbegin(), compiled.cend());

  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = spirv.size() * sizeof(uint32_t);
  create_info.pCode = spirv.data();

  if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  reflection_data_ = GetReflectionData(std::move(spirv));
}

std::vector<VkPipelineShaderStageCreateInfo> Material::GetShaderStages() const {
  VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vertex_.GetShaderModule();
  vert_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = fragment_.GetShaderModule();
  frag_shader_stage_info.pName = "main";

  return {vert_shader_stage_info, frag_shader_stage_info};
}

std::tuple<std::vector<VkVertexInputBindingDescription>, std::vector<VkVertexInputAttributeDescription>> Material::GetInputBindings()
    const {
  return std::make_tuple(GetBindingDescription(vertex_), GetAttributeDescriptions(vertex_));
}

VkDescriptorSetLayout Material::GetDescriptorSetLayout(VkDevice device) {
  if (descriptor_set_layout_ != VK_NULL_HANDLE) {
    return descriptor_set_layout_;
  }

  auto ubo_layout_bindings = GetLayoutBindings(vertex_);
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = ubo_layout_bindings.size();
  layout_info.pBindings = ubo_layout_bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  return descriptor_set_layout_;
}

}  // namespace vre::rendering