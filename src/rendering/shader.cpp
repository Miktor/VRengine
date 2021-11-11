#include "shader.hpp"

#include <stdexcept>

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

  auto result = compiler.CompileGlslToSpv(data.c_str(), ToShadercType(type), path.data(), options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    throw std::runtime_error(result.GetErrorMessage());
  }

  return result;
}

void GetReflectionData(const std::vector<uint32_t> &data) {
  spirv_cross::Compiler compiler(data);

  auto resources = compiler.get_shader_resources();

  for (auto &resource : resources.uniform_buffers) {
    unsigned set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
    unsigned binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
    SPDLOG_INFO("Image {} at set = {}, binding = {}", resource.name.c_str(), set, binding);
  }
}

}  // namespace

Shader::Shader(VkDevice device, Type type, const std::string &path) : device_(device), type_(type), path_(path) {}

VkShaderModule Shader::Init() {
  const auto data = platform::Platform::ReadFile(path_);
  auto compiled = Compile(data, path_, type_);

  std::vector<uint32_t> spirv(compiled.cbegin(), compiled.cend());
  GetReflectionData(spirv);

  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = spirv.size() * sizeof(uint32_t);
  create_info.pCode = spirv.data();

  VkShaderModule shader_module;
  if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shader_module;
}

}  // namespace vre::rendering