// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <fonts/fontawesome.h>
#include <fonts/unifont.h>
#define GL_SILENCE_DEPRECATION
#ifdef IMGUI_IMPL_OPENGL_ES2
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#endif
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include "window.h"

namespace ImPlay {
Window::Window() {
  const char* title = "ImPlay";

  config.load();
  initGLFW(title);
  initImGui();

#ifdef _WIN32
  HWND hwnd = glfwGetWin32Window(window);
  int64_t wid = config.Data.Mpv.UseWid ? static_cast<uint32_t>((intptr_t)hwnd) : 0;
  mpv = new Mpv(wid);
#else
  mpv = new Mpv();
#endif
  player = new Player(&config, &dispatch, window, mpv, title);
}

Window::~Window() {
  delete player;
  glfwMakeContextCurrent(window);
  delete mpv;
  glfwMakeContextCurrent(nullptr);

  exitImGui();
  exitGLFW();
}

bool Window::run(OptionParser& parser) {
  mpv->wakeupCb() = [](Mpv* ctx) { glfwPostEmptyEvent(); };
  mpv->updateCb() = [this](Mpv* ctx) {
    if (ctx->wantRender()) requestRender();
  };
  dispatch.wakeup() = []() { glfwPostEmptyEvent(); };

  glfwMakeContextCurrent(window);
  if (!player->init(parser)) return false;
  glfwMakeContextCurrent(nullptr);
#if defined(__APPLE__) && defined(GLFW_PATCHED)
  const char** openedFileNames = glfwGetOpenedFilenames();
  if (openedFileNames != nullptr) {
    int count = 0;
    while (openedFileNames[count] != nullptr) count++;
    player->onDropEvent(count, openedFileNames);
  }
#endif

  std::atomic_bool shutdown = false;

  std::thread renderThread([&]() {
    while (!glfwWindowShouldClose(window)) {
      {
        std::unique_lock<std::mutex> lk(renderMutex);
        auto timeout = std::chrono::milliseconds(waitTimeout);
        renderCond.wait_for(lk, timeout, [&]() { return wantRender; });
        wantRender = false;
      }

      if (config.FontReload) {
        loadFonts();
        config.FontReload = false;
        glfwMakeContextCurrent(window);
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
        glfwMakeContextCurrent(nullptr);
      }

      player->render(width, height);
    }
    shutdown = true;
  });

  int64_t cursorAutoHide = mpv->property<int64_t, MPV_FORMAT_INT64>("cursor-autohide");

  while (!shutdown) {
    glfwWaitEvents();
    player->renderGui() = true;
    mpv->waitEvent();
    dispatch.process();

    bool hasInputEvents = !ImGui::GetCurrentContext()->InputEventsQueue.empty();
    if (ownCursor && cursorAutoHide > 0 && mpv->playing() && !ImGui::GetIO().WantCaptureMouse) {
      int mode = (glfwGetTime() - lastInputAt) * 1000 > cursorAutoHide ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
      glfwSetInputMode(window, GLFW_CURSOR, mode);
    }
    if ((!glfwGetWindowAttrib(window, GLFW_VISIBLE) || glfwGetWindowAttrib(window, GLFW_ICONIFIED)) &&
        !hasInputEvents && ImGui::GetCurrentContext()->Viewports.Size == 1) {
      waitTimeout = INT_MAX;
      continue;
    }
    double delta = glfwGetTime() - lastRenderAt;
    waitTimeout = hasInputEvents ? std::max(defaultTimeout, (int)delta * 1000) : 1000;
    if (hasInputEvents && (delta > (double)defaultTimeout / 1000)) requestRender();
  }

  renderThread.join();

  if (config.Data.Window.Save) {
    glfwGetWindowPos(window, &config.Data.Window.X, &config.Data.Window.Y);
    glfwGetWindowSize(window, &config.Data.Window.W, &config.Data.Window.H);
    config.save();
  }

  return true;
}

void Window::requestRender() {
  std::unique_lock<std::mutex> lk(renderMutex);
  wantRender = true;
  lk.unlock();
  renderCond.notify_one();
  lastRenderAt = glfwGetTime();
}

void Window::initGLFW(const char* title) {
  glfwSetErrorCallback([](int error, const char* desc) { print("GLFW Error [{}]: {}\n", error, desc); });
  if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW!");

#ifdef IMGUI_IMPL_OPENGL_ES2
  eglGetDisplay(EGL_DEFAULT_DISPLAY);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  width = std::max((int)(mode->width * 0.4), 600);
  height = std::max((int)(mode->height * 0.4), 400);
  int posX = (mode->width - width) / 2;
  int posY = (mode->height - height) / 2;
  if (config.Data.Window.Save) {
    if (config.Data.Window.W > 0) width = config.Data.Window.W;
    if (config.Data.Window.H > 0) height = config.Data.Window.H;
    if (config.Data.Window.X >= 0) posX = config.Data.Window.X;
    if (config.Data.Window.Y >= 0) posY = config.Data.Window.Y;
  }

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr) throw std::runtime_error("Failed to create window!");
  glfwSetWindowSizeLimits(window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwSetWindowPos(window, posX, posY);

  glfwSetWindowUserPointer(window, this);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glfwSwapBuffers(window);
  glfwMakeContextCurrent(nullptr);

  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int w, int h) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->width = w;
    win->height = h;
    glViewport(0, 0, w, h);
  });
  glfwSetWindowContentScaleCallback(window, [](GLFWwindow* window, float x, float y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->config.Data.Interface.Scale = std::max(x, y);
    win->config.FontReload = true;
  });
  glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->shutdown();
  });
  glfwSetWindowRefreshCallback(window, [](GLFWwindow* window) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win->player->hasFile()) win->player->renderGui() = false;
    win->requestRender();
    win->dispatch.process();
  });
  glfwSetWindowPosCallback(window, [](GLFWwindow* window, int x, int y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win->player->hasFile()) win->player->renderGui() = false;
    win->requestRender();
    win->dispatch.process();
  });
  glfwSetCursorEnterCallback(window, [](GLFWwindow* window, int entered) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->ownCursor = entered;
  });
  glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (ImGui::GetIO().WantCaptureMouse) return;
    win->player->onCursorEvent(x, y);
