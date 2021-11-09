#include "application.hpp"

#include <fstream>

#include <vulkan/vulkan_core.h>

namespace vre {

namespace {

constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;

}  // namespace

static void KeyCallback(GLFWindow *window, int key, int scancode, int action, int mods) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->ProcessInput(window, key, scancode, action, mods);
}

bool Application::ProcessInput(GLFWindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return false;
  }

  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_W:
        break;
      case GLFW_KEY_A:
        break;
      case GLFW_KEY_S:
        break;
      case GLFW_KEY_D:
        break;
      default:
        break;
    }
  }

  return true;
}

void Application::Run() {
  InitWindow();
  render_core_.InitVulkan(window_);

  main_scene_.LoadFromFile();
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

    auto [command_buffer, pipeline_layout, descriptor_set] = render_core_.BeginDraw();

    main_scene_.Render(command_buffer, pipeline_layout, descriptor_set);

    render_core_.Present(command_buffer);
  }

  render_core_.WaitDeviceIdle();
}

void Application::Cleanup() {
  render_core_.Cleanup();

  glfwDestroyWindow(window_);
  glfwTerminate();
}

}  // namespace vre