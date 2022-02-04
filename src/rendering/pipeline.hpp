#pragma once

#include "rendering/render_pass.hpp"
#include "rendering/shader.hpp"

namespace vre::rendering {
class Pipeline {
 public:
  Pipeline(VkDevice device) : device_(device) {}

  VkPipeline GetPipeline() { return graphics_pipeline_; }
  VkPipelineLayout GetLayout() { return pipeline_layout_; }

 private:
  VkDevice device_;

  VkPipelineLayout pipeline_layout_;
  VkPipeline graphics_pipeline_;

 public:
};

}  // namespace vre::rendering