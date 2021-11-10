#pragma once

#include <string>

#include <vulkan/vulkan_core.h>

namespace vre::rendering {

class Shader {
 public:
  enum Type {
    kVertex,
    kFragment,
  };

  Shader(VkDevice device, Type type, const std::string &path);

  VkShaderModule Init();

 private:
  VkDevice device_;

  const std::string path_;
  const Type type_;
};

}  // namespace vre::rendering