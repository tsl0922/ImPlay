#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.hpp>
#include "player.h"
#include "fontawesome.h"

namespace ImPlay {
Player::Player() {
  window = glfwGetCurrentContext();
  mpv = new Mpv();

  initMpv();
  NFD::Init();
  setTheme(Theme::DARK);
}

Player::~Player() {
  NFD::Quit();
  delete this->mpv;
}

void Player::draw() {
  if (demo) ImGui::ShowDemoWindow(&demo);
  drawContextMenu();
}

void Player::render(int w, int h) { mpv->render(w, h); }

void Player::pollEvent() { mpv->pollEvent(); }

void Player::setCursor(double x, double y) {
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  const char* args[] = {"mouse", xs.c_str(), ys.c_str(), NULL};
  mpv->command(args);
}

void Player::setMouse(int button, int action, int mods) {
  auto s = actionMappings.find(action);
  if (s == actionMappings.end()) return;
  const std::string cmd = s->second;

  std::vector<std::string> keys;
  translateMod(keys, mods);
  s = mbtnMappings.find(button);
  if (s == actionMappings.end()) return;
  keys.push_back(s->second);
  const std::string arg = join(keys, "+");

  const char* args[] = {cmd.c_str(), arg.c_str(), NULL};
  mpv->command(args);
}

void Player::setScroll(double x, double y) {
  if (abs(x) > 0) {
    mpv->command(x > 0 ? "keypress WHEEL_LEFT" : "keypress WHEEL_RIGH");
    mpv->command(x > 0 ? "keyup WHEEL_LEFT" : "keyup WHEEL_RIGH");
  }
  if (abs(y) > 0) {
    mpv->command(y > 0 ? "keypress WHEEL_UP" : "keypress WHEEL_DOWN");
    mpv->command(y > 0 ? "keyup WHEEL_UP" : "keyup WHEEL_DOWN");
  }
}

void Player::setKey(int key, int scancode, int action, int mods) {
  auto s = actionMappings.find(action);
  if (s == actionMappings.end()) return;
  const std::string cmd = s->second;

  std::string name;
  if (mods & GLFW_MOD_SHIFT) {
    s = shiftMappings.find(key);
    if (s != shiftMappings.end()) {
      name = s->second;
      mods &= ~GLFW_MOD_SHIFT;
    }
  }
  if (name.empty()) {
    s = keyMappings.find(key);
    if (s == keyMappings.end()) return;
    name = s->second;
  }

  std::vector<std::string> keys;
  translateMod(keys, mods);
  keys.push_back(name);
  const std::string arg = join(keys, "+");

  const char* args[] = {cmd.c_str(), arg.c_str(), NULL};
  mpv->command(args);
}

void Player::setDrop(int count, const char** paths) {
  for (int i = 0; i < count; i++) {
    const char* cmd[] = {"loadfile", paths[i], i > 0 ? "append-play" : "replace", NULL};
    mpv->command(cmd);
  }
}

void Player::drawContextMenu() {
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    ImGui::OpenPopup("Context Menu");

  if (ImGui::BeginPopup("Context Menu", ImGuiWindowFlags_NoMove)) {
    if (ImGui::MenuItem(paused ? ICON_FA_PLAY " Play" : ICON_FA_PAUSE " Pause", "Space", nullptr, loaded))
      mpv->command("cycle pause");
    if (ImGui::MenuItem(ICON_FA_STOP " Stop", nullptr, nullptr, loaded)) mpv->command("stop");
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_FORWARD " Jump Forward", "UP", nullptr, loaded)) mpv->command("seek 10");
    if (ImGui::MenuItem(ICON_FA_BACKWARD " Jump Backward", "DOWN", nullptr, loaded)) mpv->command("seek -10");
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_ARROW_LEFT " Previous", "<")) mpv->command("playlist-prev");
    if (ImGui::MenuItem(ICON_FA_ARROW_RIGHT " Next", ">")) mpv->command("playlist-next");
    ImGui::Separator();
    if (ImGui::BeginMenu(ICON_FA_FILE_AUDIO " Audio")) {
      if (ImGui::MenuItem(ICON_FA_LIST " Tracks", "#", nullptr, loaded)) mpv->command("cycle audio");
      if (ImGui::MenuItem(ICON_FA_VOLUME_UP " Increase Volume", "0")) mpv->command("add volume 2");
      if (ImGui::MenuItem(ICON_FA_VOLUME_DOWN " Decrease Volume", "9")) mpv->command("add volume -2");
      if (ImGui::MenuItem(ICON_FA_VOLUME_MUTE " Mute", "m")) mpv->command("cycle mute");
      if (ImGui::MenuItem("  Increase Delay", "Ctrl +")) mpv->command("add audio-delay 0.1");
      if (ImGui::MenuItem("  Decrease Delay", "Ctrl -")) mpv->command("add audio-delay -0.1");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_VIDEO " Video")) {
      if (ImGui::MenuItem(ICON_FA_LIST " Tracks", "_", nullptr, loaded)) mpv->command("cycle video");
      if (ImGui::MenuItem(ICON_FA_SPINNER " Rotate")) mpv->command("cycle-values video-rotate 0 90 180 270");
      if (ImGui::MenuItem(ICON_FA_MINUS_CIRCLE " Zoom In", "Alt +")) mpv->command("add video-zoom  -0.1");
      if (ImGui::MenuItem(ICON_FA_PLUS_CIRCLE " Zoom Out", "Alt -")) mpv->command("add video-zoom  0.1");
      if (ImGui::MenuItem("  HW Decoding", "Ctrl h")) mpv->command("cycle-values hwdec auto no");
      if (ImGui::BeginMenu("  Effect")) {
        if (ImGui::MenuItem("  Increase Contrast", "2")) mpv->command("add contrast 1");
        if (ImGui::MenuItem("  Decrease Contrast", "1")) mpv->command("add contrast -1");
        if (ImGui::MenuItem("  Increase Brightness", "4")) mpv->command("add brightness 1");
        if (ImGui::MenuItem("  Decrease Brightness", "3")) mpv->command("add brightness -1");
        if (ImGui::MenuItem("  Increase Gamma", "6")) mpv->command("add gamma 1");
        if (ImGui::MenuItem("  Decrease Gamma", "5")) mpv->command("add gamma -1");
        if (ImGui::MenuItem("  Increase Saturation", "8")) mpv->command("add saturation 1");
        if (ImGui::MenuItem("  Decrease Saturation", "7")) mpv->command("add saturation -1");
        if (ImGui::MenuItem("  Increase Hue")) mpv->command("add hue 1");
        if (ImGui::MenuItem("  Decrease Hue")) mpv->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_FONT " Subtitle")) {
      if (ImGui::MenuItem(ICON_FA_LIST " Tracks", "j", nullptr, loaded)) mpv->command("cycle sub");
      if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Load..")) loadSub();
      if (ImGui::MenuItem("  Increase Delay", "z")) mpv->command("add sub-delay 0.1");
      if (ImGui::MenuItem("  Decrease Delay", "Z")) mpv->command("add sub-delay -0.1");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_LIST " Playlist", "F8")) mpv->command("show-text ${playlist}");
    if (ImGui::MenuItem(ICON_FA_LIST " Tracklist", "F9")) mpv->command("show-text ${track-list}");
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_EXPAND " Fullscreen", "f")) mpv->command("cycle fullscreen");
    if (ImGui::MenuItem(ICON_FA_ARROW_UP " Always Ontop", "T")) mpv->command("cycle ontop");
    if (ImGui::MenuItem(ICON_FA_SPINNER " Show Progress", "o")) mpv->command("show-progress");
    ImGui::Separator();
    if (ImGui::BeginMenu(ICON_FA_SCREWDRIVER " Tools")) {
      if (ImGui::MenuItem(ICON_FA_FILE_IMAGE " Screenshot", "s")) mpv->command("screenshot");
      if (ImGui::MenuItem(ICON_FA_BORDER_NONE " Window Border")) mpv->command("cycle border");
      if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE " Media Info", "i")) mpv->command("script-binding stats/display-stats");
      if (ImGui::MenuItem(ICON_FA_LINK " Show Keybindings")) mpv->command("script-binding stats/display-page-4");
      if (ImGui::MenuItem("  OSC visibility", "DEL")) mpv->command("script-binding osc/visibility");
      if (ImGui::MenuItem("  Script Console", "`")) mpv->command("script-binding console/enable");
      ImGui::MenuItem("  ImGui Demo", nullptr, &demo);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(ICON_FA_PALETTE " Theme")) {
      if (ImGui::MenuItem("  Dark", nullptr, theme == Theme::DARK)) setTheme(Theme::DARK);
      if (ImGui::MenuItem("  Light", nullptr, theme == Theme::LIGHT)) setTheme(Theme::LIGHT);
      if (ImGui::MenuItem("  Classic", nullptr, theme == Theme::CLASSIC)) setTheme(Theme::CLASSIC);
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open..")) openFile();
    if (ImGui::MenuItem(ICON_FA_WINDOW_CLOSE " Quit", "q")) mpv->command("quit");
    ImGui::EndPopup();
  }
}

