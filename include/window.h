#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>
#include <atomic>
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

  const int defaultTimeout = 50;  // ms

  std::mutex renderMutex;
  std::condition_variable renderCond;
  std::atomic_int waitTimeout = defaultTimeout;
  double lastRenderAt = 0;
  double lastMousePressAt = 0;
  bool wantRender = true;
};
}  // namespace ImPlay