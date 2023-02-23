// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <fonts/fontawesome.h>
#include <fonts/source_code_pro.h>
#include <fonts/unifont.h>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <thread>
#ifdef IMGUI_IMPL_OPENGL_ES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#ifdef _WIN32
#include <windowsx.h>
#endif
#include "helpers.h"
#include "theme.h"
#include "window.h"

namespace ImPlay {
Window::Window() {
  const char* title = "ImPlay";

  config.load();

  initGLFW(title);
  initImGui();

#ifdef _WIN32
  HWND hwnd = glfwGetWin32Window(window);
  wndProcOld = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
  ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProc));
  ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

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

bool Window::init(OptionParser& parser) {
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

#ifdef _WIN32
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("border", [this](int flag) {
    borderless = !static_cast<bool>(flag);
    HWND hwnd = glfwGetWin32Window(window);
    if (borderless) {
      DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
      ::SetWindowLongA(hwnd, GWL_STYLE, style | WS_CAPTION | WS_MAXIMIZEBOX | WS_THICKFRAME);
    }
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
  });
#endif

  int border = mpv->property<int, MPV_FORMAT_FLAG>("border");
  glfwSetWindowAttrib(window, GLFW_DECORATED, border);
  return true;
}

void Window::run() {
  glfwShowWindow(window);

  std::thread renderThread([&]() {
    auto nextFrame = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(window)) {
      int frameTime = 1000 / config.Data.Interface.Fps;
      nextFrame += std::chrono::milliseconds(frameTime);
      {
        std::unique_lock<std::mutex> lk(renderMutex);
        renderCond.wait_until(lk, nextFrame, [&]() { return wantRender; });
        wantRender = false;
      }
      render();
    }
    shutdown = true;
  });

  while (!shutdown) {
    glfwWaitEvents();

    mpv->waitEvent();
    dispatch.process();
  }

  renderThread.join();

  saveState();
}

void Window::render() {
  glfwMakeContextCurrent(window);

  // Reload font if changed
  if (config.FontReload) {
    loadFonts();
    config.FontReload = false;
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  player->render();
  ImGui::Render();

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  mpv->render(width, height);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
  mpv->reportSwap();

  glfwMakeContextCurrent(nullptr);

  dispatch.async([this]() { updateCursor(); });

  // This will run on main thread, conflict with:
  //   - open file dialog on macOS (block main thread)
  //   - window dragging on windows (block main thread)
  // so, we only call it when viewports are enabled and no conflicts.
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    dispatch.sync([]() {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(nullptr);
    });
  }
}

void Window::requestRender() {
  std::unique_lock<std::mutex> lk(renderMutex);
  wantRender = true;
  lk.unlock();
  renderCond.notify_one();
}

void Window::saveState() {
  if (config.Data.Window.Save) {
    glfwGetWindowPos(window, &config.Data.Window.X, &config.Data.Window.Y);
    glfwGetWindowSize(window, &config.Data.Window.W, &config.Data.Window.H);
  }
  config.Data.Mpv.Volume = mpv->volume;
  config.save();
}

