// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <strnatcmp.h>
#ifdef _WIN32
#include <windowsx.h>
#endif
#include "theme.h"
#include "window.h"

namespace ImPlay {
Window::Window(Config* config) : Player(config) {
  initGLFW();
  window = glfwCreateWindow(1280, 720, PLAYER_NAME, nullptr, nullptr);
  if (window == nullptr) throw std::runtime_error("Failed to create window!");
#ifdef _WIN32
  hwnd = glfwGetWin32Window(window);
  if (SUCCEEDED(OleInitialize(nullptr))) oleOk = true;
#endif

  initGui();
  installCallbacks(window);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
}

Window::~Window() {
  ImGui_ImplGlfw_Shutdown();
  exitGui();
#ifdef _WIN32
  if (taskbarList != nullptr) taskbarList->Release();
  if (oleOk) OleUninitialize();
#endif

  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::initGLFW() {
  glfwSetErrorCallback(
      [](int error, const char* desc) { fmt::print(fg(fmt::color::red), "GLFW [{}]: {}\n", error, desc); });
#ifdef GLFW_PATCHED
  glfwInitHint(GLFW_WIN32_MESSAGES_IN_FIBER, GLFW_TRUE);
#endif
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
  glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
}

bool Window::init(OptionParser& parser) {
  mpv->wakeupCb() = [this](Mpv* ctx) { wakeup(); };
  mpv->updateCb() = [this](Mpv* ctx) { videoWaiter.notify(); };
  if (!Player::init(parser.options)) return false;

  for (auto& path : parser.paths) {
    if (path == "-") mpv->property("input-terminal", "yes");
    mpv->commandv("loadfile", path.c_str(), "append-play", nullptr);
  }
#if defined(__APPLE__) && defined(GLFW_PATCHED)
  const char** openedFileNames = glfwGetOpenedFilenames();
  if (openedFileNames != nullptr) {
    int count = 0;
    while (openedFileNames[count] != nullptr) count++;
    onDropEvent(count, openedFileNames);
  }
#endif

#ifdef _WIN32
  if (oleOk) setupWin32Taskbar();
  wndProcOld = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
  ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProc));
  ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("border", [this](int flag) {
    borderless = !static_cast<bool>(flag);
    if (borderless) {
      DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
      ::SetWindowLong(hwnd, GWL_STYLE, style | WS_CAPTION | WS_MAXIMIZEBOX | WS_THICKFRAME);
    }
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
  });
#endif
  return true;
}

void Window::run() {
  bool shutdown = false;
  std::thread videoRenderer([&]() {
    while (!shutdown) {
      videoWaiter.wait();
      if (shutdown) break;

      if (mpv->wantRender()) {
        renderVideo();
        wakeup();
      }
    }
  });

  restoreState();
  glfwShowWindow(window);

  double lastTime = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
    if (!glfwGetWindowAttrib(window, GLFW_VISIBLE) || glfwGetWindowAttrib(window, GLFW_ICONIFIED))
      glfwWaitEvents();
    else
      glfwPollEvents();

    mpv->waitEvent();

    render();
    updateCursor();

    double targetDelta = 1.0f / config->Data.Interface.Fps;
    double delta = lastTime - glfwGetTime();
    if (delta > 0 && delta < targetDelta)
      glfwWaitEventsTimeout(delta);
    else
      lastTime = glfwGetTime();
    lastTime += targetDelta;
  }

  shutdown = true;
  videoWaiter.notify();
  videoRenderer.join();

  saveState();
}

void Window::wakeup() { glfwPostEmptyEvent(); }