#ifdef GLFW_PATCHED
    if (win->mpv->allowDrag() && win->height - y > 150 && glfwGetTime() - win->lastMousePressAt > 0.01) {
      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) glfwDragWindow(window);
    }
#endif
  });
  glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) win->lastMousePressAt = glfwGetTime();
    if (!ImGui::GetIO().WantCaptureMouse) win->player->onMouseEvent(button, action, mods);
  });
  glfwSetScrollCallback(window, [](GLFWwindow* window, double x, double y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (!ImGui::GetIO().WantCaptureMouse) win->player->onScrollEvent(x, y);
  });
  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (!ImGui::GetIO().WantCaptureKeyboard) win->player->onKeyEvent(key, scancode, action, mods);
  });
  glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char* paths[]) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!ImGui::GetIO().WantCaptureMouse) win->player->onDropEvent(count, paths);
  });
}

void Window::loadFonts() {
  float scale = config.Data.Interface.Scale;
  if (scale == 0) {
#ifdef __APPLE__
    scale = 1.0f;
#else
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    scale = std::max(xscale, yscale);
#endif
  }
  float fontSize = std::floor(config.Data.Font.Size * scale);
  float iconSize = fontSize - 2;
  ImGuiIO& io = ImGui::GetIO();
  io.DisplayFramebufferScale = ImVec2(scale, scale);

  ImGui::SetTheme(config.Data.Interface.Theme.c_str());
  ImGui::GetStyle().ScaleAllSizes(scale);

  io.Fonts->Clear();

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  io.Fonts->AddFontDefault(&cfg);
  cfg.MergeMode = true;
  ImWchar fontAwesomeRange[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  const ImWchar* unifontRange = config.buildGlyphRanges();
  io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data, font_awesome_compressed_size, iconSize, &cfg,
                                           fontAwesomeRange);
  if (config.Data.Font.Path.empty())
    io.Fonts->AddFontFromMemoryCompressedTTF(unifont_compressed_data, unifont_compressed_size, 0, &cfg, unifontRange);
  else
    io.Fonts->AddFontFromFileTTF(config.Data.Font.Path.c_str(), 0, &cfg, unifontRange);

  io.Fonts->Build();
}

void Window::initImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  loadFonts();

  glfwMakeContextCurrent(window);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef IMGUI_IMPL_OPENGL_ES2
  print("Using OpenGL ES 2.0\n");
  ImGui_ImplOpenGL3_Init("#version 100");
#elif defined(__APPLE__)
  ImGui_ImplOpenGL3_Init("#version 150");
#else
  ImGui_ImplOpenGL3_Init("#version 130");
#endif
  glfwMakeContextCurrent(nullptr);
}

void Window::exitGLFW() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::exitImGui() {
  glfwMakeContextCurrent(window);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwMakeContextCurrent(nullptr);
}
}  // namespace ImPlay