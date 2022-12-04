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

void Player::drawTracklistMenu(const char* type, const char* prop) {
  if (tracklist.empty()) return;
  if (ImGui::BeginMenuEx("Tracks", ICON_FA_LIST)) {
    for (auto& track : tracklist) {
      if (strcmp(track.type, type) == 0) {
        std::string title;
        if (track.title == nullptr)
          title = "Track " + std::to_string(track.id);
        else
          title = track.title;
        if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, track.selected))
          mpv->property(prop, MPV_FORMAT_INT64, track.id);
      }
    }
    ImGui::EndMenu();
  }
}

void Player::drawPlaylistMenu() {
  if (playlist.empty()) return;
  ImGui::Separator();
  if (ImGui::BeginMenuEx("Playlist", ICON_FA_LIST)) {
    for (auto& item : playlist) {
      std::string title;
      if (item.title != nullptr)
        title = item.title;
      else if (item.filename != nullptr)
        title = item.filename;
      else
        title = "Item " + std::to_string(item.id);
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, item.current || item.playing))
        mpv->property("playlist-pos-1", MPV_FORMAT_INT64, item.id);
    }
    ImGui::EndMenu();
  }
}

void Player::drawContextMenu() {
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    ImGui::OpenPopup("Context Menu");

  if (ImGui::BeginPopup("Context Menu", ImGuiWindowFlags_NoMove)) {
    if (ImGui::MenuItemEx(paused ? "Play" : "Pause", paused ? ICON_FA_PLAY : ICON_FA_PAUSE, "Space", false, loaded))
      mpv->command("cycle pause");
    if (ImGui::MenuItemEx("Stop", ICON_FA_STOP, nullptr, false, loaded)) mpv->command("stop");
    ImGui::Separator();
    if (ImGui::MenuItemEx("Jump Forward", ICON_FA_FORWARD, "UP", false, loaded)) mpv->command("seek 10");
    if (ImGui::MenuItemEx("Jump Backward", ICON_FA_BACKWARD, "DOWN", false, loaded)) mpv->command("seek -10");
    ImGui::Separator();
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT, "<")) mpv->command("playlist-prev");
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT, ">")) mpv->command("playlist-next");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Audio", ICON_FA_FILE_AUDIO)) {
      drawTracklistMenu("audio", "aid");
      if (ImGui::MenuItemEx("Increase Volume", ICON_FA_VOLUME_UP, "0")) mpv->command("add volume 2");
      if (ImGui::MenuItemEx("Decrease Volume", ICON_FA_VOLUME_DOWN, "9")) mpv->command("add volume -2");
      if (ImGui::MenuItemEx("Mute", ICON_FA_VOLUME_MUTE, "m")) mpv->command("cycle mute");
      if (ImGui::MenuItem("Increase Delay", "Ctrl +")) mpv->command("add audio-delay 0.1");
      if (ImGui::MenuItem("Decrease Delay", "Ctrl -")) mpv->command("add audio-delay -0.1");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Video", ICON_FA_VIDEO)) {
      drawTracklistMenu("video", "vid");
      if (ImGui::MenuItemEx("Rotate", ICON_FA_SPINNER)) mpv->command("cycle-values video-rotate 0 90 180 270");
      if (ImGui::MenuItemEx("Zoom In", ICON_FA_MINUS_CIRCLE, "Alt +")) mpv->command("add video-zoom  -0.1");
      if (ImGui::MenuItemEx("Zoom Out", ICON_FA_PLUS_CIRCLE, "Alt -")) mpv->command("add video-zoom  0.1");
      if (ImGui::MenuItem("HW Decoding", "Ctrl h")) mpv->command("cycle-values hwdec auto no");
      if (ImGui::BeginMenu("Effect")) {
        if (ImGui::MenuItem("Increase Contrast", "2")) mpv->command("add contrast 1");
        if (ImGui::MenuItem("Decrease Contrast", "1")) mpv->command("add contrast -1");
        if (ImGui::MenuItem("Increase Brightness", "4")) mpv->command("add brightness 1");
        if (ImGui::MenuItem("Decrease Brightness", "3")) mpv->command("add brightness -1");
        if (ImGui::MenuItem("Increase Gamma", "6")) mpv->command("add gamma 1");
        if (ImGui::MenuItem("Decrease Gamma", "5")) mpv->command("add gamma -1");
        if (ImGui::MenuItem("Increase Saturation", "8")) mpv->command("add saturation 1");
        if (ImGui::MenuItem("Decrease Saturation", "7")) mpv->command("add saturation -1");
        if (ImGui::MenuItem("Increase Hue")) mpv->command("add hue 1");
        if (ImGui::MenuItem("Decrease Hue")) mpv->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Subtitle", ICON_FA_FONT)) {
      drawTracklistMenu("sub", "sid");
      if (ImGui::MenuItemEx("Load..", ICON_FA_FOLDER_OPEN)) loadSub();
      if (ImGui::MenuItem("Increase Delay", "z")) mpv->command("add sub-delay 0.1");
      if (ImGui::MenuItem("Decrease Delay", "Z")) mpv->command("add sub-delay -0.1");
      ImGui::EndMenu();
    }
    drawPlaylistMenu();
    ImGui::Separator();
    if (ImGui::MenuItemEx("Fullscreen", ICON_FA_EXPAND, "f")) mpv->command("cycle fullscreen");
    if (ImGui::MenuItemEx("Always Ontop", ICON_FA_ARROW_UP, "T")) mpv->command("cycle ontop");
    if (ImGui::MenuItemEx("Show Progress", ICON_FA_SPINNER, "o")) mpv->command("show-progress");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Tools", ICON_FA_HAMMER)) {
      if (ImGui::MenuItemEx("Screenshot", ICON_FA_FILE_IMAGE, "s")) mpv->command("screenshot");
      if (ImGui::MenuItemEx("Window Border", ICON_FA_BORDER_NONE)) mpv->command("cycle border");
      if (ImGui::MenuItemEx("Media Info", ICON_FA_INFO_CIRCLE, "i")) mpv->command("script-binding stats/display-stats");
      if (ImGui::MenuItemEx("Show Keybindings", ICON_FA_LINK)) mpv->command("script-binding stats/display-page-4");
      if (ImGui::MenuItem("OSC visibility", "DEL")) mpv->command("script-binding osc/visibility");
      if (ImGui::MenuItem("Script Console", "`")) mpv->command("script-binding console/enable");
      ImGui::MenuItem("ImGui Demo", nullptr, &demo);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Theme", ICON_FA_PALETTE)) {
      if (ImGui::MenuItem("Dark", nullptr, theme == Theme::DARK)) setTheme(Theme::DARK);
      if (ImGui::MenuItem("Light", nullptr, theme == Theme::LIGHT)) setTheme(Theme::LIGHT);
      if (ImGui::MenuItem("Classic", nullptr, theme == Theme::CLASSIC)) setTheme(Theme::CLASSIC);
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("Open..", ICON_FA_FOLDER_OPEN)) openFile();
    if (ImGui::MenuItemEx("Quit", ICON_FA_WINDOW_CLOSE, "q")) mpv->command("quit");
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
    auto w = mpv->property<int64_t>("dwidth", MPV_FORMAT_INT64);
    auto h = mpv->property<int64_t>("dheight", MPV_FORMAT_INT64);
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

  mpv->observeProperty("track-list", MPV_FORMAT_NODE, [=, this](void* data) {
    mpv_node* node = static_cast<mpv_node*>(data);
    tracklist = mpv->toTracklist(node);
  });

  mpv->observeProperty("playlist", MPV_FORMAT_NODE, [=, this](void* data) {
    mpv_node* node = static_cast<mpv_node*>(data);
    playlist = mpv->toPlaylist(node);
  });

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