#include "scene.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <memory>

#include "helpers.hpp"
#include "node.hpp"
#include "rendering/mesh.hpp"
#include "rendering/render_core.hpp"
#include "serialization/gltf_loader.hpp"

namespace vre::scene {

void Scene::LoadFromFile() {
  root_node_ = serialization::GLTFLoader::LoadFromFile("assets/scenes/basic.gltf");
}

void Scene::CreateCamera() {
  main_camera_node_ = &GetRootNode().CreateChildNode("main_camera");
  main_camera_ = reinterpret_cast<vre::scene::Camera *>(
      &main_camera_node_->Attach(std::make_unique<vre::scene::Camera>()));

  main_camera_node_->transform_.position = glm::vec3(0, -2, -10);
}

void Scene::InitializeVulkan(rendering::RenderCore &renderer) {
  for (auto &child : root_node_->childrens_) {
    if (child->mesh_) {
      child->mesh_->InitializeVulkan(renderer);
    }
  }

  if (root_node_->mesh_) {
    root_node_->mesh_->InitializeVulkan(renderer);
  }
}

void Scene::Update() {
}

void Scene::Render(rendering::RenderContext &context) {
  context.render_data.camera_view = main_camera_->GetView();
  context.render_data.camera_projection = main_camera_->GetProjection();

  for (auto &child : root_node_->childrens_) {
    if (child->mesh_) {
      child->mesh_->Render(context, child->GetTransform());
    }
  }

  if (root_node_->mesh_) {
    root_node_->mesh_->Render(context, root_node_->GetTransform());
  }
}

Node &Scene::GetRootNode() {
  VR_ASSERT(root_node_);
  return *root_node_;
}

Camera &Scene::GetMainCamera() {
  VR_ASSERT(main_camera_);
  return *main_camera_;
}

Node &Scene::GetMainCameraNode() {
  VR_ASSERT(main_camera_node_);
  return *main_camera_node_;
}

void Scene::Cleanup() {
  root_node_.reset();
}

}  // namespace vre::scene