#include <imgui.h>
#include <imgui_internal.h>
#include <fonts/fontawesome.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <filesystem>
#include "views/context_menu.h"

namespace ImPlay::Views {
ContextMenu::ContextMenu(Mpv *mpv) {
  this->mpv = mpv;
  setTheme(Theme::DARK);
}

ContextMenu::~ContextMenu() {}

void ContextMenu::draw() {
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    ImGui::OpenPopup("##context_menu");

  bool paused = (bool)mpv->property<int, MPV_FORMAT_FLAG>("pause");
  bool playing = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-playing-pos") != -1;
  if (ImGui::BeginPopup("##context_menu", ImGuiWindowFlags_NoMove)) {
    if (ImGui::MenuItemEx(paused ? "Play" : "Pause", paused ? ICON_FA_PLAY : ICON_FA_PAUSE, "Space", false, playing))
      mpv->command("cycle pause");
    if (ImGui::MenuItemEx("Stop", ICON_FA_STOP, nullptr, false, playing)) mpv->command("stop");
    ImGui::Separator();
    if (ImGui::MenuItemEx("Jump Forward", ICON_FA_FORWARD, "UP", false, playing)) mpv->command("seek 10");
    if (ImGui::MenuItemEx("Jump Backward", ICON_FA_BACKWARD, "DOWN", false, playing)) mpv->command("seek -10");
    if (ImGui::MenuItemEx("Show Progress", ICON_FA_SPINNER, "o", false, playing)) mpv->command("show-progress");
    ImGui::Separator();
    drawChapterlist();
    drawPlaylist();
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Audio", ICON_FA_FILE_AUDIO)) {
      drawTracklist("audio", "aid");
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
      drawTracklist("video", "vid");
      if (ImGui::BeginMenuEx("Rotate", ICON_FA_SPINNER)) {
        if (ImGui::MenuItem("90°")) mpv->command("set video-rotate 90");
        if (ImGui::MenuItem("180°")) mpv->command("set video-rotate 180");
        if (ImGui::MenuItem("270°")) mpv->command("set video-rotate 270");
        if (ImGui::MenuItem("Reset")) mpv->command("set video-rotate 0");
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
        if (ImGui::MenuItem("Reset")) mpv->command("set video-aspect -1");
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
      if (ImGui::MenuItem("Toggle A-B Loop", "l", false, playing)) mpv->command("ab-loop");
      if (ImGui::MenuItem("Toggle Deinterlace", "d")) mpv->command("cycle deinterlace");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Subtitle", ICON_FA_FONT)) {
      drawTracklist("sub", "sid");
      if (ImGui::MenuItemEx("Load..", ICON_FA_FOLDER_OPEN)) action(Action::OPEN_SUB);
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
    if (ImGui::MenuItemEx("Command Palette", ICON_FA_SEARCH, "Ctrl+Shift+p")) action(Action::PALETTE);
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
    drawProfilelist();
    if (ImGui::BeginMenuEx("Theme", ICON_FA_PALETTE)) {
      if (ImGui::MenuItem("Dark", nullptr, theme == Theme::DARK)) setTheme(Theme::DARK);
      if (ImGui::MenuItem("Light", nullptr, theme == Theme::LIGHT)) setTheme(Theme::LIGHT);
      if (ImGui::MenuItem("Classic", nullptr, theme == Theme::CLASSIC)) setTheme(Theme::CLASSIC);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Help", ICON_FA_QUESTION_CIRCLE)) {
      if (ImGui::MenuItemEx("About", ICON_FA_INFO_CIRCLE)) action(Action::ABOUT);
      if (ImGui::MenuItem("Keybindings")) mpv->command("script-binding stats/display-page-4");
      ImGui::MenuItem("ImGui Demo", nullptr, &demo);
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("Open..", ICON_FA_FOLDER_OPEN)) action(Action::OPEN_FILE);
    if (ImGui::MenuItemEx("Quit", ICON_FA_WINDOW_CLOSE, "q")) mpv->command("quit");
    ImGui::EndPopup();
  }
  if (demo) ImGui::ShowDemoWindow(&demo);
}

void ContextMenu::drawTracklist(const char *type, const char *prop) {
  auto tracklist = mpv->tracklist(type);

  std::vector<Mpv::TrackItem> list;
  std::copy_if(tracklist.begin(), tracklist.end(), std::back_inserter(list),
               [type](const auto &track) { return strcmp(track.type, type) == 0; });
  if (ImGui::BeginMenuEx("Tracks", ICON_FA_LIST, !list.empty())) {
    for (auto &track : list) {
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

void ContextMenu::drawChapterlist() {
  auto chapterlist = mpv->chapterlist();
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("chapter");

  if (ImGui::BeginMenuEx("Chapters", ICON_FA_LIST, !chapterlist.empty())) {
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT)) mpv->command("add chapter -1");
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT)) mpv->command("add chapter 1");
    ImGui::Separator();
    for (auto &chapter : chapterlist) {
      std::string title = fmt::format("{} [{:%H:%M:%S}]", chapter.title, std::chrono::duration<int>((int)chapter.time));
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, chapter.id == pos)) {
        mpv->commandv("seek", std::to_string(chapter.time).c_str(), "absolute", nullptr);
      }
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawPlaylist() {
  auto playlist = mpv->playlist();
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos");

  if (ImGui::BeginMenuEx("Playlist", ICON_FA_LIST, !playlist.empty())) {
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT, "<", false, playlist.size() > 1))
      mpv->command("playlist-prev");
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT, ">", false, playlist.size() > 1)) mpv->command("playlist-next");
    ImGui::Separator();
    if (ImGui::MenuItem("Clear")) mpv->command("playlist-clear");
    if (ImGui::MenuItem("Shuffle")) mpv->command("playlist-shuffle");
    if (ImGui::MenuItem("Infinite Loop", "L")) mpv->command("cycle-values loop-file inf no");
    ImGui::Separator();
    for (auto &item : playlist) {
      std::string title;
      if (item.title != nullptr)
        title = item.title;
      else if (item.filename != nullptr)
        title = std::filesystem::path(item.filename).filename().string();
      else
        title = fmt::format("Item {}", item.id);
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, item.id == pos))
        mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", item.id);
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawProfilelist() {
  auto profilelist = mpv->profilelist();

  if (ImGui::BeginMenuEx("Profiles", ICON_FA_USER)) {
    for (auto &profile : profilelist) {
      if (ImGui::MenuItem(profile.c_str()))
        mpv->command(fmt::format("show-text {}; apply-profile {}", profile, profile).c_str());
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::action(ContextMenu::Action action) {
  auto s = actionHandlers.find(action);
  if (s != actionHandlers.end()) s->second();
}

void ContextMenu::setTheme(Theme theme) {
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
}  // namespace ImPlay::Views