void Window::updateCursor() {
  if (!ownCursor || mpv->cursorAutohide == "" || ImGui::GetIO().WantCaptureMouse || ImGui::IsMouseDragging(0)) return;

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

void Window::installCallbacks(GLFWwindow* target) {
  glfwSetWindowUserPointer(target, this);

  glfwSetWindowContentScaleCallback(target, [](GLFWwindow* window, float x, float y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->config->Data.Interface.Scale = std::max(x, y);
    win->config->FontReload = true;
  });
  glfwSetWindowCloseCallback(target, [](GLFWwindow* window) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->shutdown();
  });
  glfwSetWindowSizeCallback(target, [](GLFWwindow* window, int w, int h) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->render();
  });
  glfwSetWindowPosCallback(target, [](GLFWwindow* window, int x, int y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->render();
  });
  glfwSetCursorEnterCallback(target, [](GLFWwindow* window, int entered) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->ownCursor = entered;
  });
  glfwSetCursorPosCallback(target, [](GLFWwindow* window, double x, double y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (ImGui::GetIO().WantCaptureMouse) return;
#ifdef __APPLE__
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    x *= xscale;
    y *= yscale;
#endif
    win->onCursorEvent(x, y);
#ifdef GLFW_PATCHED
    if (win->mpv->allowDrag() && win->height - y > 280) {  // 280: height of the OSC bar
      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) glfwDragWindow(window);
    }
#endif
  });
  glfwSetMouseButtonCallback(target, [](GLFWwindow* window, int button, int action, int mods) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (!ImGui::GetIO().WantCaptureMouse) win->handleMouse(button, action, mods);
  });
  glfwSetScrollCallback(target, [](GLFWwindow* window, double x, double y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (!ImGui::GetIO().WantCaptureMouse) win->onScrollEvent(x, y);
  });
  glfwSetKeyCallback(target, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->lastInputAt = glfwGetTime();
    if (!ImGui::GetIO().WantCaptureKeyboard) win->handleKey(key, action, mods);
  });
  glfwSetDropCallback(target, [](GLFWwindow* window, int count, const char** paths) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!ImGui::GetIO().WantCaptureMouse) win->onDropEvent(count, paths);
  });
}

void Window::handleKey(int key, int action, int mods) {
  std::string name;
  if (mods & GLFW_MOD_SHIFT) {
    if (auto s = shiftMappings.find(key); s != shiftMappings.end()) {
      name = s->second;
      mods &= ~GLFW_MOD_SHIFT;
    }
  }
  if (name.empty()) {
    auto s = keyMappings.find(key);
    if (s == keyMappings.end()) return;
    name = s->second;
  }

  std::vector<std::string> keys;
  translateMod(keys, mods);
  keys.push_back(name);
  sendKeyEvent(fmt::format("{}", join(keys, "+")), action);
}

void Window::handleMouse(int button, int action, int mods) {
  std::vector<std::string> keys;
  translateMod(keys, mods);
  auto s = mbtnMappings.find(button);
  if (s == mbtnMappings.end()) return;
  keys.push_back(s->second);
  sendKeyEvent(fmt::format("{}", join(keys, "+")), action);
}

void Window::sendKeyEvent(std::string key, bool action) {
  if (action == GLFW_PRESS)
    onKeyDownEvent(key);
  else if (action == GLFW_RELEASE)
    onKeyUpEvent(key);
}

void Window::translateMod(std::vector<std::string>& keys, int mods) {
  if (mods & GLFW_MOD_CONTROL) keys.emplace_back("Ctrl");
  if (mods & GLFW_MOD_ALT) keys.emplace_back("Alt");
  if (mods & GLFW_MOD_SHIFT) keys.emplace_back("Shift");
  if (mods & GLFW_MOD_SUPER) keys.emplace_back("Meta");
}

#ifdef _WIN32
int64_t Window::GetWid() { return config->Data.Mpv.UseWid ? static_cast<uint32_t>((intptr_t)hwnd) : 0; }
#endif

GLAddrLoadFunc Window::GetGLAddrFunc() { return (GLAddrLoadFunc)glfwGetProcAddress; }

std::string Window::GetClipboardString() {
  const char* s = glfwGetClipboardString(window);
  return s != nullptr ? s : "";
}

void Window::GetMonitorSize(int* w, int* h) {
  const GLFWvidmode* mode = glfwGetVideoMode(getMonitor(window));
  *w = mode->width;
  *h = mode->height;
}

int Window::GetMonitorRefreshRate() {
  const GLFWvidmode* mode = glfwGetVideoMode(getMonitor(window));
  return mode->refreshRate;
}

void Window::GetFramebufferSize(int* w, int* h) { glfwGetFramebufferSize(window, w, h); }

void Window::MakeContextCurrent() { glfwMakeContextCurrent(window); }

void Window::DeleteContext() { glfwMakeContextCurrent(nullptr); }

void Window::SwapBuffers() { glfwSwapBuffers(window); }

void Window::SetSwapInterval(int interval) { glfwSwapInterval(interval); }

