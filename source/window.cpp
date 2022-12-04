#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <iostream>
#include "window.h"
#include "fontawesome.h"

namespace ImPlay {
Window::Window(const char* title, int width, int height) {
  this->title = title;
  this->width = width;
  this->height = height;

  initGLFW();
  initImGui();

  player = new Player(title);
  show();
}

Window::~Window() {
  delete player;
  exitImGui();
  exitGLFW();
}

void Window::loop() {
  while (!glfwWindowShouldClose(window)) {
    if (!glfwGetWindowAttrib(window, GLFW_VISIBLE) || glfwGetWindowAttrib(window, GLFW_ICONIFIED))
      glfwWaitEvents();
    else
      glfwPollEvents();

    player->pollEvent();

    render();
  }
}

void Window::render() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  player->draw();

  ImGui::Render();

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  player->render(viewport->WorkSize.x, viewport->WorkSize.y);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow* backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }
  glfwSwapBuffers(window);
}

void Window::redraw() {
  if (auto g = ImGui::GetCurrentContext(); g == nullptr || g->WithinFrameScope) return;
  render();
}

void Window::show() { glfwShowWindow(window); }

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

  window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create window!" << std::endl;
    std::abort();
  }

  glfwSetWindowUserPointer(window, this);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2);

  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int w, int h) { glViewport(0, 0, w, h); });
  glfwSetWindowPosCallback(window, [](GLFWwindow* window, int x, int y) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->redraw();
  });
  glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int w, int h) {
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->redraw();
  });
  glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->setCursor(x, y);
  });
  glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->setMouse(button, action, mods);
  });
  glfwSetScrollCallback(window, [](GLFWwindow* window, double x, double y) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->setScroll(x, y);
  });
  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->setKey(key, scancode, action, mods);
  });
  glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char* paths[]) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->player->setDrop(count, paths);
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
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  io.Fonts->AddFontDefault(&cfg);
  cfg.MergeMode = true;
  ImWchar fontAwesomeRange[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data, font_awesome_compressed_size, iconSize, &cfg,
                                           fontAwesomeRange);
  io.Fonts->Build();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
#if defined(__APPLE__)
  ImGui_ImplOpenGL3_Init("#version 150");
#else
  ImGui_ImplOpenGL3_Init("#version 130");
#endif
}

void Window::exitGLFW() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::exitImGui() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
}  // namespace ImPlay