void Player::openFile() {
  nfdchar_t* outPath;
  if (NFD::OpenDialog(outPath) == NFD_OKAY) {
    const char* cmd[] = {"loadfile", outPath, "replace", NULL};
    mpv->command(cmd);
    free(outPath);
  }
}

void Player::loadSub() {
  nfdchar_t* outPath;
  if (NFD::OpenDialog(outPath) == NFD_OKAY) {
    const char* cmd[] = {"sub-add", outPath, "select", NULL};
    mpv->command(cmd);
    free(outPath);
  }
}

void Player::initMpv() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [=, this](void* data) { glfwSetWindowShouldClose(window, true); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [=, this](void* data) {
    int64_t w{0}, h{0};
    if (mpv->property("dwidth", MPV_FORMAT_INT64, &w) || mpv->property("dheight", MPV_FORMAT_INT64, &h)) return;
    if (w > 0 && h > 0) {
      glfwSetWindowSize(window, w, h);
      glfwSetWindowAspectRatio(window, w, h);
    }
  });

  mpv->observeEvent(MPV_EVENT_START_FILE, [=, this](void* data) { loaded = true; });

  mpv->observeEvent(MPV_EVENT_END_FILE, [=, this](void* data) { loaded = false; });

  mpv->observeProperty("media-title", MPV_FORMAT_STRING, [=, this](void* data) {
    char* title = static_cast<char*>(*(char**)data);
    glfwSetWindowTitle(window, title);
  });

  mpv->observeProperty("border", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    glfwSetWindowAttrib(window, GLFW_DECORATED, enable ? GLFW_TRUE : GLFW_FALSE);
  });

  mpv->observeProperty("window-maximized", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    if (enable)
      glfwMaximizeWindow(window);
    else
      glfwRestoreWindow(window);
  });

  mpv->observeProperty("window-minimized", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    if (enable)
      glfwIconifyWindow(window);
    else
      glfwRestoreWindow(window);
  });

  mpv->observeProperty("fullscreen", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
    if (isFullscreen == enable) return;

    static int x, y, w, h;
    if (enable) {
      glfwGetWindowPos(window, &x, &y);
      glfwGetWindowSize(window, &w, &h);
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else
      glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
  });

  mpv->observeProperty("ontop", MPV_FORMAT_FLAG, [=, this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    glfwSetWindowAttrib(window, GLFW_FLOATING, enable ? GLFW_TRUE : GLFW_FALSE);
  });

  mpv->observeProperty("pause", MPV_FORMAT_FLAG, [=, this](void* data) { paused = static_cast<bool>(*(int*)data); });

  mpv->command("keybind MBTN_RIGHT ignore");
}

void Player::setTheme(Theme theme) {
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
}  // namespace ImPlay