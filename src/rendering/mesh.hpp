#pragma once

#include "common.hpp"

#include "rendering/buffers.hpp"
#include "rendering/render_core.hpp"
#include "rendering/shader.hpp"

namespace vre ::rendering {

class RenderCore;

struct Primitive {
  uint32_t index_start = 0;
  uint32_t vertex_start = 0;

  uint32_t index_count = 0;
  uint32_t vertex_count = 0;
};

class Mesh {
 public:
  void AddPrimitive(std::vector<glm::vec3> vert, const std::vector<uint32_t> &indicies);

  void InitializeVulkan(RenderCore &renderer);
  void Render(rendering::RenderContext &context, const glm::mat4 &transform);

 private:
  std::vector<Primitive> primitives_;

  std::vector<glm::vec3> pos_;
  std::vector<uint32_t> indicies_;

  std::shared_ptr<Buffer> index_buffer_;
  std::shared_ptr<Buffer> vertex_buffer_;
  std::shared_ptr<Material> material_;
};

}  // namespace vre::rendering