#include "shader.hpp"
#include <vulkan/vulkan_core.h>

#include <iostream>
#include <memory>
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

ResourceLayout GetResourceLayout(std::vector<uint32_t> &&spirv) {
  spirv_cross::Compiler compiler(std::move(spirv));

  auto resources = compiler.get_shader_resources();
  ResourceLayout result;

  for (auto &resource : resources.stage_inputs) {
    ResourceLayout::Input input{};
    input.name = resource.name;
    input.location = compiler.get_decoration(resource.id, spv::DecorationLocation);

    const auto &type = compiler.get_type(resource.base_type_id);
    input.width = type.width * type.vecsize * type.columns / 8;

    result.inputs.push_back(input);
  }

  for (auto &resource : resources.uniform_buffers) {
    const auto set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    const auto binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
    SPDLOG_INFO("UB {} at set = {}, binding = {}", resource.name.c_str(), set, binding);

    DescriptorSetLayout::UniformBuffer ub{};
    ub.name = resource.name;
    ub.binding = binding;

    result.descriptor_set_layouts[set].uniform_buffers.push_back(ub);
  }

  return result;
}

std::vector<VkVertexInputBindingDescription> GetBindingDescription(const Shader &vertex) {
  std::vector<VkVertexInputBindingDescription> result;

  for (const auto &input : vertex.GetResourceLayout().inputs) {
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

  for (const auto &input : vertex.GetResourceLayout().inputs) {
    VkVertexInputAttributeDescription attribute_description{};

    attribute_description.binding = 0;
    attribute_description.location = input.location;
    attribute_description.format = VK_FORMAT_R32G32_SFLOAT;
    attribute_description.offset = input.offset;

    result.push_back(attribute_description);
  }

  return result;
}

std::unordered_map<uint8_t, std::vector<VkDescriptorSetLayoutBinding>> GetLayoutBindings(const CombinedResourceLayout &resource_layout) {
  std::unordered_map<uint8_t, std::vector<VkDescriptorSetLayoutBinding>> result;

  for (const auto &[set, descriptor_set_layoout] : resource_layout.descriptor_set_layouts) {
    for (const auto &ubo : descriptor_set_layoout.uniform_buffers) {
      VkDescriptorSetLayoutBinding layout_binding{};

      layout_binding.binding = ubo.binding;
      layout_binding.descriptorCount = 1;
      layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      layout_binding.pImmutableSamplers = nullptr;
      layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

      result[set].push_back(layout_binding);
    }
  }

  return result;
}

std::vector<VkDescriptorSetLayout> GetDescriptorSetLayouts(
    VkDevice device, const std::unordered_map<uint8_t, std::vector<VkDescriptorSetLayoutBinding>> &bindings) {
  std::vector<VkDescriptorSetLayout> result;
  result.reserve(bindings.size());

  for (const auto &[_, binding] : bindings) {
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = binding.size();
    layout_info.pBindings = binding.data();

    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    CHECK_VK_SUCCESS(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout));
    result.push_back(descriptor_set_layout);
  }

  return result;
}

void UpdateFromShader(CombinedResourceLayout &result, const Shader &shader) {
  for (const auto &[set, layout] : shader.GetResourceLayout().descriptor_set_layouts) {
    auto &result_layout = result.descriptor_set_layouts[set];
    result_layout.uniform_buffers.insert(result_layout.uniform_buffers.end(), layout.uniform_buffers.begin(), layout.uniform_buffers.end());
  }
}

CombinedResourceLayout BuildCombinedResourceLayout(const Shader &fragment, const Shader &vertex) {
  CombinedResourceLayout result;

  UpdateFromShader(result, fragment);
  UpdateFromShader(result, vertex);

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

  CHECK_VK_SUCCESS(vkCreateShaderModule(device_, &create_info, nullptr, &shader_module_));

  resource_layout_ = ::vre::rendering::GetResourceLayout(std::move(spirv));
}

PipelineLayout::PipelineLayout(VkDevice device, const CombinedResourceLayout &resource_layout)
    : device_(device), resource_layout_(resource_layout) {
  descriptor_set_layouts_ = ::vre::rendering::GetDescriptorSetLayouts(device_, GetLayoutBindings(resource_layout_));

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = descriptor_set_layouts_.size();
  pipeline_layout_info.pSetLayouts = descriptor_set_layouts_.data();

  CHECK_VK_SUCCESS(vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_));
}

Material::Material(VkDevice device, Shader &&fragment, Shader &&vertex)
    : device_(device), fragment_(std::move(fragment)), vertex_(std::move(vertex)) {
  combined_resource_layout_ = BuildCombinedResourceLayout(fragment_, vertex_);
  pipeline_layout_ = std::make_shared<PipelineLayout>(device_, combined_resource_layout_);
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

PipelineLayout &Material::GetPipelineLayout() const { return *pipeline_layout_; }

}  // namespace vre::rendering