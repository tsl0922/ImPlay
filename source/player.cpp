#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.hpp>
#include <algorithm>
#include <cstring>
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
  if (about) showAbout();
  if (ImGui::IsKeyDown(ImGuiKey_P) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
    commandPalette.open = true;
    commandPalette.justOpened = true;
    ImGui::OpenPopup("Command Palette");
  }
  if (commandPalette.open) showCommandPalette();
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

void Player::showAbout() {
  if (about) ImGui::OpenPopup("About");
  ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Always);
  if (ImGui::BeginPopupModal("About", &about, ImGuiWindowFlags_NoResize)) {
    ImGui::Text("ImPlay");
    ImGui::Separator();
    ImGui::TextWrapped("\nImPlay is a cross-platform desktop media player.");
    ImGui::EndPopup();
  }
}

void Player::showCommandPalette() {
  auto viewport = ImGui::GetMainViewport();
  auto pos = viewport->Pos;
  auto size = viewport->Size;
  ImGui::SetNextWindowSize(ImVec2(650, 0.0f), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(pos.x + size.x * 0.5F, pos.y), ImGuiCond_Always, ImVec2(0.5F, 0.0F));
  if (ImGui::BeginPopup("Command Palette")) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

    if (commandPalette.justOpened) {
      auto textState = ImGui::GetInputTextState(ImGui::GetID("##command_input"));
      if (textState != nullptr) {
        textState->Stb.cursor = strlen(commandPalette.buffer.data());
      }
      ImGui::SetKeyboardFocusHere(0);
      commandPalette.focusInput = false;
    }

    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint(
            "##command_input", "TIP: Press Space to select result", commandPalette.buffer.data(),
            commandPalette.buffer.size(), ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue,
            [](ImGuiInputTextCallbackData* data) -> int {
              auto p = static_cast<Player*>(data->UserData);
              p->commandPalette.matches = p->getCommandMatches(data->Buf);
              return 0;
            },
            this)) {
      if (!commandPalette.matches.empty()) {
        auto& [title, command] = commandPalette.matches.front();
        mpv->command(command.c_str());
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::PopItemWidth();

    if (commandPalette.justOpened) {
      commandPalette.focusInput = true;
      commandPalette.matches = getCommandMatches("");
      std::memset(commandPalette.buffer.data(), 0x00, commandPalette.buffer.size());
      commandPalette.justOpened = false;
    }

    ImGui::Separator();

    for (const auto& [title, command] : commandPalette.matches) {
      if (ImGui::Selectable(title.c_str(), false, ImGuiSelectableFlags_DontClosePopups)) mpv->command(command.c_str());
    }
    ImGui::EndPopup();
  } else {
    commandPalette.open = false;
  }
}

std::vector<Player::CommandMatch> Player::getCommandMatches(const std::string& command) {
  std::vector<Player::CommandMatch> matches;
  matches.push_back({"Pause", "cycle pause"});
  if (command.empty()) {
    matches.push_back({"Stop", "stop"});
    matches.push_back({"Quit", "quit"});
  }
  return matches;
}

void Player::drawTracklistMenu(const char* type, const char* prop) {
  if (ImGui::BeginMenuEx("Tracks", ICON_FA_LIST, !tracklist.empty())) {
    for (auto& track : tracklist) {
      if (strcmp(track.type, type) == 0) {
        std::string title;
        if (track.title == nullptr)
          title = "Track " + std::to_string(track.id);
        else
          title = track.title;
        if (track.lang != nullptr) title += " [" + std::string(track.lang) + "]";
        if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, track.selected))
          mpv->property(prop, MPV_FORMAT_INT64, track.id);
      }
    }
    ImGui::EndMenu();
  }
}

void Player::drawChapterlistMenu() {
  if (ImGui::BeginMenuEx("Chapters", ICON_FA_LIST, !chapterlist.empty())) {
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT)) mpv->command("add chapter -1");
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT)) mpv->command("add chapter 1");
    ImGui::Separator();
    for (auto& chapter : chapterlist) {
      std::string title = chapter.title;
      title += " [" + std::to_string(chapter.time) + "]";
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, chapter.selected)) {
        const char* args[] = {"seek", std::to_string(chapter.time).c_str(), "absolute", NULL};
        mpv->command(args);
      }
    }
    ImGui::EndMenu();
  }
}

