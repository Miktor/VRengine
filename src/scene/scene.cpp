#include "scene.hpp"

#include "node.hpp"
#include "rendering/mesh.hpp"
#include "rendering/render_core.hpp"
#include "serialization/gltf_loader.hpp"

namespace vre::scene {

void Scene::LoadFromFile() { root_nodes_ = serialization::GLTFLoader::LoadFromFile("assets/scenes/basic.gltf"); }

void Scene::Initialize() {}

void Scene::InitializeVulkan(rendering::RenderCore &renderer) {
  for (auto &node : root_nodes_) {
    for (auto &child : node->childrens_) {
      if (child->mesh_) {
        child->mesh_->InitializeVulkan(renderer);
      }
    }

    if (node->mesh_) {
      node->mesh_->InitializeVulkan(renderer);
    }
  }
}

void Scene::Update() {}

void Scene::Render(VkCommandBuffer command_buffers, VkPipelineLayout pipeline_layout, VkDescriptorSet descriptor_set) {
  for (auto &node : root_nodes_) {
    for (auto &child : node->childrens_) {
      if (child->mesh_) {
        child->mesh_->Render(command_buffers, pipeline_layout, descriptor_set);
      }
    }

    if (node->mesh_) {
      node->mesh_->Render(command_buffers, pipeline_layout, descriptor_set);
    }
  }
}

}  // namespace vre::scene