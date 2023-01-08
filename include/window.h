#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "player.h"
#include "config.h"
#include "helpers.h"

namespace ImPlay {
class Window {
 public:
  Window(const char *title, int width, int height);
  ~Window();

  bool run(Helpers::OptionParser optionParser);

 private:
  void render();
  void requestRender();

  void initGLFW();
  void initImGui();
  void exitGLFW();
  void exitImGui();

  Config *config;
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