void Player::drawPlaylistMenu() {
  if (ImGui::BeginMenuEx("Playlist", ICON_FA_LIST, !playlist.empty())) {
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT, "<", false, playlist.size() > 1))
      mpv->command("playlist-prev");
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT, ">", false, playlist.size() > 1)) mpv->command("playlist-next");
    ImGui::Separator();
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
    drawChapterlistMenu();
    drawPlaylistMenu();
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Audio", ICON_FA_FILE_AUDIO)) {
      drawTracklistMenu("audio", "aid");
      if (ImGui::MenuItemEx("Increase Volume", ICON_FA_VOLUME_UP, "0")) mpv->command("add volume 2");
      if (ImGui::MenuItemEx("Decrease Volume", ICON_FA_VOLUME_DOWN, "9")) mpv->command("add volume -2");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Mute", ICON_FA_VOLUME_MUTE, "m")) mpv->command("cycle mute");
      ImGui::Separator();
      if (ImGui::MenuItem("Increase Delay", "Ctrl +")) mpv->command("add audio-delay 0.1");
      if (ImGui::MenuItem("Decrease Delay", "Ctrl -")) mpv->command("add audio-delay -0.1");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Video", ICON_FA_VIDEO)) {
      drawTracklistMenu("video", "vid");
      if (ImGui::BeginMenuEx("Rotate", ICON_FA_SPINNER)) {
        if (ImGui::MenuItem("0")) mpv->command("set video-rotate 0");
        if (ImGui::MenuItem("90")) mpv->command("set video-rotate 90");
        if (ImGui::MenuItem("180")) mpv->command("set video-rotate 180");
        if (ImGui::MenuItem("270")) mpv->command("set video-rotate 270");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenuEx("Zoom", ICON_FA_GLASSES)) {
        if (ImGui::MenuItemEx("Zoom In", ICON_FA_MINUS_CIRCLE, "Alt +")) mpv->command("add video-zoom  -0.1");
        if (ImGui::MenuItemEx("Zoom Out", ICON_FA_PLUS_CIRCLE, "Alt -")) mpv->command("add video-zoom  0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("1:4 Quarter")) mpv->command("set video-zoom -2");
        if (ImGui::MenuItem("1:2 Half")) mpv->command("set video-zoom -1");
        if (ImGui::MenuItem("1:1 Original")) mpv->command("set video-zoom 0");
        if (ImGui::MenuItem("2:1 Double")) mpv->command("set video-zoom 1");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Aspect")) {
        if (ImGui::MenuItem("16:9")) mpv->command("set video-aspect 16:9");
        if (ImGui::MenuItem("4:3")) mpv->command("set video-aspect 4:3");
        if (ImGui::MenuItem("1:1")) mpv->command("set video-aspect 1:1");
        if (ImGui::MenuItem("Disable")) mpv->command("set video-aspect no");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Effect")) {
        if (ImGui::MenuItem("Increase Contrast", "2")) mpv->command("add contrast 1");
        if (ImGui::MenuItem("Decrease Contrast", "1")) mpv->command("add contrast -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Increase Brightness", "4")) mpv->command("add brightness 1");
        if (ImGui::MenuItem("Decrease Brightness", "3")) mpv->command("add brightness -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Increase Gamma", "6")) mpv->command("add gamma 1");
        if (ImGui::MenuItem("Decrease Gamma", "5")) mpv->command("add gamma -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Increase Saturation", "8")) mpv->command("add saturation 1");
        if (ImGui::MenuItem("Decrease Saturation", "7")) mpv->command("add saturation -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Increase Hue")) mpv->command("add hue 1");
        if (ImGui::MenuItem("Decrease Hue")) mpv->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("HW Decoding", "Ctrl h")) mpv->command("cycle-values hwdec auto no");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Subtitle", ICON_FA_FONT)) {
      drawTracklistMenu("sub", "sid");
      if (ImGui::MenuItemEx("Load..", ICON_FA_FOLDER_OPEN)) loadSub();
      ImGui::Separator();
      if (ImGui::MenuItem("Increase Delay", "z")) mpv->command("add sub-delay 0.1");
      if (ImGui::MenuItem("Decrease Delay", "Z")) mpv->command("add sub-delay -0.1");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("Fullscreen", ICON_FA_EXPAND, "f")) mpv->command("cycle fullscreen");
    if (ImGui::MenuItemEx("Always Ontop", ICON_FA_ARROW_UP, "T")) mpv->command("cycle ontop");
    if (ImGui::MenuItemEx("Show Progress", ICON_FA_SPINNER, "o")) mpv->command("show-progress");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Tools", ICON_FA_HAMMER)) {
      if (ImGui::MenuItemEx("Screenshot", ICON_FA_FILE_IMAGE, "s")) mpv->command("screenshot");
      if (ImGui::MenuItemEx("Window Border", ICON_FA_BORDER_NONE)) mpv->command("cycle border");
      if (ImGui::MenuItemEx("Media Info", ICON_FA_INFO_CIRCLE, "i")) mpv->command("script-binding stats/display-stats");
      if (ImGui::MenuItem("OSC visibility", "DEL")) mpv->command("script-binding osc/visibility");
      if (ImGui::MenuItem("Script Console", "`")) mpv->command("script-binding console/enable");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Theme", ICON_FA_PALETTE)) {
      if (ImGui::MenuItem("Dark", nullptr, theme == Theme::DARK)) setTheme(Theme::DARK);
      if (ImGui::MenuItem("Light", nullptr, theme == Theme::LIGHT)) setTheme(Theme::LIGHT);
      if (ImGui::MenuItem("Classic", nullptr, theme == Theme::CLASSIC)) setTheme(Theme::CLASSIC);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Help", ICON_FA_QUESTION_CIRCLE)) {
      if (ImGui::MenuItemEx("About", ICON_FA_INFO_CIRCLE)) about = true;
      if (ImGui::MenuItem("Keybindings")) mpv->command("script-binding stats/display-page-4");
      ImGui::MenuItem("ImGui Demo", nullptr, &demo);
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("Open..", ICON_FA_FOLDER_OPEN)) openFile();
    if (ImGui::MenuItemEx("Quit", ICON_FA_WINDOW_CLOSE, "q")) mpv->command("quit");
    ImGui::EndPopup();
  }
}

