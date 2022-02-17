#include <memory>
#include <vector>
#include "helpers.hpp"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>

#include "gltf_loader.hpp"
#include "rendering/mesh.hpp"
#include "scene/node.hpp"

namespace vre::serialization {

namespace {

void LoadMesh(const tinygltf::Model &model, const tinygltf::Mesh &mesh, scene::Node &node) {
  auto new_mesh = std::make_unique<rendering::Mesh>();

  for (const auto &primitive : mesh.primitives) {
    std::vector<glm::vec3> vertex_buffer;
    {
      const float *buffer_pos = nullptr;

      int pos_byte_stride;

      VR_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end());

      const tinygltf::Accessor &pos_accessor = model.accessors[primitive.attributes.find("POSITION")->second];
      const tinygltf::BufferView &pos_view = model.bufferViews[pos_accessor.bufferView];
      buffer_pos = reinterpret_cast<const float *>(
          &(model.buffers[pos_view.buffer].data[pos_accessor.byteOffset + pos_view.byteOffset]));
      pos_byte_stride = pos_accessor.ByteStride(pos_view) != 0
                            ? (pos_accessor.ByteStride(pos_view) / sizeof(float))
                            : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

      for (size_t v = 0; v < pos_accessor.count; v++) {
        vertex_buffer.push_back(glm::make_vec3(&buffer_pos[v * pos_byte_stride]));
      }
    }

    std::vector<uint32_t> index_buffer;
    if (primitive.indices > -1) {
      const tinygltf::Accessor &accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
      const tinygltf::BufferView &buffer_view = model.bufferViews[accessor.bufferView];
      const tinygltf::Buffer &buffer = model.buffers[buffer_view.buffer];

      const auto index_count = static_cast<uint32_t>(accessor.count);
      const void *data_ptr = &(buffer.data[accessor.byteOffset + buffer_view.byteOffset]);

      switch (accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
          const auto *buf = static_cast<const uint32_t *>(data_ptr);
          for (size_t index = 0; index < index_count; index++) {
            index_buffer.push_back(buf[index]);
          }
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
          const auto *buf = static_cast<const uint16_t *>(data_ptr);
          for (size_t index = 0; index < index_count; index++) {
            index_buffer.push_back(buf[index]);
          }
          break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
          const auto *buf = static_cast<const uint8_t *>(data_ptr);
          for (size_t index = 0; index < index_count; index++) {
            index_buffer.push_back(buf[index]);
          }
          break;
        }
        default:
          SPDLOG_ERROR("Index component type {} not supported!", accessor.componentType);
          return;
      }
    }

    new_mesh->AddPrimitive(std::move(vertex_buffer), std::move(index_buffer));
  }

  node.mesh_ = std::move(new_mesh);
}

void LoadNode(scene::Node &parent, const tinygltf::Node &node, const tinygltf::Model &model) {
  auto &new_node = parent.CreateChildNode(node.name);

  // Generate local node matrix
  auto translation = glm::vec3(0.0F);
  if (node.translation.size() == 3) {
    translation = glm::make_vec3(node.translation.data());
    new_node.transform_.position = translation;
  }

  if (node.rotation.size() == 4) {
    glm::quat q = glm::make_quat(node.rotation.data());
    new_node.transform_.rotation = q;
  }

  auto scale = glm::vec3(1.0F);
  if (node.scale.size() == 3) {
    scale = glm::make_vec3(node.scale.data());
    new_node.transform_.scale = scale;
  }

  for (const auto &child : node.children) {
    LoadNode(new_node, model.nodes[child], model);
  }

  // Node contains mesh data
  if (node.mesh > -1) {
    const tinygltf::Mesh &mesh = model.meshes[node.mesh];
    LoadMesh(model, mesh, new_node);
  }
}

}  // namespace

std::unique_ptr<scene::Node> GLTFLoader::LoadFromFile(const std::string &filename) {
  tinygltf::Model gltf_model;
  tinygltf::TinyGLTF gltf_context;
  std::string error;
  std::string warning;

  bool binary = false;
  size_t extpos = filename.rfind('.', filename.length());
  if (extpos != std::string::npos) {
    binary = (filename.substr(extpos + 1, filename.length() - extpos) == "glb");
  }

  const bool file_loaded = binary ? gltf_context.LoadBinaryFromFile(&gltf_model, &error, &warning, filename)
                                  : gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning, filename);
  auto root_node = std::make_unique<scene::Node>();
  if (file_loaded) {
    const tinygltf::Scene &scene =
        gltf_model.scenes[gltf_model.defaultScene > -1 ? gltf_model.defaultScene : 0];
    for (const auto &node : scene.nodes) {
      LoadNode(*root_node, gltf_model.nodes[node], gltf_model);
    }
  } else {
    SPDLOG_ERROR("Could not load gltf file: {}", error);
  }

  return root_node;
}

}  // namespace vre::serialization