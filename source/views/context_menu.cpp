// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <fonts/fontawesome.h>
#include "views/context_menu.h"
#include "helpers.h"

namespace ImPlay::Views {
ContextMenu::ContextMenu(Config *config, Mpv *mpv) : View() {
  this->config = config;
  this->mpv = mpv;
}

void ContextMenu::draw() {
  if (m_open) {
    ImGui::OpenPopup("##context_menu");
    m_open = false;
  }

  if (ImGui::BeginPopup("##context_menu", ImGuiWindowFlags_NoMove)) {
    bool paused = mpv->paused();
    bool playing = mpv->playing();
    auto playlist = mpv->playlist();
    auto chapterlist = mpv->chapterList();
    if (ImGui::GetIO().AppFocusLost || ImGui::GetWindowViewport()->Flags & ImGuiViewportFlags_Minimized)
      ImGui::CloseCurrentPopup();
    if (ImGui::MenuItemEx(paused ? "Play" : "Pause", paused ? ICON_FA_PLAY : ICON_FA_PAUSE, "Space", false, playing))
      mpv->command("cycle pause");
    if (ImGui::MenuItemEx("Stop", ICON_FA_STOP, nullptr, false, playing)) mpv->command("stop");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Playback", ICON_FA_PLAY_CIRCLE)) {
      if (ImGui::MenuItemEx("Seek +10s", ICON_FA_FORWARD, "WHEEL_UP", false, playing)) mpv->command("seek 10");
      if (ImGui::MenuItem("Seek +1m", "UP", false, playing)) mpv->command("seek 60");
      if (ImGui::MenuItemEx("Seek -10s", ICON_FA_BACKWARD, "WHEEL_DOWN", false, playing)) mpv->command("seek -10");
      if (ImGui::MenuItem("Seek -1m", "DOWN", false, playing)) mpv->command("seek -60");
      if (ImGui::BeginMenu("Speed", playing)) {
        if (ImGui::MenuItem("2x")) mpv->command("multiply speed 2.0");
        if (ImGui::MenuItem("1.5x")) mpv->command("multiply speed 1.5");
        if (ImGui::MenuItem("1.25x")) mpv->command("multiply speed 1.25");
        if (ImGui::MenuItem("1.0x")) mpv->command("set speed 1.0");
        if (ImGui::MenuItem("0.75x")) mpv->command("multiply speed 0.75");
        if (ImGui::MenuItem("0.5x")) mpv->command("multiply speed 0.5");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItemEx("Next Frame", ICON_FA_ARROW_RIGHT, ".", false, playing)) mpv->command("frame-step");
      if (ImGui::MenuItemEx("Previous Frame", ICON_FA_ARROW_LEFT, ",", false, playing)) mpv->command("frame-back-step");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Next Media", ICON_FA_ARROW_RIGHT, ">", false, playlist.size() > 1))
        mpv->command("playlist-next");
      if (ImGui::MenuItemEx("Previous Media", ICON_FA_ARROW_LEFT, "<", false, playlist.size() > 1))
        mpv->command("playlist-prev");
      if (ImGui::MenuItemEx("Playlist", ICON_FA_LIST, nullptr, false, playlist.size() > 0))
        mpv->command("script-message-to implay command-palette playlist");
      if (ImGui::MenuItem("Playlist Loop", nullptr, false, playlist.size() > 0))
        mpv->command("cycle-values loop-playlist inf no");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Next Chapter", ICON_FA_ARROW_RIGHT, nullptr, false, chapterlist.size() > 1))
        mpv->command("add chapter 1");
      if (ImGui::MenuItemEx("Previous Chapter", ICON_FA_ARROW_LEFT, nullptr, false, chapterlist.size() > 1))
        mpv->command("add chapter -1");
      if (ImGui::MenuItemEx("Chapters", ICON_FA_LIST, nullptr, false, chapterlist.size() > 0))
        mpv->command("script-message-to implay command-palette chapters");
      ImGui::Separator();
      if (ImGui::MenuItem("A-B Loop", "l", false, playing)) mpv->command("ab-loop");
      if (ImGui::MenuItem("File Loop", "L", false, playing)) mpv->command("cycle-values loop-file inf no");
      ImGui::EndMenu();
    }
    drawPlaylist(playlist);
    drawChapterlist(chapterlist);
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Audio", ICON_FA_VOLUME_UP)) {
      drawTracklist("audio", "aid");
      drawAudioDeviceList();
      ImGui::Separator();
      if (ImGui::MenuItemEx("Volume +2", ICON_FA_VOLUME_UP, "0")) mpv->command("add volume 2");
      if (ImGui::MenuItemEx("Volume -2", ICON_FA_VOLUME_DOWN, "9")) mpv->command("add volume -2");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Mute", ICON_FA_VOLUME_MUTE, "m")) mpv->command("cycle mute");
      ImGui::Separator();
      if (ImGui::MenuItem("Delay +0.1", "Ctrl +")) mpv->command("add audio-delay 0.1");
      if (ImGui::MenuItem("Delay -0.1", "Ctrl -")) mpv->command("add audio-delay -0.1");
      ImGui::Separator();
      if (ImGui::MenuItem("Control Panel")) mpv->command("script-message-to implay quickview audio");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Video", ICON_FA_VIDEO)) {
      drawTracklist("video", "vid");
      if (ImGui::BeginMenuEx("Rotate", ICON_FA_SPINNER)) {
        if (ImGui::MenuItem("90°")) mpv->command("set video-rotate 90");
        if (ImGui::MenuItem("180°")) mpv->command("set video-rotate 180");
        if (ImGui::MenuItem("270°")) mpv->command("set video-rotate 270");
        if (ImGui::MenuItem("0°")) mpv->command("set video-rotate 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenuEx("Scale", ICON_FA_GLASSES)) {
        if (ImGui::MenuItemEx("Scale In", ICON_FA_MINUS_CIRCLE)) mpv->command("add window-scale -0.1");
        if (ImGui::MenuItemEx("Scale Out", ICON_FA_PLUS_CIRCLE)) mpv->command("add window-scale 0.1");
        if (ImGui::MenuItem("Reset")) mpv->command("set window-scale 0");
        ImGui::Separator();
        if (ImGui::MenuItem("1:4 Quarter")) mpv->command("set window-scale 0.25");
        if (ImGui::MenuItem("1:2 Half")) mpv->command("set window-scale 0.5");
        if (ImGui::MenuItem("1:1 Original")) mpv->command("set window-scale 1");
        if (ImGui::MenuItem("2:1 Double")) mpv->command("set window-scale 2");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Panscan")) {
        if (ImGui::MenuItem("Increase Size", "Alt++")) mpv->command("add video-zoom 0.1");
        if (ImGui::MenuItem("Decrease Size", "Alt+-")) mpv->command("add video-zoom -0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("Increase Height", "W")) mpv->command("add panscan 0.1");
        if (ImGui::MenuItem("Decrease Height", "w")) mpv->command("add panscan -0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("Move Left", "Alt+right")) mpv->command("add video-pan-x -0.1");
        if (ImGui::MenuItem("Move Right", "Alt+left")) mpv->command("add video-pan-x 0.1");
        if (ImGui::MenuItem("Move Down", "Alt+up")) mpv->command("add video-pan-y 0.1");
        if (ImGui::MenuItem("Move Up", "Alt+down")) mpv->command("add video-pan-y -0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("Reset", "Alt+BS")) mpv->command("set video-zoom 0; set video-pan-x 0 ; set video-pan-y 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Aspect Ratio")) {
        if (ImGui::MenuItem("16:9")) mpv->command("set video-aspect 16:9");
        if (ImGui::MenuItem("4:3")) mpv->command("set video-aspect 4:3");
        if (ImGui::MenuItem("2.35:1")) mpv->command("set video-aspect 2.35:1");
        if (ImGui::MenuItem("1:1")) mpv->command("set video-aspect 1:1");
        if (ImGui::MenuItem("Reset")) mpv->command("set video-aspect -1");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Equalizer")) {
        if (ImGui::MenuItem("Brightness +1", "4")) mpv->command("add brightness 1");
        if (ImGui::MenuItem("Brightness -1", "3")) mpv->command("add brightness -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Contrast +1", "2")) mpv->command("add contrast 1");
        if (ImGui::MenuItem("Contrast -1", "1")) mpv->command("add contrast -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Saturation +1", "8")) mpv->command("add saturation 1");
        if (ImGui::MenuItem("Saturation -1", "7")) mpv->command("add saturation -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Gamma +1", "6")) mpv->command("add gamma 1");
        if (ImGui::MenuItem("Gamma -1", "5")) mpv->command("add gamma -1");
        ImGui::Separator();
        if (ImGui::MenuItem("Hue +1")) mpv->command("add hue 1");
        if (ImGui::MenuItem("Hue -1")) mpv->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("HW Decoding", "Ctrl+h")) mpv->command("cycle-values hwdec auto no");
      if (ImGui::MenuItem("Deinterlace", "d")) mpv->command("cycle deinterlace");
      ImGui::Separator();
      if (ImGui::MenuItem("Control Panel")) mpv->command("script-message-to implay quickview video");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Subtitle", ICON_FA_FONT)) {
      drawTracklist("sub", "sid");
      if (ImGui::MenuItemEx("Load..", ICON_FA_FOLDER_OPEN)) mpv->command("script-message-to implay load-sub");
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
      ImGui::Separator();
      if (ImGui::MenuItem("Control Panel")) mpv->command("script-message-to implay quickview subtitle");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("Fullscreen", ICON_FA_EXPAND, "f")) mpv->command("cycle fullscreen");
    if (ImGui::MenuItemEx("Quick Panel", ICON_FA_COGS)) mpv->command("script-message-to implay quickview");
    if (ImGui::MenuItemEx("Command Palette", ICON_FA_SEARCH, "Ctrl+Shift+p"))
      mpv->command("script-message-to implay command-palette");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Tools", ICON_FA_TOOLS)) {
      if (ImGui::MenuItemEx("Screenshot", ICON_FA_FILE_IMAGE, "s")) mpv->command("async screenshot");
      if (ImGui::MenuItemEx("Window Border", ICON_FA_BORDER_NONE)) mpv->command("cycle border");
      if (ImGui::MenuItemEx("Window Dragging", ICON_FA_HAND_POINTER)) mpv->command("cycle window-dragging");
      if (ImGui::MenuItemEx("Window Ontop", ICON_FA_ARROW_UP, "T")) mpv->command("cycle ontop");
      if (ImGui::MenuItemEx("Show Progress", ICON_FA_SPINNER, "o", false, playing)) mpv->command("show-progress");
      if (ImGui::MenuItem("OSC visibility", "DEL")) mpv->command("script-binding osc/visibility");
      ImGui::Separator();
      drawProfilelist();
      if (ImGui::BeginMenuEx("Theme", ICON_FA_PALETTE)) {
        const char *themes[] = {"dark", "light", "classic"};
        for (int i = 0; i < IM_ARRAYSIZE(themes); i++) {
          std::string title = tolower(themes[i]);
          title[0] = std::toupper(title[0]);
          if (ImGui::MenuItem(title.c_str(), nullptr, config->Data.Interface.Theme == themes[i])) {
            config->Data.Interface.Theme = themes[i];
            mpv->commandv("script-message-to", "implay", "theme", themes[i], nullptr);
            config->save();
          }
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenuEx("Stats", ICON_FA_INFO_CIRCLE)) {
        if (ImGui::MenuItem("Media Info", "i")) mpv->command("script-binding stats/display-page-1");
        if (ImGui::MenuItem("Extended Frame Timings")) mpv->command("script-binding stats/display-page-2");
        if (ImGui::MenuItem("Cache Statistics")) mpv->command("script-binding stats/display-page-3");
        if (ImGui::MenuItem("Internal performance info")) mpv->command("script-binding stats/display-page-0");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Metrics & Debug")) mpv->command("script-message-to implay metrics");
      if (ImGui::MenuItem("Open Config Dir")) openUri(datadir());
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("Help", ICON_FA_QUESTION_CIRCLE)) {
      if (ImGui::MenuItemEx("About", ICON_FA_INFO_CIRCLE)) mpv->command("script-message-to implay about");
      if (ImGui::MenuItemEx("Settings", ICON_FA_COG)) mpv->command("script-message-to implay settings");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::BeginMenuEx("Open", ICON_FA_FOLDER_OPEN)) {
      if (ImGui::MenuItemEx("Open Files..", ICON_FA_FILE)) mpv->command("script-message-to implay open");
      if (ImGui::MenuItemEx("Open Folder..", ICON_FA_FOLDER_PLUS)) mpv->command("script-message-to implay open-folder");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Open URL..", ICON_FA_LINK)) mpv->command("script-message-to implay open-url");
      if (ImGui::MenuItemEx("Open Clipboard", ICON_FA_CLIPBOARD))
        mpv->command("script-message-to implay open-clipboard");
      ImGui::Separator();
      if (ImGui::MenuItemEx("Open DVD/Blu-ray Folder..", ICON_FA_FOLDER_OPEN))
        mpv->command("script-message-to implay open-disk");
      if (ImGui::MenuItemEx("Open DVD/Blu-ray ISO Image..", ICON_FA_COMPACT_DISC))
        mpv->command("script-message-to implay open-iso");
      ImGui::EndMenu();
    }
    if (ImGui::MenuItemEx("Quit", ICON_FA_WINDOW_CLOSE, "q"))
      mpv->command(config->Data.Mpv.WatchLater ? "quit-watch-later" : "quit");
    ImGui::EndPopup();
  }
}

void ContextMenu::drawAudioDeviceList() {
  auto devices = mpv->audioDeviceList();
  char *name = mpv->property("audio-device");
  if (ImGui::BeginMenuEx("Devices", ICON_FA_AUDIO_DESCRIPTION, !devices.empty())) {
    for (auto &device : devices) {
      auto title = format("[{}] {}", device.description, device.name);
      if (ImGui::MenuItem(title.c_str(), nullptr, device.name == name))
        mpv->property("audio-device", device.name.c_str());
    }
    ImGui::EndMenu();
  }
  mpv_free(name);
}

void ContextMenu::drawTracklist(const char *type, const char *prop) {
  auto items = mpv->trackList();
  auto value = mpv->property(prop);
  if (ImGui::BeginMenuEx("Tracks", ICON_FA_LIST)) {
    if (ImGui::MenuItem("Disable", nullptr, strcmp(value, "no") == 0))
      mpv->commandv("cycle-values", prop, "no", "auto", nullptr);
    for (auto &track : items) {
      if (track.type != type) continue;
      auto title = track.title.empty() ? format("Track {}", track.id) : track.title;
      if (!track.lang.empty()) title += format(" [{}]", track.lang);
      if (ImGui::MenuItem(title.c_str(), nullptr, track.selected))
        mpv->property<int64_t, MPV_FORMAT_INT64>(prop, track.id);
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawChapterlist(std::vector<Mpv::ChapterItem> items) {
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("chapter");
  if (ImGui::BeginMenuEx("Chapters", ICON_FA_LIST, !items.empty())) {
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT)) mpv->command("add chapter 1");
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT)) mpv->command("add chapter -1");
    ImGui::Separator();
    if (ImGui::MenuItem("Control Panel")) mpv->command("script-message-to implay quickview chapters");
    ImGui::Separator();
    int i = 0;
    for (auto &chapter : items) {
      if (i == 10) break;
      auto title = chapter.title.empty() ? format("Chapter {}", chapter.id + 1) : chapter.title;
      title = format("{} [{:%H:%M:%S}]", title, std::chrono::duration<int>((int)chapter.time));
      if (ImGui::MenuItem(title.c_str(), nullptr, chapter.id == pos)) {
        mpv->commandv("seek", std::to_string(chapter.time).c_str(), "absolute", nullptr);
      }
      i++;
    }
    if (items.size() > 10) {
      if (ImGui::MenuItem(format("All.. ({})", items.size()).c_str()))
        mpv->command("script-message-to implay command-palette chapters");
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawPlaylist(std::vector<Mpv::PlayItem> items) {
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos");
  if (ImGui::BeginMenuEx("Playlist", ICON_FA_LIST)) {
    if (ImGui::MenuItemEx("Next", ICON_FA_ARROW_RIGHT, ">", false, items.size() > 1)) mpv->command("playlist-next");
    if (ImGui::MenuItemEx("Previous", ICON_FA_ARROW_LEFT, "<", false, items.size() > 1)) mpv->command("playlist-prev");
    ImGui::Separator();
    if (ImGui::MenuItemEx("Add Files..", ICON_FA_FILE_UPLOAD))
      mpv->command("script-message-to implay playlist-add-files");
    if (ImGui::MenuItemEx("Add Folder..", ICON_FA_FOLDER_PLUS))
      mpv->command("script-message-to implay playlist-add-folder");
    ImGui::Separator();
    if (ImGui::MenuItem("Clear")) mpv->command("playlist-clear");
    if (ImGui::MenuItem("Shuffle")) mpv->command("playlist-shuffle");
    if (ImGui::MenuItem("Loop", "L")) mpv->command("cycle-values loop-playlist inf no");
    ImGui::Separator();
    if (ImGui::MenuItem("Control Panel")) mpv->command("script-message-to implay quickview playlist");
    ImGui::Separator();
    int i = 0;
    for (auto &item : items) {
      if (i == 10) break;
      std::string title = item.title;
      if (title.empty() && !item.filename.empty()) title = item.filename;
      if (title.empty()) title = format("Item {}", item.id + 1);
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, item.id == pos))
        mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", item.id);
      i++;
    }
    if (items.size() > 10) {
      if (ImGui::MenuItem(format("All.. ({})", items.size()).c_str()))
        mpv->command("script-message-to implay command-palette playlist");
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawProfilelist() {
  auto profilelist = mpv->profileList();
  if (ImGui::BeginMenuEx("Profiles", ICON_FA_USER)) {
    for (auto &profile : profilelist) {
      if (ImGui::MenuItem(profile.c_str()))
        mpv->command(format("show-text {}; apply-profile {}", profile, profile).c_str());
    }
    ImGui::EndMenu();
  }
}
}  // namespace ImPlay::Views