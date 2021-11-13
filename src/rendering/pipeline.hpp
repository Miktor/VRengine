#pragma once

#include "rendering/render_pass.hpp"
#include "rendering/shader.hpp"

namespace vre::rendering {
class Pipeline {
 public:
  Pipeline(VkDevice device) : device_(device) {}

  VkPipeline GetPipeline() { return graphics_pipeline_; }
  VkPipelineLayout GetLayoput() { return pipeline_layout_; }

 private:
  VkDevice device_;

  VkPipelineLayout pipeline_layout_;
  VkPipeline graphics_pipeline_;

 public:
  void CreateGraphicsPipeline(VkRenderPass render_pass, Material &material, const VkExtent2D &swap_chain_extent, VkPolygonMode mode);
};

}  // namespace vre::rendering