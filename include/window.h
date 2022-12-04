#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include "player.h"

namespace ImPlay {
class Window {
 public:
  Window(const char *title, int width, int height);
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
  const char *title;
  int width, height;
};
}  // namespace ImPlay