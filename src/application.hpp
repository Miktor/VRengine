#pragma once

#include "common.hpp"

#include "rendering/buffers.hpp"
#include "rendering/render_core.hpp"
#include "scene/scene.hpp"

namespace vre {

struct ControlsState {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool up = false;
  bool down = false;
};

struct MousePos {
  double x = .0;
  double y = .0;
};

class Application {
 public:
  void Run();

  void ProcessInput(GLFWwindow *window, int key, int scancode, int action, int mods);
  void ProcessMouseKey(GLFWwindow *window, int button, int action, int mods);
  void ProcessMouseMove(GLFWwindow *window, double xpos, double ypos);

 protected:
  GLFWwindow *window_ = nullptr;

  rendering::RenderCore render_core_;

  scene::Scene main_scene_;

  ControlsState controlls_state_;

  MousePos last_mouse_pos_;
  MousePos mouse_move_;
  bool move_camera_ = false;

  virtual void Cleanup();

 private:
  void InitWindow();
  void MainLoop();
};

}  // namespace vre