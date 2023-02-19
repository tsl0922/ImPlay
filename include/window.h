// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "player.h"
#include "config.h"
#include "dispatch.h"

namespace ImPlay {
class Window {
 public:
  Window();
  ~Window();

  bool init(OptionParser& parser);
  void run();

 private:
  void eventLoop();
  void renderLoop();
  void requestRender();
  void updateCursor();
  void saveState();

  void loadFonts();
  void initGLFW(const char *title);
  void initImGui();
  void exitGLFW();
  void exitImGui();

  Config config;
  Dispatch dispatch;
  GLFWwindow *window = nullptr;
  Mpv *mpv = nullptr;
  Player *player = nullptr;
  int width = 1280;
  int height = 720;

  std::mutex renderMutex;
  std::condition_variable renderCond;
  std::atomic_bool shutdown = false;
  std::atomic_int waitTimeout = 50;
  bool ownCursor = false;
  bool wantRender = true;
  double lastInputAt = 0;
  double lastMousePressAt = 0;
};
}  // namespace ImPlay