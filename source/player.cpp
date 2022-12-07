#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.hpp>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/printf.h>
#include <fmt/color.h>
#include "player.h"
#include "fontawesome.h"

namespace ImPlay {
Player::Player(const char* title) {
  this->title = title;
  window = glfwGetCurrentContext();
  mpv = new Mpv();

  NFD::Init();
  setTheme(Theme::DARK);
}

Player::~Player() {
  NFD::Quit();
  delete mpv;
}

bool Player::init(int argc, char* argv[]) {
  mpv->option("config", "yes");
  mpv->option("osc", "yes");
  mpv->option("idle", "yes");
  mpv->option("force-window", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");

  optionParser.parse(argc, argv);
  for (auto& [key, value] : optionParser.options) {
    if (int err = mpv->option(key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return false;
    }
  }

  mpv->init();

  initMpv();

  for (auto& path : optionParser.paths) mpv->commandv("loadfile", path.c_str(), "append-play", nullptr);

  mpv->command("keybind MBTN_RIGHT ignore");

  return true;
}

void Player::draw() {
  if (demo) ImGui::ShowDemoWindow(&demo);
  if (about) showAbout();

  ImGuiIO& io = ImGui::GetIO();
  if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyDown(ImGuiKey_P)) showCommandPalette();
  commandPalette.draw();

  drawContextMenu();
}

void Player::render(int w, int h) { mpv->render(w, h); }

void Player::pollEvent() { mpv->pollEvent(); }

void Player::setCursor(double x, double y) {
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  mpv->commandv("mouse", xs.c_str(), ys.c_str(), nullptr);
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
  const std::string arg = fmt::format("{}", fmt::join(keys, "+"));
  mpv->commandv(cmd.c_str(), arg.c_str(), nullptr);
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
  const std::string arg = fmt::format("{}", fmt::join(keys, "+"));
  mpv->commandv(cmd.c_str(), arg.c_str(), nullptr);
}

void Player::setDrop(int count, const char** paths) {
  std::sort(paths, paths + count, [](const auto& a, const auto& b) { return strcmp(a, b) < 0; });
  for (int i = 0; i < count; i++) {
    mpv->commandv("loadfile", paths[i], i > 0 ? "append-play" : "replace", nullptr);
  }
}

void Player::showAbout() {
  if (about) ImGui::OpenPopup("About");
  ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Always);
  if (ImGui::BeginPopupModal("About", &about, ImGuiWindowFlags_NoResize)) {
    ImGui::Text("ImPlay");
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::TextWrapped("ImPlay is a cross-platform desktop media player.");
    ImGui::EndPopup();
  }
}

void Player::showCommandPalette() {
  commandPalette.show(bindinglist, [=, this](std::string cmd) { mpv->command(cmd.c_str()); });
}

void Player::CommandPalette::show(std::vector<Mpv::BindingItem>& bindinglist, Player::CommandHandler callback) {
  this->bindinglist = bindinglist;
  this->callback = callback;
  open = true;
}

void Player::CommandPalette::draw() {
  if (open) {
    ImGui::OpenPopup("##command_palette");
    open = false;
    justOpened = true;
  }

  auto viewport = ImGui::GetMainViewport();
  auto pos = viewport->Pos;
  auto size = viewport->Size;
  auto width = size.x * 0.5f;
  auto height = size.y * 0.45f;
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(pos.x + size.x * 0.5f, pos.y + 50.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
  if (ImGui::BeginPopup("##command_palette")) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
    if (focusInput) {
      auto textState = ImGui::GetInputTextState(ImGui::GetID("##command_input"));
      if (textState != nullptr) textState->Stb.cursor = strlen(buffer.data());
      ImGui::SetKeyboardFocusHere(0);
      focusInput = false;
    }

    ImGui::PushItemWidth(-1);
    ImGui::InputTextWithHint(
        "##command_input", "TIP: Press Space to select result", buffer.data(), buffer.size(),
        ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue,
        [](ImGuiInputTextCallbackData* data) -> int {
          auto cp = static_cast<Player::CommandPalette*>(data->UserData);
          cp->match(data->Buf);
          return 0;
        },
        this);
    ImGui::PopItemWidth();

    if (justOpened) {
      focusInput = true;
      match("");
      std::memset(buffer.data(), 0x00, buffer.size());
      justOpened = false;
    }

    ImGui::Separator();

    ImGui::BeginChild("##command_matches");
    auto leftWidth = width * 0.75f;
    auto rightWidth = width * 0.25f;
    if (rightWidth < 200.0f) rightWidth = 200.0f;
    for (const auto& match : matches) {
      std::string title = match.comment;
      if (title.empty()) title = match.command;
      ImGui::SetNextItemWidth(leftWidth);
      int charLimit = leftWidth / ImGui::GetFontSize();
      title = title.substr(0, charLimit);
      if (title.size() == charLimit) title += "...";

      ImGui::PushID(&match);
      if (ImGui::Selectable("")) callback(match.command);
      ImGui::SameLine();
      ImGui::Text("%s", title.c_str());
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", match.command.c_str());
      ImGui::SameLine(ImGui::GetWindowWidth() - rightWidth);
      ImGui::BeginDisabled();
      ImGui::Button(match.key.c_str());
      ImGui::EndDisabled();
      ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

void Player::CommandPalette::match(const std::string& input) {
  constexpr static auto MatchCommand = [](const std::string& input, const std::string& text) -> int {
    if (input.empty()) return 1;
    if (text.starts_with(input)) return 3;
    if (text.find(input) != std::string::npos) return 2;
    return 0;
  };

  matches.clear();
  for (auto& binding : bindinglist) {
    std::string key = binding.key ? binding.key : "";
    std::string command = binding.cmd ? binding.cmd : "";
    std::string comment = binding.comment ? binding.comment : "";
    if (command.empty() || command == "ignore") continue;
    int score = MatchCommand(input, comment) * 2;
    if (score == 0) score = MatchCommand(input, command);
    if (score > 0) matches.push_back({key, command, comment, score});
  }
  if (input.empty()) return;

  std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
    if (a.score != b.score) return a.score > b.score;
    if (a.comment != b.comment) return a.comment < b.comment;
    return a.command < b.command;
  });
}

void Player::drawTracklistMenu(const char* type, const char* prop) {
  if (ImGui::BeginMenuEx("Tracks", ICON_FA_LIST, !tracklist.empty())) {
    for (auto& track : tracklist) {
      if (strcmp(track.type, type) == 0) {
        std::string title;
        if (track.title == nullptr)
          title = fmt::format("Track {}", track.id);
        else
          title = track.title;
        if (track.lang != nullptr) title += fmt::format(" [{}]", track.lang);
        if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, track.selected))
          mpv->property<int64_t, MPV_FORMAT_INT64>(prop, track.id);
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
      std::string title = fmt::format("{} [{:%H:%M:%S}]", chapter.title, std::chrono::duration<int>((int)chapter.time));
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, chapter.selected)) {
        mpv->commandv("seek", std::to_string(chapter.time).c_str(), "absolute", nullptr);
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
    if (ImGui::MenuItem("Clear")) mpv->command("playlist-clear");
    if (ImGui::MenuItem("Shuffle")) mpv->command("playlist-shuffle");
    if (ImGui::MenuItem("Infinite Loop", "L")) mpv->command("cycle-values loop-file inf no");
    ImGui::Separator();
    for (auto& item : playlist) {
      std::string title;
      if (item.title != nullptr)
        title = item.title;
      else if (item.filename != nullptr)
        title = item.filename;
      else
        title = fmt::format("Item {}", item.id);
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, item.current || item.playing))
        mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos-1", item.id);
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
    if (ImGui::MenuItemEx("Show Progress", ICON_FA_SPINNER, "o")) mpv->command("show-progress");
    ImGui::Separator();
    drawChapterlistMenu();
    drawPlaylistMenu();
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Audio", ICON_FA_FILE_AUDIO)) {
      drawTracklistMenu("audio", "aid");
      if (ImGui::MenuItemEx("Volume +2", ICON_FA_VOLUME_UP, "0")) mpv->command("add volume 2");
      if (ImGui::MenuItemEx("Volume -2", ICON_FA_VOLUME_DOWN, "9")) mpv->command("add volume -2");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Mute", ICON_FA_VOLUME_MUTE, "m")) mpv->command("cycle mute");
      ImGui::Separator();
      if (ImGui::MenuItem("Delay +0.1", "Ctrl +")) mpv->command("add audio-delay 0.1");
      if (ImGui::MenuItem("Delay -0.1", "Ctrl -")) mpv->command("add audio-delay -0.1");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Video", ICON_FA_VIDEO)) {
      drawTracklistMenu("video", "vid");
      if (ImGui::BeginMenuEx("Rotate", ICON_FA_SPINNER)) {
        if (ImGui::MenuItem("90°")) mpv->command("set video-rotate 90");
        if (ImGui::MenuItem("180°")) mpv->command("set video-rotate 180");
        if (ImGui::MenuItem("270°")) mpv->command("set video-rotate 270");
        if (ImGui::MenuItem("Original")) mpv->command("set video-rotate 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenuEx("Zoom", ICON_FA_GLASSES)) {
        if (ImGui::MenuItemEx("Zoom In", ICON_FA_MINUS_CIRCLE, "Alt +")) mpv->command("add video-zoom  -0.1");
        if (ImGui::MenuItemEx("Zoom Out", ICON_FA_PLUS_CIRCLE, "Alt -")) mpv->command("add video-zoom  0.1");
        if (ImGui::MenuItem("Rest", "Alt+BS")) mpv->command("set video-zoom 0");
        ImGui::Separator();
        if (ImGui::MenuItem("1:4 Quarter")) mpv->command("set video-zoom -2");
        if (ImGui::MenuItem("1:2 Half")) mpv->command("set video-zoom -1");
        if (ImGui::MenuItem("1:1 Original")) mpv->command("set video-zoom 0");
        if (ImGui::MenuItem("2:1 Double")) mpv->command("set video-zoom 1");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Move")) {
        if (ImGui::MenuItem("Right", "Alt+left")) mpv->command("add video-pan-x 0.1");
        if (ImGui::MenuItem("Left", "Alt+right")) mpv->command("add video-pan-x -0.1");
        if (ImGui::MenuItem("Down", "Alt+up")) mpv->command("add video-pan-y 0.1");
        if (ImGui::MenuItem("Up", "Alt+down")) mpv->command("add video-pan-y -0.1");
        if (ImGui::MenuItem("Rest", "Alt+BS")) mpv->command("set video-pan-x 0 ; set video-pan-y 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Panscan")) {
        if (ImGui::MenuItem("+0.1", "W")) mpv->command("add panscan 0.1");
        if (ImGui::MenuItem("-0.1", "w")) mpv->command("add panscan -0.1");
        if (ImGui::MenuItem("Reset")) mpv->command("set panscan 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Speed")) {
        if (ImGui::MenuItem("2x")) mpv->command("multiply speed 2.0");
        if (ImGui::MenuItem("1.5x")) mpv->command("multiply speed 1.5");
        if (ImGui::MenuItem("1.25x")) mpv->command("multiply speed 1.25");
        if (ImGui::MenuItem("1.0x")) mpv->command("set speed 1");
        if (ImGui::MenuItem("0.75x")) mpv->command("multiply speed 0.75");
        if (ImGui::MenuItem("0.5x")) mpv->command("multiply speed 0.5");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Aspect")) {
        if (ImGui::MenuItem("16:9")) mpv->command("set video-aspect 16:9");
        if (ImGui::MenuItem("4:3")) mpv->command("set video-aspect 4:3");
        if (ImGui::MenuItem("2.35:1")) mpv->command("set video-aspect 2.35:1");
        if (ImGui::MenuItem("1:1")) mpv->command("set video-aspect 1:1");
        if (ImGui::MenuItem("Original")) mpv->command("set video-aspect -1");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Effect")) {
        if (ImGui::MenuItem("Contrast +1", "2")) mpv->command("add contrast 1");
        if (ImGui::MenuItem("Contrast -1", "1")) mpv->command("add contrast -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Brightness +1", "4")) mpv->command("add brightness 1");
        if (ImGui::MenuItem("Brightness -1", "3")) mpv->command("add brightness -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Gamma +1", "6")) mpv->command("add gamma 1");
        if (ImGui::MenuItem("Gamma -1", "5")) mpv->command("add gamma -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Saturation +1", "8")) mpv->command("add saturation 1");
        if (ImGui::MenuItem("Saturation -1", "7")) mpv->command("add saturation -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Hue +1")) mpv->command("add hue 1");
        if (ImGui::MenuItem("Hue -1")) mpv->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("HW Decoding", "Ctrl h")) mpv->command("cycle-values hwdec auto no");
      if (ImGui::MenuItem("Toggle A-B Loop", "l")) mpv->command("ab-loop");
      if (ImGui::MenuItem("Toggle Deinterlace", "d")) mpv->command("cycle deinterlace");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Subtitle", ICON_FA_FONT)) {
      drawTracklistMenu("sub", "sid");
      if (ImGui::MenuItemEx("Load..", ICON_FA_FOLDER_OPEN)) loadSub();
      if (ImGui::MenuItem("Show/Hide", "v")) mpv->command("cycle sub-visibility");
      ImGui::Separator();
      if (ImGui::MenuItem("Move Up", "r")) mpv->command("add sub-pos -1");
      if (ImGui::MenuItem("Move Down", "R")) mpv->command("add sub-pos +1");
      ImGui::Separator();
      if (ImGui::MenuItem("Delay +0.1", "z")) mpv->command("add sub-delay 0.1");
      if (ImGui::MenuItem("Delay -0.1", "Z")) mpv->command("add sub-delay -0.1");
      ImGui::Separator();
      if (ImGui::MenuItem("Scale +0.1", "F")) mpv->command("add sub-scale 0.1");
      if (ImGui::MenuItem("Scale -0.1", "G")) mpv->command("add sub-scale -0.1");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("Fullscreen", ICON_FA_EXPAND, "f")) mpv->command("cycle fullscreen");
    if (ImGui::MenuItemEx("Always Ontop", ICON_FA_ARROW_UP, "T")) mpv->command("cycle ontop");
    if (ImGui::MenuItemEx("Command Palette", ICON_FA_SEARCH, "Ctrl+Shift+p")) showCommandPalette();
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Tools", ICON_FA_HAMMER)) {
      if (ImGui::MenuItemEx("Screenshot", ICON_FA_FILE_IMAGE, "s")) mpv->command("async screenshot");
      if (ImGui::MenuItemEx("Window Border", ICON_FA_BORDER_NONE)) mpv->command("cycle border");
      if (ImGui::MenuItemEx("Media Info", ICON_FA_INFO_CIRCLE, "i"))
        mpv->command("script-binding stats/display-page-1");
      if (ImGui::MenuItem("Show/Hide OSC", "DEL")) mpv->command("script-binding osc/visibility");
      if (ImGui::MenuItem("Script Console", "`")) mpv->command("script-binding console/enable");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Profiles", ICON_FA_USER)) {
      for (auto& profile : profilelist) {
        if (ImGui::MenuItem(profile.c_str()))
          mpv->command(fmt::format("show-text {}; apply-profile {}", profile, profile).c_str());
      }
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
      mpv->commandv("loadfile", path, i > 0 ? "append-play" : "replace", nullptr);
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
      mpv->commandv("sub-add", path, i > 0 ? "auto" : "select", nullptr);
      NFD::PathSet::FreePath(path);
    }
    NFD::PathSet::Free(outPaths);
  }
}