void Player::openFile() {
  const nfdpathset_t* outPaths;
  nfdfilteritem_t filterItem[] = {
      {"Videos Files", "mkv,mp4,avi,mov,flv,mpg,webm,wmv,ts,vob,264,265,asf,avc,avs,dav,h264,h265,hevc,m2t,m2ts"},
      {"Audio Files", "mp3,flac,m4a,mka,mp2,ogg,opus,aac,ac3,dts,dtshd,dtshr,dtsma,eac3,mpa,mpc,thd,w64"},
      {"Image Files", "jpg,bmp,png,gif,webp"},
  };
  if (NFD::OpenDialogMultiple(outPaths, filterItem, 3) == NFD_OKAY) {
    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);
    for (auto i = 0; i < numPaths; i++) {
      nfdchar_t* path;
      NFD::PathSet::GetPath(outPaths, i, path);
      const char* cmd[] = {"loadfile", path, i > 0 ? "append-play" : "replace", NULL};
      mpv->command(cmd);
      NFD::PathSet::FreePath(path);
    }
    NFD::PathSet::Free(outPaths);
  }
}

void Player::loadSub() {
  const nfdpathset_t* outPaths;
  nfdfilteritem_t filterItem[] = {
      {"Subtitle Files", "srt,ass,idx,sub,sup,ttxt,txt,ssa,smi,mks"},
  };
  if (NFD::OpenDialogMultiple(outPaths, filterItem, 1) == NFD_OKAY) {
    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);
    for (auto i = 0; i < numPaths; i++) {
      nfdchar_t* path;
      NFD::PathSet::GetPath(outPaths, i, path);
      const char* cmd[] = {"sub-add", path, i > 0 ? "auto" : "select", NULL};
      mpv->command(cmd);
      NFD::PathSet::FreePath(path);
    }
    NFD::PathSet::Free(outPaths);
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
    std::sort(playlist.begin(), playlist.end(),
              [](const auto& a, const auto& b) { return strcmp(a.filename, b.filename) < 0; });
  });

  mpv->observeProperty("chapter-list", MPV_FORMAT_NODE, [=, this](void* data) {
    mpv_node* node = static_cast<mpv_node*>(data);
    chapterlist = mpv->toChapterlist(node);
  });

  mpv->observeProperty("chapter", MPV_FORMAT_INT64, [=, this](void* data) {
    int64_t ImDrawIdx = static_cast<int64_t>(*(int64_t*)data);
    for (auto& chapter : chapterlist) chapter.selected = chapter.id == ImDrawIdx;
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