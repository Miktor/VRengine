#include "command_buffer.hpp"
#include <vulkan/vulkan_core.h>
#include <vector>

#include "helpers.hpp"
#include "rendering/render_core.hpp"
#include "rendering/shader.hpp"
#include "rendering/uniform_buffer_allocator.hpp"

namespace vre::rendering {

namespace {

VkRect2D GetScissors(const BeginRenderInfo &info) {
  auto scissors = info.render_pass_info.render_area;

  uint32_t fb_width = info.framebuffer->GetWidth();
  uint32_t fb_height = info.framebuffer->GetHeight();

  scissors.offset.x = std::min(fb_width, uint32_t(scissors.offset.x));
  scissors.offset.y = std::min(fb_height, uint32_t(scissors.offset.y));
  scissors.extent.width = std::min(fb_width - scissors.offset.x, scissors.extent.width);
  scissors.extent.height = std::min(fb_height - scissors.offset.y, scissors.extent.height);

  return scissors;
}

}  // namespace

CommandBuffer::~CommandBuffer() {
  if (ubo_allocated_data_) {
    auto &allocator = core_->GetUniformBufferPoolAllocator();
    allocator.Deallocate(ubo_allocated_data_);
  }
}

void CommandBuffer::Start() {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
}

void CommandBuffer::BeginRenderPass(const BeginRenderInfo &info) {
  std::vector<VkClearValue> clear_values;
  clear_values.resize(info.render_pass_info.color_attachments.size());
  uint32_t num_clear_values = 0;

  for (unsigned i = 0; i < info.render_pass_info.color_attachments.size(); i++) {
    const auto &clear_color = info.render_pass_info.clear_color[i];

    if ((info.render_pass_info.clear_attachments & (1U << i)) != 0U) {
      clear_values[i].color = clear_color;
      num_clear_values = i + 1;
    }
  }
  clear_values.resize(num_clear_values);

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = info.render_pass->GetRenderPass();
  render_pass_info.framebuffer = info.framebuffer->GetFramebuffer();
  render_pass_info.renderArea = GetScissors(info);
  render_pass_info.clearValueCount = clear_values.size();
  render_pass_info.pClearValues = clear_values.data();

  vkCmdBeginRenderPass(command_buffer_, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  state_.render_pass = info.render_pass;
}

void CommandBuffer::SetViewport(const VkViewport &viewport) {
  vkCmdSetViewport(command_buffer_, 0, 1, &viewport);
}

void CommandBuffer::SetScissors(const VkRect2D &scissor) { vkCmdSetScissor(command_buffer_, 0, 1, &scissor); }

void CommandBuffer::SetDescriptorSet(uint8_t set, VkDescriptorSet descriptor_set) {
  state_.descriptor_sets[set] = descriptor_set;
}

void CommandBuffer::BindVertexBuffers(uint32_t binding, const Buffer &buffer, VkDeviceSize offset,
                                      VkDeviceSize stride, VkVertexInputRate step_rate) {
  VR_ASSERT(state_.material);  // TODO: check correct buffer

  const auto vk_buffer = buffer.GetBuffer();
  vkCmdBindVertexBuffers(command_buffer_, binding, 1, &vk_buffer, &offset);
}

void CommandBuffer::BindIndexBuffer(const Buffer &buffer, VkDeviceSize offset, VkIndexType index_type) {
  VR_ASSERT(state_.material);

  vkCmdBindIndexBuffer(command_buffer_, buffer.GetBuffer(), offset, index_type);
}

void CommandBuffer::BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer &buffer,
                                      const VkDeviceSize offset, const VkDeviceSize size) {
  VR_ASSERT(state_.material);

  state_.resource_bindings[set].push_back({});
  auto &resource_binding = state_.resource_bindings[set].back();
  resource_binding.buffer = buffer.GetBuffer();
  resource_binding.offset = offset;
  resource_binding.size = size;
}

void CommandBuffer::AllocateUniformBuffer(uint32_t set, uint32_t binding, const VkDeviceSize size,
                                          const void *data) {
  VR_ASSERT(data);

  if (ubo_allocated_data_ == nullptr) {
    auto &allocator = core_->GetUniformBufferPoolAllocator();
    ubo_allocated_data_ = allocator.Allocate(size);
  }

  auto allocation = ubo_allocated_data_->Allocate(size);
  BindUniformBuffer(set, binding, ubo_allocated_data_->GetBuffer(), allocation.offset, allocation.size);

  memcpy(ubo_allocated_data_->GetBuffer().GetMappedData(), data, size);
}

void CommandBuffer::BindMaterial(Material &material) { state_.material = &material; }

void CommandBuffer::DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index,
                                int32_t vertex_offset, uint32_t first_instance) {
  BindDescriptorSet(0);
  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, BuildGraphicsPipeline());
  vkCmdDrawIndexed(command_buffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::BindDescriptorSet(uint32_t set) {
  VR_ASSERT(state_.material);

  auto &pipeline_layout = state_.material->GetPipelineLayout();
  if (const auto &descriptor_sets = state_.descriptor_sets;
      descriptor_sets.find(set) != descriptor_sets.end()) {
    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout.GetPipelineLayout(), set, 1, &descriptor_sets.find(set)->second,
                            0, nullptr);
    return;
  }

  std::vector<uint32_t> dynamic_offsets;
  for (const auto &resource_binding : state_.resource_bindings[set]) {
    dynamic_offsets.push_back(resource_binding.offset);
  }

  auto &allocator = pipeline_layout.GetDescriptorSetAllocator(set);
  auto descriptor_set = allocator.GetSet();

  auto update_template = pipeline_layout.GetUpdateTemplate(set);
  vkUpdateDescriptorSetWithTemplate(core_->GetDevice(), descriptor_set, update_template,
                                    state_.resource_bindings[set].data());

  vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout.GetPipelineLayout(), set, 1, &descriptor_set,
                          dynamic_offsets.size(), dynamic_offsets.data());
}

VkPipeline CommandBuffer::BuildGraphicsPipeline() {
  VR_ASSERT(state_.material);

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  VkDynamicState states[] = {
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_VIEWPORT,
  };
  dynamic_state.pDynamicStates = states;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = state_.is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0F;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0F;
  color_blending.blendConstants[1] = 0.0F;
  color_blending.blendConstants[2] = 0.0F;
  color_blending.blendConstants[3] = 0.0F;

  auto [binding_descriptions, attribute_descriptions] = state_.material->GetInputBindings();

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = binding_descriptions.size();
  vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
  vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

  auto shader_stages = state_.material->GetShaderStages();
  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = shader_stages.size();
  pipeline_info.pStages = shader_stages.data();
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.layout = state_.material->GetPipelineLayout().GetPipelineLayout();
  pipeline_info.renderPass = state_.render_pass->GetRenderPass();
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  VkPipeline graphics_pipeline;
  CHECK_VK_SUCCESS(vkCreateGraphicsPipelines(core_->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                             &graphics_pipeline));

  return graphics_pipeline;
}

}  // namespace vre::rendering