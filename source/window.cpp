#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include <iostream>
#include "window.h"
#include "input.h"
#include "fontawesome.h"

namespace mpv {
Window::Window() {
  initGLFW();
  initImGui();
  NFD::Init();
  initPlayer();
  show();
}

Window::~Window() {
  NFD::Quit();
  delete player;
  exitImGui();
  exitGLFW();
}

void Window::loop() {
  while (!glfwWindowShouldClose(window)) {
    if (!glfwGetWindowAttrib(window, GLFW_VISIBLE))
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

  draw();

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
  if (auto g = ImGui::GetCurrentContext(); g == nullptr || g->WithinFrameScope)
    return;
  render();
}

void Window::show() { glfwShowWindow(window); }

void Window::draw() {
  if (demo) ImGui::ShowDemoWindow(&demo);

  drawContextMenu();
}

void Window::drawContextMenu() {
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
      !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    ImGui::OpenPopup("Context Menu");

  if (ImGui::BeginPopup("Context Menu", ImGuiWindowFlags_NoMove)) {
    if (ImGui::MenuItem(paused ? ICON_FA_PLAY " Play" : ICON_FA_PAUSE " Pause",
                        "Space", nullptr, loaded))
      player->command("cycle pause");
    if (ImGui::MenuItem(ICON_FA_STOP " Stop", nullptr, nullptr, loaded))
      player->command("stop");
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_FORWARD " Jump Forward", "UP", nullptr, loaded))
      player->command("seek 10");
    if (ImGui::MenuItem(ICON_FA_BACKWARD " Jump Backward", "DOWN", nullptr,
                        loaded))
      player->command("seek -10");
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_ARROW_LEFT " Previous", "<"))
      player->command("playlist-prev");
    if (ImGui::MenuItem(ICON_FA_ARROW_RIGHT " Next", ">"))
      player->command("playlist-next");
    ImGui::Separator();
    if (ImGui::BeginMenu(ICON_FA_FILE_AUDIO " Audio")) {
      if (ImGui::MenuItem(ICON_FA_LIST " Tracks", "#", nullptr, loaded))
        player->command("cycle audio");
      if (ImGui::MenuItem(ICON_FA_VOLUME_UP " Increase Volume", "0"))
        player->command("add volume 2");
      if (ImGui::MenuItem(ICON_FA_VOLUME_DOWN " Decrease Volume", "9"))
        player->command("add volume -2");
      if (ImGui::MenuItem(ICON_FA_VOLUME_MUTE " Mute", "m"))
        player->command("cycle mute");
      if (ImGui::MenuItem("  Increase Delay", "Ctrl +"))
        player->command("add audio-delay 0.1");
      if (ImGui::MenuItem("  Decrease Delay", "Ctrl -"))
        player->command("add audio-delay -0.1");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_VIDEO " Video")) {
      if (ImGui::MenuItem(ICON_FA_LIST " Tracks", "_", nullptr, loaded))
        player->command("cycle video");
      if (ImGui::MenuItem(ICON_FA_SPINNER " Rotate"))
        player->command("cycle-values video-rotate 0 90 180 270");
      if (ImGui::MenuItem(ICON_FA_MINUS_CIRCLE " Zoom In", "Alt +"))
        player->command("add video-zoom  -0.1");
      if (ImGui::MenuItem(ICON_FA_PLUS_CIRCLE " Zoom Out", "Alt -"))
        player->command("add video-zoom  0.1");
      if (ImGui::MenuItem("  HW Decoding", "Ctrl h"))
        player->command("cycle-values hwdec auto no");
      if (ImGui::BeginMenu("  Effect")) {
        if (ImGui::MenuItem("  Increase Contrast", "2"))
          player->command("add contrast 1");
        if (ImGui::MenuItem("  Decrease Contrast", "1"))
          player->command("add contrast -1");
        if (ImGui::MenuItem("  Increase Brightness", "4"))
          player->command("add brightness 1");
        if (ImGui::MenuItem("  Decrease Brightness", "3"))
          player->command("add brightness -1");
        if (ImGui::MenuItem("  Increase Gamma", "6"))
          player->command("add gamma 1");
        if (ImGui::MenuItem("  Decrease Gamma", "5"))
          player->command("add gamma -1");
        if (ImGui::MenuItem("  Increase Saturation", "8"))
          player->command("add saturation 1");
        if (ImGui::MenuItem("  Decrease Saturation", "7"))
          player->command("add saturation -1");
        if (ImGui::MenuItem("  Increase Hue")) player->command("add hue 1");
        if (ImGui::MenuItem("  Decrease Hue")) player->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_FONT " Subtitle")) {
      if (ImGui::MenuItem(ICON_FA_LIST " Tracks", "j", nullptr, loaded))
        player->command("cycle sub");
      if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load..")) loadSub();
      if (ImGui::MenuItem("  Increase Delay", "z"))
        player->command("add sub-delay 0.1");
      if (ImGui::MenuItem("  Decrease Delay", "Z"))
        player->command("add sub-delay -0.1");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_LIST " Playlist", "F8"))
      player->command("show-text ${playlist}");
    if (ImGui::MenuItem(ICON_FA_LIST " Tracklist", "F9"))
      player->command("show-text ${track-list}");
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_EXPAND " Fullscreen", "f"))
      player->command("cycle fullscreen");
    if (ImGui::MenuItem(ICON_FA_ARROW_UP " Always Ontop", "T"))
      player->command("cycle ontop");
    if (ImGui::MenuItem(ICON_FA_SPINNER " Show Progress", "o"))
      player->command("show-progress");
    ImGui::Separator();
    if (ImGui::BeginMenu(ICON_FA_SCREWDRIVER " Tools")) {
      if (ImGui::MenuItem(ICON_FA_FILE_IMAGE " Screenshot", "s"))
        player->command("screenshot");
      if (ImGui::MenuItem(ICON_FA_BORDER_NONE " Window Border"))
        player->command("cycle border");
      if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE " Media Info", "i"))
        player->command("script-binding stats/display-stats");
      if (ImGui::MenuItem(ICON_FA_LINK " Show Keybindings"))
        player->command("script-binding stats/display-page-4");
      if (ImGui::MenuItem("  OSC visibility", "DEL"))
        player->command("script-binding osc/visibility");
      if (ImGui::MenuItem("  Script Console", "`"))
        player->command("script-binding console/enable");
      if (ImGui::BeginMenu("  ImGui Theme")) {
        if (ImGui::MenuItem("  Dark", nullptr, theme == Theme::DARK))
          setTheme(Theme::DARK);
        if (ImGui::MenuItem("  Light", nullptr, theme == Theme::LIGHT))
          setTheme(Theme::LIGHT);
        if (ImGui::MenuItem("  Classic", nullptr, theme == Theme::CLASSIC))
          setTheme(Theme::CLASSIC);
        ImGui::EndMenu();
      }
      ImGui::MenuItem("  ImGui Demo", nullptr, &demo);
      ImGui::EndMenu();
    }
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open..")) openFile();
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_WINDOW_CLOSE " Quit", "q"))
      glfwSetWindowShouldClose(window, true);
    ImGui::EndPopup();
  }
}

