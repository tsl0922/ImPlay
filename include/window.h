#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include "player.h"

#define TITLE "ImPlay"
#define WIDTH 1280
#define HEIGHT 720

namespace ImPlay {
class Window {
 public:
  Window();
  ~Window();

  void loop();

 private:
  void render();
  void redraw();
  void show();

  void initGLFW();
  void initImGui();
  void exitGLFW();
  void exitImGui();

  GLFWwindow *window;
  Player *player;
};
}  // namespace ImPlay