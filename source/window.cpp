#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <fonts/fontawesome.h>
#include <fonts/unifont.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include "window.h"
#include "dispatch.h"

namespace ImPlay {
Window::Window(const char* title, int width, int height) {
  this->title = title;
  this->width = width;
  this->height = height;

  initGLFW();
  initImGui();

  player = new Player(window, title);
}

Window::~Window() {
  delete player;

  exitImGui();
  exitGLFW();
}

bool Window::run(int argc, char* argv[]) {
  glfwMakeContextCurrent(window);
  if (!player->init(argc, argv)) return false;
  glfwShowWindow(window);
  glfwMakeContextCurrent(nullptr);

  std::atomic_bool shutdown = false;

  std::thread renderThread([&]() {
    while (!glfwWindowShouldClose(window)) {
      {
        std::unique_lock<std::mutex> lk(renderMutex);
        auto timeout = std::chrono::milliseconds(waitTimeout);
        renderCond.wait_for(lk, timeout, [&]() { return wantRender; });
        wantRender = false;
      }
      render();
    }
    shutdown = true;
  });

  while (!shutdown) {
    glfwWaitEvents();
    player->waitEvent();

    if (player->wantRender()) requestRender();

    updateWaitTimeout();

    dispatch_process();
  }

  renderThread.join();

  return true;
}

void Window::updateWaitTimeout() {
  bool shouldRender = (glfwGetWindowAttrib(window, GLFW_VISIBLE) && !glfwGetWindowAttrib(window, GLFW_ICONIFIED)) ||
                      (ImGui::GetIO().MetricsActiveWindows > 1);
  int delta = ImGui::GetCurrentContext()->Viewports.size() > 1 ? 0 : (glfwGetTime() - lastUserInput) * 1000;
  waitTimeout = shouldRender ? std::max(defaultWaitTimeout, delta) : std::numeric_limits<int>::max();
}

void Window::render() {
  glfwMakeContextCurrent(window);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  player->draw();
  ImGui::Render();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  player->render(width, height);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
  glfwMakeContextCurrent(nullptr);

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    dispatch_sync(
        [&](void* data) {
          ImGui::UpdatePlatformWindows();
          ImGui::RenderPlatformWindowsDefault();
        },
        nullptr);
  }
}

void Window::requestRender() {
  std::unique_lock<std::mutex> lk(renderMutex);
  wantRender = true;
  lk.unlock();
  renderCond.notify_one();
}

void Window::wakeupRender() {
  if (glfwGetTime() - lastUserInput < (double)defaultWaitTimeout / 1000) return;
  lastUserInput = glfwGetTime();
  requestRender();
}

void Window::initGLFW() {
  if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW!" << std::endl;
    std::abort();
  }

#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Failed to create window!" << std::endl;
    std::abort();
  }

  glfwSetWindowUserPointer(window, this);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwMakeContextCurrent(nullptr);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2);

  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int w, int h) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->width = w;
    win->height = h;
    glViewport(0, 0, w, h);
  });
  glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->shutdown();
  });
  glfwSetWindowRefreshCallback(window, [](GLFWwindow* window) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->requestRender();
    dispatch_process();
  });
  glfwSetWindowPosCallback(window, [](GLFWwindow* window, int x, int y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->requestRender();
    dispatch_process();
  });
  glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->wakeupRender();
    if (!ImGui::GetIO().WantCaptureMouse) win->player->setCursor(x, y);
  });
  glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->wakeupRender();
    if (!ImGui::GetIO().WantCaptureMouse) win->player->setMouse(button, action, mods);
  });
  glfwSetScrollCallback(window, [](GLFWwindow* window, double x, double y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->wakeupRender();
    if (!ImGui::GetIO().WantCaptureMouse) win->player->setScroll(x, y);
  });
  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->wakeupRender();
    if (!ImGui::GetIO().WantCaptureKeyboard) win->player->setKey(key, scancode, action, mods);
  });
  glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char* paths[]) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!ImGui::GetIO().WantCaptureMouse) win->player->setDrop(count, paths);
  });
}

void Window::initImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  float scale = 1.0f;
  glfwGetWindowContentScale(window, &scale, nullptr);
#if defined(__APPLE__)
  scale /= 2.0f;
#endif
  float fontSize = 13.0f * scale;
  float iconSize = 11.0f * scale;

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.DisplayFramebufferScale = ImVec2(scale, scale);

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(scale);

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  io.Fonts->AddFontDefault(&cfg);
  cfg.MergeMode = true;
  ImWchar fontAwesomeRange[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  ImWchar unifontRange[] = {0x0100, 0xFFF0, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data, font_awesome_compressed_size, iconSize, &cfg,
                                           fontAwesomeRange);
  io.Fonts->AddFontFromMemoryCompressedTTF(unifont_compressed_data, unifont_compressed_size, 0, &cfg, unifontRange);
  io.Fonts->Build();

  glfwMakeContextCurrent(window);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
#if defined(__APPLE__)
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