void Window::openFile() {
  nfdchar_t* outPath;
  if (NFD::OpenDialog(outPath) == NFD_OKAY) {
    const char* cmd[] = {"loadfile", outPath, "replace", NULL};
    player->command(cmd);
    free(outPath);
  }
}

void Window::loadSub() {
  nfdchar_t* outPath;
  if (NFD::OpenDialog(outPath) == NFD_OKAY) {
    const char* cmd[] = {"sub-add", outPath, "select", NULL};
    player->command(cmd);
    free(outPath);
  }
}

void Window::setTheme(Theme theme) {
  switch (theme) {
    case Theme::DARK:
      ImGui::StyleColorsDark();
      break;
    case Theme::LIGHT:
      ImGui::StyleColorsLight();
      break;
    case Theme::CLASSIC:
      ImGui::StyleColorsClassic();
      break;
  }
  this->theme = theme;
}

void Window::initPlayer() {
  player = new Player();

  player->observeEvent(MPV_EVENT_SHUTDOWN, [=, this](void* data) {
    glfwSetWindowShouldClose(window, true);
  });

  player->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [=, this](void* data) {
    int64_t w{0}, h{0};
    if (player->property("dwidth", w) || player->property("dheight", h)) return;
    if (w > 0 && h > 0) {
      glfwSetWindowSize(window, w, h);
      glfwSetWindowAspectRatio(window, w, h);
    }
  });

  player->observeEvent(MPV_EVENT_START_FILE,
                       [=, this](void* data) { loaded = true; });

  player->observeEvent(MPV_EVENT_END_FILE,
                       [=, this](void* data) { loaded = false; });

  player->observeProperty("media-title", MPV_FORMAT_STRING,
                          [=, this](void* data) {
                            char* title = static_cast<char*>(*(char**)data);
                            glfwSetWindowTitle(window, title);
                          });

  player->observeProperty("border", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    glfwSetWindowAttrib(window, GLFW_DECORATED,
                        enable ? GLFW_TRUE : GLFW_FALSE);
  });

  player->observeProperty("window-maximized", MPV_FORMAT_FLAG,
                          [=, this](void* data) {
                            bool enable = static_cast<bool>(*(int*)data);
                            if (enable)
                              glfwMaximizeWindow(window);
                            else
                              glfwRestoreWindow(window);
                          });

  player->observeProperty("window-minimized", MPV_FORMAT_FLAG,
                          [=, this](void* data) {
                            bool enable = static_cast<bool>(*(int*)data);
                            if (enable)
                              glfwIconifyWindow(window);
                            else
                              glfwRestoreWindow(window);
                          });

  player->observeProperty("fullscreen", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
    if (isFullscreen == enable) return;

    static int x, y, w, h;
    if (enable) {
      glfwGetWindowPos(window, &x, &y);
      glfwGetWindowSize(window, &w, &h);
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                           mode->refreshRate);
    } else
      glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
  });

  player->observeProperty("ontop", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    glfwSetWindowAttrib(window, GLFW_FLOATING, enable ? GLFW_TRUE : GLFW_FALSE);
  });

  player->observeProperty("pause", MPV_FORMAT_FLAG, [=, this](void* data) {
    paused = static_cast<bool>(*(int*)data);
  });

  player->command("keybind MBTN_RIGHT ignore");
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

  window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
  if (window == nullptr) {
    std::cout << "Failed to create window!" << std::endl;
    std::abort();
  }

  glfwSetWindowUserPointer(window, this);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  int width, height;
  glfwGetWindowSize(window, &width, &height);
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwSetWindowPos(window, (mode->width - width) / 2,
                   (mode->height - height) / 2);

  glfwSetFramebufferSizeCallback(window, input::fb_size_cb);
  glfwSetWindowPosCallback(window, input::win_pos_cb);
  glfwSetWindowSizeCallback(window, input::win_size_cb);
  glfwSetCursorPosCallback(window, input::cursor_pos_cb);
  glfwSetMouseButtonCallback(window, input::mouse_button_cb);
  glfwSetScrollCallback(window, input::scroll_cb);
  glfwSetKeyCallback(window, input::key_cb);
  glfwSetDropCallback(window, input::drop_cb);
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
  setTheme(Theme::DARK);

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  io.Fonts->AddFontDefault(&cfg);
  cfg.MergeMode = true;
  ImWchar fontAwesomeRange[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_compressed_data,
                                           font_awesome_compressed_size,
                                           iconSize, &cfg, fontAwesomeRange);
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
}  // namespace mpv