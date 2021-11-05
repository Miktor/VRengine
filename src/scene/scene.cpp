#include "scene.hpp"

#include "node.hpp"
#include "rendering/mesh.hpp"
#include "serialization/gltf_loader.hpp"

namespace vre::scene {

void Scene::LoadFromFile() { root_nodes_ = serialization::GLTFLoader::LoadFromFile("assets/scenes/basic.gltf"); }

void Scene::Initialize() {}

void Scene::InitializeVulkan(Application &app) {
  for (auto &node : root_nodes_) {
    for (auto &child : node->childrens_) {
      if (child->mesh_) {
        child->mesh_->InitializeVulkan(app);
      }
    }

    if (node->mesh_) {
      node->mesh_->InitializeVulkan(app);
    }
  }
}

void Scene::Update() {}

void Scene::Render(VkCommandBuffer command_buffers) {
  for (auto &node : root_nodes_) {
    for (auto &child : node->childrens_) {
      if (child->mesh_) {
        child->mesh_->Render(command_buffers);
      }
    }

    if (node->mesh_) {
      node->mesh_->Render(command_buffers);
    }
  }
}

}  // namespace vre::scene