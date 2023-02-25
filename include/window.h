// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <vector>
#include <array>
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

namespace ImPlay {
class Window {
 public:
  Window();
  ~Window();

  bool init(OptionParser &parser);
  void run();

 private:
  void wakeup();
  void render();
  void updateCursor();
  void saveState();

  void loadFonts();
  void initGLFW(const char *title);
  void initImGui();
  void exitGLFW();
  void exitImGui();

  Config config;
  GLFWwindow *window = nullptr;
  Mpv *mpv = nullptr;
  Player *player = nullptr;
#ifdef _WIN32
  bool borderless = false;
  WNDPROC wndProcOld = nullptr;
  static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

  std::mutex mutex;
  std::condition_variable cv;
  bool notified = false;
  bool ownCursor = true;
  double lastInputAt = 0;
};
}  // namespace ImPlay