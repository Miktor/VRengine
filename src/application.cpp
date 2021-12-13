#include "application.hpp"

#include <fstream>

#include <vulkan/vulkan_core.h>

#include "scene/node.hpp"

namespace vre {

namespace {

constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;

}  // namespace

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->ProcessInput(window, key, scancode, action, mods);
}

bool Application::ProcessInput(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return false;
  }

  if (action == GLFW_PRESS) {
    constexpr float kStep = 0.1F;
    glm::vec3 transform(0.0F);
    switch (key) {
      case GLFW_KEY_W:
        transform += glm::vec3(0, 1.F, 0);
        break;
      case GLFW_KEY_A:
        transform += glm::vec3(-1.F, 0, 0);
        break;
      case GLFW_KEY_S:
        transform += glm::vec3(0, -1.F, 0);
        break;
      case GLFW_KEY_D:
        transform += glm::vec3(1.F, 0, 0);
        break;
    }

    main_scene_.main_camera_node_->transform_.position += transform * kStep;
  }

  return true;
}

void Application::Run() {
  InitWindow();
  render_core_.InitVulkan(window_);

  main_scene_.LoadFromFile();
  main_scene_.CreateCamera();
  main_scene_.InitializeVulkan(render_core_);

  MainLoop();
  Cleanup();
}

void Application::InitWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);

  glfwSetWindowUserPointer(window_, this);
  glfwSetKeyCallback(window_, KeyCallback);
}

void Application::MainLoop() {
  while (glfwWindowShouldClose(window_) == 0) {
    glfwPollEvents();

    auto context = render_core_.BeginDraw();

    main_scene_.Render(context);

    render_core_.Present(context);
  }

  render_core_.WaitDeviceIdle();
}

void Application::Cleanup() {
  render_core_.Cleanup();

  glfwDestroyWindow(window_);
  glfwTerminate();
}

}  // namespace vre