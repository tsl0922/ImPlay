#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>
#include "player.h"

namespace ImPlay {
class Window {
 public:
  Window(const char *title, int width, int height);
  ~Window();

  bool run(int argc, char *argv[]);

 private:
  void render();
  void requestRender();

  void initGLFW();
  void initImGui();
  void exitGLFW();
  void exitImGui();

  GLFWwindow *window = nullptr;
  Player *player = nullptr;
  const char *title;
  int width, height;

  std::mutex renderMutex;
  std::condition_variable renderCv;
  bool wantRender = true;
};
}  // namespace ImPlay