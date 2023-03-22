// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>
#ifdef IMGUI_IMPL_OPENGL_ES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
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
  explicit Window(Config *config);
  ~Window();

  bool init(OptionParser &parser);
  void run();

 private:
  void wakeup();
  void render();
  void renderVideo();
  void updateCursor();
  void saveState();

  void loadFonts();
  void reloadFonts();
  void initGLFW(const char *title);
  void initImGui();
  void exitGLFW();
  void exitImGui();

  Config *config;
  GLFWwindow *window = nullptr;
  Mpv *mpv = nullptr;
  Player *player = nullptr;
  int width = 1280, height = 720;
  bool ownCursor = true;
  double lastInputAt = 0;
  GLuint fbo, tex;
  std::mutex glCtxLock;
#ifdef _WIN32
  bool borderless = false;
  WNDPROC wndProcOld = nullptr;
  static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

  struct GLCtxGuard {
   public:
    inline GLCtxGuard(GLFWwindow *ctx, std::mutex *lock) : lk(lock) {
      lk->lock();
      glfwMakeContextCurrent(ctx);
    }
    inline ~GLCtxGuard() {
      glfwMakeContextCurrent(nullptr);
      lk->unlock();
    }

   private:
    std::mutex *lk;
  };

  struct Waiter {
   public:
    void wait();
    void wait_until(std::chrono::steady_clock::time_point time);
    void notify();

   private:
    std::mutex lock;
    std::condition_variable cond;
    bool notified = false;
  };

  struct Waiter eventWaiter, videoWaiter;
};
}  // namespace ImPlay