void Player::initMpv() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [=, this](void* data) { glfwSetWindowShouldClose(window, true); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [=, this](void* data) {
    if (!loaded) return;
    auto w = mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    auto h = mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (w > 0 && h > 0) {
      glfwSetWindowSize(window, w, h);
      glfwSetWindowAspectRatio(window, w, h);
    }
  });

  mpv->observeEvent(MPV_EVENT_START_FILE, [=, this](void* data) { loaded = true; });

  mpv->observeEvent(MPV_EVENT_END_FILE, [=, this](void* data) {
    loaded = false;
    glfwSetWindowTitle(window, title);
    glfwSetWindowAspectRatio(window, GLFW_DONT_CARE, GLFW_DONT_CARE);
  });

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

  mpv->observeProperty("chapter-list", MPV_FORMAT_NODE, [=, this](void* data) {
    mpv_node* node = static_cast<mpv_node*>(data);
    chapterlist = mpv->toChapterlist(node);
  });

  mpv->observeProperty("chapter", MPV_FORMAT_INT64, [=, this](void* data) {
    int64_t ImDrawIdx = static_cast<int64_t>(*(int64_t*)data);
    for (auto& chapter : chapterlist) chapter.selected = chapter.id == ImDrawIdx;
  });

  mpv->observeProperty("input-bindings", MPV_FORMAT_NODE, [=, this](void* data) {
    mpv_node* node = static_cast<mpv_node*>(data);
    bindinglist = mpv->toBindinglist(node);
  });

  mpv->observeProperty("profile-list", MPV_FORMAT_STRING, [=, this](void* data) {
    char* payload = static_cast<char*>(*(char**)data);
    profilelist = mpv->toProfilelist(payload);
  });
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