void Window::updateCursor() {
  if (!ownCursor || ImGui::GetIO().WantCaptureMouse) return;

  bool cursor = true;
  if (mpv->cursorAutohide == "no")
    cursor = true;
  else if (mpv->cursorAutohide == "always")
    cursor = false;
  else
    cursor = (glfwGetTime() - lastInputAt) * 1000 < std::stoi(mpv->cursorAutohide);
  glfwSetInputMode(window, GLFW_CURSOR, cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  ImGui::SetMouseCursor(cursor ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_None);
}

void Window::initGLFW(const char* title) {
  glfwSetErrorCallback(
      [](int error, const char* desc) { fmt::print(fg(fmt::color::red), "GLFW [{}]: {}\n", error, desc); });
  if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW!");

#if defined(IMGUI_IMPL_OPENGL_ES3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);

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
#ifdef IMGUI_IMPL_OPENGL_ES3
  if (!gladLoadGLES2(glfwGetProcAddress)) throw std::runtime_error("Failed to load GLES 2!");
#else
  if (!gladLoadGL(glfwGetProcAddress)) throw std::runtime_error("Failed to load GL!");
#endif
  glfwSwapInterval(1);
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
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
    win->requestRender();
    win->dispatch.process();
  });
  glfwSetWindowPosCallback(window, [](GLFWwindow* window, int x, int y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
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
#ifdef __APPLE__
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    x *= xscale;
    y *= yscale;
#endif
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
  float fontSize = config.Data.Font.Size;
  float iconSize = fontSize - 2;
  float scale = config.Data.Interface.Scale;
  if (scale == 0) {
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    scale = std::max(xscale, yscale);
  }
  if (scale <= 0) scale = 1.0f;
  fontSize = std::floor(fontSize * scale);
  iconSize = std::floor(iconSize * scale);

  ImGuiIO& io = ImGui::GetIO();
  ImGuiStyle style;
  std::string theme = config.Data.Interface.Theme;

  {
    ImGui::SetTheme(theme.c_str(), &style);
    style.TabRounding = 4;
    style.ScrollbarRounding = 9;
    style.WindowRounding = 7;
    style.GrabRounding = 3;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ChildRounding = 4;
    style.WindowShadowSize = 50.0f;
    style.ScrollbarSize = 10.0f;
    style.Colors[ImGuiCol_WindowShadow] = ImVec4(0, 0, 0, 1.0f);
  }

#ifdef __APPLE__
  io.FontGlobalScale = 1.0f / scale;
#else
  style.ScaleAllSizes(scale);
#endif
  ImGui::GetStyle() = style;

  io.Fonts->Clear();

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  ImWchar fa_range[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  const ImWchar* font_range = config.buildGlyphRanges();
  io.Fonts->AddFontFromMemoryCompressedTTF(source_code_pro_compressed_data, source_code_pro_compressed_size, 0, &cfg);
  cfg.MergeMode = true;
  io.Fonts->AddFontFromMemoryCompressedTTF(fa_compressed_data, fa_compressed_size, iconSize, &cfg, fa_range);
  if (config.Data.Font.Path.empty())
    io.Fonts->AddFontFromMemoryCompressedTTF(unifont_compressed_data, unifont_compressed_size, 0, &cfg, font_range);
  else
    io.Fonts->AddFontFromFileTTF(config.Data.Font.Path.c_str(), 0, &cfg, font_range);

  io.Fonts->Build();
}

void Window::initImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  if (config.Data.Interface.Docking) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  if (config.Data.Interface.Viewports) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  loadFonts();

  glfwMakeContextCurrent(window);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef IMGUI_IMPL_OPENGL_ES3
  ImGui_ImplOpenGL3_Init("#version 300 es");
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

#ifdef _WIN32
// workaround for: https://github.com/glfw/glfw/issues/2074
// borderless window: https://github.com/rossy/borderless-window
LRESULT CALLBACK Window::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  auto win = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  switch (uMsg) {
    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
      ::SetTimer(hWnd, reinterpret_cast<UINT_PTR>(win), USER_TIMER_MINIMUM, nullptr);
      break;
    case WM_TIMER:
      if (wParam == reinterpret_cast<UINT_PTR>(win)) {
        win->mpv->waitEvent();
        win->dispatch.process();
      }
      break;
    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
      ::KillTimer(hWnd, reinterpret_cast<UINT_PTR>(win));
      break;
    case WM_NCACTIVATE:
    case WM_NCPAINT:
      if (win->borderless) return DefWindowProcW(hWnd, uMsg, wParam, lParam);
      break;
    case WM_NCCALCSIZE: {
      if (!win->borderless) break;

      RECT& rect = *reinterpret_cast<RECT*>(lParam);
      RECT client = rect;

      DefWindowProcW(hWnd, uMsg, wParam, lParam);

      if (IsMaximized(hWnd)) {
        HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = {.cbSize = sizeof mi};
        GetMonitorInfoW(mon, &mi);
        rect = mi.rcWork;
      } else {
        rect = client;
      }

      return 0;
    }
    case WM_NCHITTEST: {
      if (!win->borderless) break;
      if (IsMaximized(hWnd)) return HTCLIENT;

      POINT mouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      ScreenToClient(hWnd, &mouse);
      RECT client;
      GetClientRect(hWnd, &client);

      int frame_size = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
      int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

      if (mouse.y < frame_size) {
        if (mouse.x < diagonal_width) return HTTOPLEFT;
        if (mouse.x >= client.right - diagonal_width) return HTTOPRIGHT;
        return HTTOP;
      }

      if (mouse.y >= client.bottom - frame_size) {
        if (mouse.x < diagonal_width) return HTBOTTOMLEFT;
        if (mouse.x >= client.right - diagonal_width) return HTBOTTOMRIGHT;
        return HTBOTTOM;
      }

      if (mouse.x < frame_size) return HTLEFT;
      if (mouse.x >= client.right - frame_size) return HTRIGHT;

      return HTCLIENT;
    } break;
  }
  return ::CallWindowProc(win->wndProcOld, hWnd, uMsg, wParam, lParam);
}
#endif
}  // namespace ImPlay