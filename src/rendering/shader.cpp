#include "shader.hpp"

#include <shaderc/shaderc.hpp>
#include <stdexcept>

#include "common.hpp"
#include "platform/platform.hpp"
#include "shaderc/shaderc.h"

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

shaderc::SpvCompilationResult Compile(const std::string &path, Shader::Type type) {
  const auto data = platform::Platform::ReadFile(path);
  shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  //options.SetOptimizationLevel(shaderc_optimization_level_size);

  auto result = compiler.CompileGlslToSpv(data.c_str(), ToShadercType(type), path.data(), options);

  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    throw std::runtime_error(result.GetErrorMessage());
  }

  return result;
}

}  // namespace

Shader::Shader(VkDevice device, Type type, const std::string &path) : device_(device), type_(type), path_(path) {}

VkShaderModule Shader::Init() {
  auto compiled = Compile(path_, type_);

  std::vector<uint32_t> spirv(compiled.cbegin(), compiled.cend());
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