#include "scene.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include "node.hpp"
#include "rendering/mesh.hpp"
#include "rendering/render_core.hpp"
#include "serialization/gltf_loader.hpp"

namespace vre::scene {

void Scene::LoadFromFile() {
  root_nodes_ = serialization::GLTFLoader::LoadFromFile("assets/scenes/basic.gltf");
}

void Scene::CreateCamera() {
  main_camera_ = std::make_shared<vre::scene::Camera>();

  main_camera_node_ = GetRootNode()->CreateChildNode("main_camera");
  main_camera_node_->Attach(main_camera_);

  main_camera_node_->transform_.position = glm::vec3(0, -2, -10);
}

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

void Scene::Render(rendering::RenderContext &context) {
  context.render_data.camera_view = main_camera_->GetView();
  context.render_data.camera_projection = main_camera_->GetProjection();

  for (auto &node : root_nodes_) {
    for (auto &child : node->childrens_) {
      if (child->mesh_) {
        child->mesh_->Render(context, child->GetTransform());
      }
    }

    if (node->mesh_) {
      node->mesh_->Render(context, node->GetTransform());
    }
  }
}

std::shared_ptr<Node> Scene::GetRootNode() { return root_nodes_.front(); }

void Scene::Cleanup() {
  main_camera_node_.reset();
  main_camera_.reset();
  root_nodes_.clear();
}
}  // namespace vre::scene