void Window::BackendNewFrame() { ImGui_ImplGlfw_NewFrame(); };

void Window::GetWindowScale(float* x, float* y) { glfwGetWindowContentScale(window, x, y); }

void Window::GetWindowPos(int* x, int* y) { glfwGetWindowPos(window, x, y); }

void Window::SetWindowPos(int x, int y) { glfwSetWindowPos(window, x, y); }

void Window::GetWindowSize(int* w, int* h) { glfwGetWindowSize(window, w, h); }

void Window::SetWindowSize(int w, int h) { glfwSetWindowSize(window, w, h); }

void Window::SetWindowTitle(std::string title) { glfwSetWindowTitle(window, title.c_str()); }

void Window::SetWindowAspectRatio(int num, int den) { glfwSetWindowAspectRatio(window, num, den); }

void Window::SetWindowMaximized(bool m) {
  if (m)
    glfwMaximizeWindow(window);
  else
    glfwRestoreWindow(window);
}

void Window::SetWindowMinimized(bool m) {
  if (m)
    glfwIconifyWindow(window);
  else
    glfwRestoreWindow(window);
}

void Window::SetWindowDecorated(bool d) { glfwSetWindowAttrib(window, GLFW_DECORATED, d); }

void Window::SetWindowFloating(bool f) { glfwSetWindowAttrib(window, GLFW_FLOATING, f); }

void Window::SetWindowFullscreen(bool fs) {
  bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
  if (isFullscreen == fs) return;

  static int x, y, w, h;
  if (fs) {
    glfwGetWindowPos(window, &x, &y);
    glfwGetWindowSize(window, &w, &h);
    GLFWmonitor* monitor = getMonitor(window);
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  } else
    glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
}

void Window::SetWindowShouldClose(bool c) { glfwSetWindowShouldClose(window, c); }

GLFWmonitor* Window::getMonitor(GLFWwindow* target) {
  int n, wx, wy, ww, wh, mx, my;
  int bestoverlap = 0;

  glfwGetWindowPos(target, &wx, &wy);
  glfwGetWindowSize(target, &ww, &wh);
  GLFWmonitor* bestmonitor = nullptr;
  auto monitors = glfwGetMonitors(&n);

  for (int i = 0; i < n; i++) {
    const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
    glfwGetMonitorPos(monitors[i], &mx, &my);
    int overlap = std::max(0, std::min(wx + ww, mx + mode->width) - std::max(wx, mx)) *
                  std::max(0, std::min(wy + wh, my + mode->height) - std::max(wy, my));
    if (bestoverlap < overlap) {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }

  return bestmonitor != nullptr ? bestmonitor : glfwGetPrimaryMonitor();
}

#ifdef _WIN32
void Window::setupWin32Taskbar() {
  if (FAILED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (void**)&taskbarList))) return;
  if (FAILED(taskbarList->HrInit())) {
    taskbarList->Release();
    taskbarList = nullptr;
    return;
  }
  mpv->observeEvent(MPV_EVENT_START_FILE, [this](void*) { taskbarList->SetProgressState(hwnd, TBPF_NORMAL); });
  mpv->observeEvent(MPV_EVENT_END_FILE, [this](void*) { taskbarList->SetProgressState(hwnd, TBPF_NOPROGRESS); });
  mpv->observeProperty<int64_t, MPV_FORMAT_INT64>("percent-pos", [this](int64_t pos) {
    if (pos > 0) taskbarList->SetProgressValue(hwnd, pos, 100);
  });
}

// borderless window: https://github.com/rossy/borderless-window
LRESULT CALLBACK Window::wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  auto win = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  switch (uMsg) {
    case WM_NCLBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCXBUTTONDOWN:
      if (win->config->Data.Mpv.UseWid && ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopup)) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddFocusEvent(false); // may close popup
      }
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

void Window::Waiter::wait() {
  std::unique_lock<std::mutex> l(lock);
  cond.wait(l, [this] { return notified; });
  notified = false;
}

void Window::Waiter::wait_until(std::chrono::steady_clock::time_point time) {
  std::unique_lock<std::mutex> l(lock);
  cond.wait_until(l, time, [this] { return notified; });
  notified = false;
}

void Window::Waiter::notify() {
  {
    std::lock_guard<std::mutex> l(lock);
    notified = true;
  }
  cond.notify_one();
}
}  // namespace ImPlay