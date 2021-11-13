#pragma once

#include "common.hpp"

namespace vre::rendering {
class RenderPass {
 public:
  RenderPass(VkDevice device) : device_(device) {}

 private:
  VkDevice device_;

 private:
};

}  // namespace vre::rendering