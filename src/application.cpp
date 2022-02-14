#include "application.hpp"

#include <fstream>

#include <vulkan/vulkan_core.h>

#include "GLFW/glfw3.h"
#include "scene/node.hpp"

namespace vre {

namespace {

constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;

void UpdateControlsState(ControlsState &state, int key, bool new_value) {
  switch (key) {
    case GLFW_KEY_W:
      state.forward = new_value;
      break;
    case GLFW_KEY_S:
      state.backward = new_value;
      break;
    case GLFW_KEY_A:
      state.left = new_value;
      break;
    case GLFW_KEY_D:
      state.right = new_value;
      break;
    case GLFW_KEY_LEFT_SHIFT:
      state.up = new_value;
      break;
    case GLFW_KEY_LEFT_CONTROL:
      state.down = new_value;
      break;
  }
}

}  // namespace

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->ProcessInput(window, key, scancode, action, mods);
}

static void MouseMoveCallback(GLFWwindow *window, double xpos, double ypos) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->ProcessMouseMove(window, xpos, ypos);
}

static void MouseKeyCallback(GLFWwindow *window, int button, int action, int mods) {
  auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  app->ProcessMouseKey(window, button, action, mods);
}

void Application::ProcessInput(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }

  if (action == GLFW_PRESS) {
    UpdateControlsState(controlls_state_, key, true);
  }
  if (action == GLFW_RELEASE) {
    UpdateControlsState(controlls_state_, key, false);
  }
}

void Application::ProcessMouseKey(GLFWwindow *window, int button, int action, int mods) {
  switch (button) {
    case GLFW_MOUSE_BUTTON_1:
      if (action == GLFW_PRESS) {
        move_camera_ = true;
      }
      if (action == GLFW_RELEASE) {
        move_camera_ = false;
      }
  }
}

void Application::ProcessMouseMove(GLFWwindow *window, double xpos, double ypos) {
  MousePos current_pos{xpos, ypos};
  mouse_move_.x = last_mouse_pos_.x - current_pos.x;
  mouse_move_.y = last_mouse_pos_.y - current_pos.y;
  last_mouse_pos_ = current_pos;
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
  glfwSetMouseButtonCallback(window_, MouseKeyCallback);
  glfwSetCursorPosCallback(window_, MouseMoveCallback);
}

void Application::MainLoop() {
  while (glfwWindowShouldClose(window_) == 0) {
    glfwPollEvents();

    constexpr float kStep = 0.1F;
    glm::vec3 transform(0.0F);
    const auto &camera = main_scene_.main_camera_;
    if (controlls_state_.forward) {
      transform += camera->GetRelativeForward();
    }
    if (controlls_state_.backward) {
      transform -= camera->GetRelativeForward();
    }
    if (controlls_state_.left) {
      transform += camera->GetRelativeLeft();
    }
    if (controlls_state_.right) {
      transform -= camera->GetRelativeLeft();
    }
    if (controlls_state_.up) {
      transform += camera->GetUp();
    }
    if (controlls_state_.down) {
      transform -= camera->GetUp();
    }

    auto &camera_transform = main_scene_.main_camera_node_->transform_;
    camera_transform.position += transform * kStep;

    if (move_camera_) {
      constexpr float kMouseSensitivity = 0.1f;
      if (mouse_move_.x != 0.0) {
        camera->AddYaw(float(mouse_move_.x) * kMouseSensitivity);
      }

      if (mouse_move_.y != 0.0) {
        camera->AddPitch(float(mouse_move_.y) * kMouseSensitivity);
      }
    }
    mouse_move_ = {0, 0};

    main_scene_.GetRootNode()->transform_.rotation *= glm::angleAxis(glm::radians(1.f), camera->GetUp());

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