// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <fonts/fontawesome.h>
#include "helpers.h"
#include "theme.h"
#include "views/context_menu.h"

namespace ImPlay::Views {
void ContextMenu::draw() {
  if (m_open) {
    ImGui::OpenPopup("##context_menu");
    m_open = false;
  }

  if (ImGui::BeginPopup("##context_menu", ImGuiWindowFlags_NoMove)) {
    bool paused = mpv->paused();
    bool playing = mpv->playing();
    auto playlist = mpv->playlist;
    auto chapterlist = mpv->chapters;
    if (ImGui::GetIO().AppFocusLost || ImGui::GetWindowViewport()->Flags & ImGuiViewportFlags_Minimized)
      ImGui::CloseCurrentPopup();
    if (ImGui::MenuItemEx(paused ? "menu.play"_i18n : "menu.pause"_i18n, paused ? ICON_FA_PLAY : ICON_FA_PAUSE, "Space",
                          false, playing))
      mpv->command("cycle pause");
    if (ImGui::MenuItemEx("menu.stop"_i18n, ICON_FA_STOP, nullptr, false, playing)) mpv->command("stop");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("menu.playback"_i18n, ICON_FA_PLAY_CIRCLE)) {
      if (ImGui::MenuItemEx("menu.playback.seek_forward.10s"_i18n, ICON_FA_FORWARD, "WHEEL_UP", false, playing))
        mpv->command("seek 10");
      if (ImGui::MenuItem("menu.playback.seek_forward.1m"_i18n, "UP", false, playing)) mpv->command("seek 60");
      if (ImGui::MenuItemEx("menu.playback.seek_back.10s"_i18n, ICON_FA_BACKWARD, "WHEEL_DOWN", false, playing))
        mpv->command("seek -10");
      if (ImGui::MenuItem("menu.playback.seek_back.1m"_i18n, "DOWN", false, playing)) mpv->command("seek -60");
      if (ImGui::BeginMenu("menu.playback.speed"_i18n, playing)) {
        if (ImGui::MenuItem("2x")) mpv->command("multiply speed 2.0");
        if (ImGui::MenuItem("1.5x")) mpv->command("multiply speed 1.5");
        if (ImGui::MenuItem("1.25x")) mpv->command("multiply speed 1.25");
        if (ImGui::MenuItem("1.0x")) mpv->command("set speed 1.0");
        if (ImGui::MenuItem("0.75x")) mpv->command("multiply speed 0.75");
        if (ImGui::MenuItem("0.5x")) mpv->command("multiply speed 0.5");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.playback.next_frame"_i18n, ICON_FA_ARROW_RIGHT, ".", false, playing))
        mpv->command("frame-step");
      if (ImGui::MenuItemEx("menu.playback.previous_frame"_i18n, ICON_FA_ARROW_LEFT, ",", false, playing))
        mpv->command("frame-back-step");
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.playback.next_media"_i18n, ICON_FA_ARROW_RIGHT, ">", false, playlist.size() > 1))
        mpv->command("playlist-next");
      if (ImGui::MenuItemEx("menu.playback.previous_media"_i18n, ICON_FA_ARROW_LEFT, "<", false, playlist.size() > 1))
        mpv->command("playlist-prev");
      if (ImGui::MenuItemEx("menu.playlist"_i18n, ICON_FA_LIST, nullptr, false, playlist.size() > 0))
        mpv->command("script-message-to implay command-palette playlist");
      if (ImGui::MenuItem("menu.playback.playlist_loop"_i18n, nullptr, false, playlist.size() > 0))
        mpv->command("cycle-values loop-playlist inf no");
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.playback.next_chapter"_i18n, ICON_FA_ARROW_RIGHT, nullptr, false,
                            chapterlist.size() > 1))
        mpv->command("add chapter 1");
      if (ImGui::MenuItemEx("menu.playback.previous_chapter"_i18n, ICON_FA_ARROW_LEFT, nullptr, false,
                            chapterlist.size() > 1))
        mpv->command("add chapter -1");
      if (ImGui::MenuItemEx("menu.chapters"_i18n, ICON_FA_LIST, nullptr, false, chapterlist.size() > 0))
        mpv->command("script-message-to implay command-palette chapters");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.playback.ab_loop"_i18n, "l", false, playing)) mpv->command("ab-loop");
      if (ImGui::MenuItem("menu.playback.file_loop"_i18n, "L", false, playing))
        mpv->command("cycle-values loop-file inf no");
      ImGui::EndMenu();
    }
    drawPlaylist(playlist);
    drawChapterlist(chapterlist);
    ImGui::Separator();
    if (ImGui::BeginMenuEx("menu.audio"_i18n, ICON_FA_VOLUME_UP)) {
      drawTracklist("audio", "aid", mpv->aid);
      drawAudioDeviceList();
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.audio.inc_volume"_i18n, ICON_FA_VOLUME_UP, "0")) mpv->command("add volume 2");
      if (ImGui::MenuItemEx("menu.audio.dec_volume"_i18n, ICON_FA_VOLUME_DOWN, "9")) mpv->command("add volume -2");
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.audio.mute"_i18n, ICON_FA_VOLUME_MUTE, "m")) mpv->command("cycle mute");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.audio.inc_delay"_i18n, "Ctrl +")) mpv->command("add audio-delay 0.1");
      if (ImGui::MenuItem("menu.audio.dec_delay"_i18n, "Ctrl -")) mpv->command("add audio-delay -0.1");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.quickview"_i18n)) mpv->command("script-message-to implay quickview audio");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("menu.video"_i18n, ICON_FA_VIDEO)) {
      drawTracklist("video", "vid", mpv->vid);
      if (ImGui::BeginMenuEx("menu.video.rotate"_i18n, ICON_FA_SPINNER)) {
        if (ImGui::MenuItem("90°")) mpv->command("set video-rotate 90");
        if (ImGui::MenuItem("180°")) mpv->command("set video-rotate 180");
        if (ImGui::MenuItem("270°")) mpv->command("set video-rotate 270");
        if (ImGui::MenuItem("0°")) mpv->command("set video-rotate 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenuEx("menu.video.scale"_i18n, ICON_FA_GLASSES)) {
        if (ImGui::MenuItemEx("menu.video.scale.in"_i18n, ICON_FA_MINUS_CIRCLE)) mpv->command("add window-scale -0.1");
        if (ImGui::MenuItemEx("menu.video.scale.out"_i18n, ICON_FA_PLUS_CIRCLE)) mpv->command("add window-scale 0.1");
        if (ImGui::MenuItem("menu.video.scale.reset"_i18n)) mpv->command("set window-scale 0");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.scale.1_4"_i18n)) mpv->command("set window-scale 0.25");
        if (ImGui::MenuItem("menu.video.scale.1_2"_i18n)) mpv->command("set window-scale 0.5");
        if (ImGui::MenuItem("menu.video.scale.1_1"_i18n)) mpv->command("set window-scale 1");
        if (ImGui::MenuItem("menu.video.scale.2_1"_i18n)) mpv->command("set window-scale 2");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("menu.video.panscan"_i18n)) {
        if (ImGui::MenuItem("menu.video.panscan.inc"_i18n, "W")) mpv->command("add panscan 0.1");
        if (ImGui::MenuItem("menu.video.panscan.dec"_i18n, "w")) mpv->command("add panscan -0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.panscan.move_left"_i18n, "Alt+right")) mpv->command("add video-pan-x -0.1");
        if (ImGui::MenuItem("menu.video.panscan.move_right"_i18n, "Alt+left")) mpv->command("add video-pan-x 0.1");
        if (ImGui::MenuItem("menu.video.panscan.move_down"_i18n, "Alt+up")) mpv->command("add video-pan-y 0.1");
        if (ImGui::MenuItem("menu.video.panscan.move_up"_i18n, "Alt+down")) mpv->command("add video-pan-y -0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.panscan.zoom_out"_i18n, "Alt++")) mpv->command("add video-zoom 0.1");
        if (ImGui::MenuItem("menu.video.panscan.zoom_in"_i18n, "Alt+-")) mpv->command("add video-zoom -0.1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.panscan.reset"_i18n, "Alt+BS"))
          mpv->command("set video-zoom 0; set video-pan-x 0 ; set video-pan-y 0");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("menu.video.aspect_ratio"_i18n)) {
        if (ImGui::MenuItem("16:9")) mpv->command("set video-aspect 16:9");
        if (ImGui::MenuItem("16:10")) mpv->command("set video-aspect 16:10");
        if (ImGui::MenuItem("4:3")) mpv->command("set video-aspect 4:3");
        if (ImGui::MenuItem("2.35:1")) mpv->command("set video-aspect 2.35:1");
        if (ImGui::MenuItem("1.85:1")) mpv->command("set video-aspect 1.85:1");
        if (ImGui::MenuItem("1:1")) mpv->command("set video-aspect 1:1");
        if (ImGui::MenuItem("menu.video.aspect_ratio.reset"_i18n)) mpv->command("set video-aspect -1");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("menu.video.equalizer"_i18n)) {
        if (ImGui::MenuItem("menu.video.equalizer.inc_brightness"_i18n, "4")) mpv->command("add brightness 1");
        if (ImGui::MenuItem("menu.video.equalizer.dec_brightness"_i18n, "3")) mpv->command("add brightness -1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.equalizer.inc_contrast"_i18n, "2")) mpv->command("add contrast 1");
        if (ImGui::MenuItem("menu.video.equalizer.dec_contrast"_i18n, "1")) mpv->command("add contrast -1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.equalizer.inc_saturation"_i18n, "8")) mpv->command("add saturation 1");
        if (ImGui::MenuItem("menu.video.equalizer.dec_saturation"_i18n, "7")) mpv->command("add saturation -1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.equalizer.inc_gamma"_i18n, "6")) mpv->command("add gamma 1");
        if (ImGui::MenuItem("menu.video.equalizer.dec_gamma"_i18n, "5")) mpv->command("add gamma -1");
        ImGui::Separator();
        if (ImGui::MenuItem("menu.video.equalizer.inc_hue"_i18n)) mpv->command("add hue 1");
        if (ImGui::MenuItem("menu.video.equalizer.dec_hue"_i18n)) mpv->command("add hue -1");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("menu.video.hw_decoding"_i18n, "Ctrl+h")) mpv->command("cycle-values hwdec auto no");
      if (ImGui::MenuItem("menu.video.deinterlace"_i18n, "d")) mpv->command("cycle deinterlace");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.quickview"_i18n)) mpv->command("script-message-to implay quickview video");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("menu.subtitle"_i18n, ICON_FA_FONT)) {
      drawTracklist("sub", "sid", mpv->sid);
      if (ImGui::MenuItemEx("menu.subtitle.load"_i18n, ICON_FA_FOLDER_OPEN))
        mpv->command("script-message-to implay load-sub");
      if (ImGui::MenuItem("menu.subtitle.show_hide"_i18n, "v")) mpv->command("cycle sub-visibility");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.subtitle.move_up"_i18n, "r")) mpv->command("add sub-pos -1");
      if (ImGui::MenuItem("menu.subtitle.move_down"_i18n, "R")) mpv->command("add sub-pos +1");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.subtitle.inc_delay"_i18n, "z")) mpv->command("add sub-delay 0.1");
      if (ImGui::MenuItem("menu.subtitle.dec_delay"_i18n, "Z")) mpv->command("add sub-delay -0.1");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.subtitle.inc_scale"_i18n, "F")) mpv->command("add sub-scale 0.1");
      if (ImGui::MenuItem("menu.subtitle.dec_scale"_i18n, "G")) mpv->command("add sub-scale -0.1");
      ImGui::Separator();
      if (ImGui::MenuItem("menu.quickview"_i18n)) mpv->command("script-message-to implay quickview subtitle");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItemEx("menu.fullscreen"_i18n, ICON_FA_EXPAND, "f")) mpv->command("cycle fullscreen");
    if (ImGui::MenuItemEx("menu.quickview"_i18n, ICON_FA_COGS)) mpv->command("script-message-to implay quickview");
    if (ImGui::MenuItemEx("menu.command_palette"_i18n, ICON_FA_SEARCH, "Ctrl+Shift+p"))
      mpv->command("script-message-to implay command-palette");
    ImGui::Separator();
    if (ImGui::BeginMenuEx("menu.tools"_i18n, ICON_FA_TOOLS)) {
      if (ImGui::MenuItemEx("menu.tools.screenshot"_i18n, ICON_FA_FILE_IMAGE, "s")) mpv->command("async screenshot");
      if (ImGui::MenuItemEx("menu.tools.window_border"_i18n, ICON_FA_BORDER_NONE)) mpv->command("cycle border");
      if (ImGui::MenuItemEx("menu.tools.window_dragging"_i18n, ICON_FA_HAND_POINTER))
        mpv->command("cycle window-dragging");
      if (ImGui::MenuItemEx("menu.tools.window_ontop"_i18n, ICON_FA_ARROW_UP, "T")) mpv->command("cycle ontop");
      if (ImGui::MenuItemEx("menu.tools.show_progress"_i18n, ICON_FA_SPINNER, "o", false, playing))
        mpv->command("show-progress");
      if (ImGui::MenuItem("menu.tools.osc_visibility"_i18n, "DEL")) mpv->command("script-binding osc/visibility");
      ImGui::Separator();
      drawProfilelist();
      if (ImGui::BeginMenuEx("menu.tools.theme"_i18n, ICON_FA_PALETTE)) {
        auto themes = ImGui::Themes();
        for (auto &title : themes) {
          auto theme = tolower(title);
          if (ImGui::MenuItem(title, nullptr, config->Data.Interface.Theme == theme)) {
            config->Data.Interface.Theme = theme;
            mpv->commandv("script-message-to", "implay", "theme", theme.c_str(), nullptr);
            config->save();
          }
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenuEx("menu.tools.stats"_i18n, ICON_FA_INFO_CIRCLE)) {
        if (ImGui::MenuItem("menu.tools.stats.1"_i18n, "i")) mpv->command("script-binding stats/display-page-1");
        if (ImGui::MenuItem("menu.tools.stats.2"_i18n)) mpv->command("script-binding stats/display-page-2");
        if (ImGui::MenuItem("menu.tools.stats.3"_i18n)) mpv->command("script-binding stats/display-page-3");
        if (ImGui::MenuItem("menu.tools.stats.4"_i18n)) mpv->command("script-binding stats/display-page-0");
        ImGui::EndMenu();
      }
      ImGui::Separator();
      if (ImGui::MenuItem("menu.tools.debug"_i18n)) mpv->command("script-message-to implay metrics");
      if (ImGui::MenuItem("menu.tools.open_config_dir"_i18n)) openUrl(config->dir());
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenuEx("menu.help"_i18n, ICON_FA_QUESTION_CIRCLE)) {
      if (ImGui::MenuItemEx("menu.help.about"_i18n, ICON_FA_INFO_CIRCLE))
        mpv->command("script-message-to implay about");
      if (ImGui::MenuItemEx("menu.help.settings"_i18n, ICON_FA_COG)) mpv->command("script-message-to implay settings");
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::BeginMenuEx("menu.open"_i18n, ICON_FA_FOLDER_OPEN)) {
      if (ImGui::MenuItemEx("menu.open.files"_i18n, ICON_FA_FILE)) mpv->command("script-message-to implay open");
      if (ImGui::MenuItemEx("menu.open.folder"_i18n, ICON_FA_FOLDER_PLUS))
        mpv->command("script-message-to implay open-folder");
      drawRecentFiles();
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.open.url"_i18n, ICON_FA_LINK)) mpv->command("script-message-to implay open-url");
      if (ImGui::MenuItemEx("menu.open.clipboard"_i18n, ICON_FA_CLIPBOARD))
        mpv->command("script-message-to implay open-clipboard");
      ImGui::Separator();
      if (ImGui::MenuItemEx("menu.open.disk"_i18n, ICON_FA_FOLDER_OPEN))
        mpv->command("script-message-to implay open-disk");
      if (ImGui::MenuItemEx("menu.open.iso"_i18n, ICON_FA_COMPACT_DISC))
        mpv->command("script-message-to implay open-iso");
      ImGui::EndMenu();
    }
    if (ImGui::MenuItemEx("menu.quit"_i18n, ICON_FA_WINDOW_CLOSE, "q"))
      mpv->command(config->Data.Mpv.WatchLater ? "quit-watch-later" : "quit");
    ImGui::EndPopup();
  }
}

void ContextMenu::drawAudioDeviceList() {
  auto devices = mpv->audioDevices;
  if (ImGui::BeginMenuEx("menu.audio.devices"_i18n, ICON_FA_AUDIO_DESCRIPTION, !devices.empty())) {
    for (auto &device : devices) {
      auto title = format("[{}] {}", device.description, device.name);
      if (ImGui::MenuItem(title.c_str(), nullptr, device.name == mpv->audioDevice))
        mpv->property("audio-device", device.name.c_str());
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawTracklist(const char *type, const char *prop, std::string pos) {
  if (ImGui::BeginMenuEx("menu.tracks"_i18n, ICON_FA_LIST)) {
    if (ImGui::MenuItem("menu.tracks.disable"_i18n, nullptr, pos == "no"))
      mpv->commandv("cycle-values", prop, "no", "auto", nullptr);
    for (auto &track : mpv->tracks) {
      if (track.type != type) continue;
      auto title = track.title.empty() ? format("menu.tracks.item"_i18n, track.id) : track.title;
      if (!track.lang.empty()) title += format(" [{}]", track.lang);
      if (ImGui::MenuItem(title.c_str(), nullptr, track.selected))
        mpv->property<int64_t, MPV_FORMAT_INT64>(prop, track.id);
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawChapterlist(std::vector<Mpv::ChapterItem> items) {
  auto pos = mpv->chapter;
  if (ImGui::BeginMenuEx("menu.chapters"_i18n, ICON_FA_LIST, !items.empty())) {
    if (ImGui::MenuItemEx("menu.chapters.next"_i18n, ICON_FA_ARROW_RIGHT)) mpv->command("add chapter 1");
    if (ImGui::MenuItemEx("menu.chapters.previous"_i18n, ICON_FA_ARROW_LEFT)) mpv->command("add chapter -1");
    ImGui::Separator();
    if (ImGui::MenuItem("menu.quickview"_i18n)) mpv->command("script-message-to implay quickview chapters");
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
      if (ImGui::MenuItem(format("{} ({})", "menu.chapters.all"_i18n, items.size()).c_str()))
        mpv->command("script-message-to implay command-palette chapters");
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawPlaylist(std::vector<Mpv::PlayItem> items) {
  auto pos = mpv->playlistPos;
  if (ImGui::BeginMenuEx("menu.playlist"_i18n, ICON_FA_LIST)) {
    if (ImGui::MenuItemEx("menu.playlist.next"_i18n, ICON_FA_ARROW_RIGHT, ">", false, items.size() > 1))
      mpv->command("playlist-next");
    if (ImGui::MenuItemEx("menu.playlist.previous"_i18n, ICON_FA_ARROW_LEFT, "<", false, items.size() > 1))
      mpv->command("playlist-prev");
    ImGui::Separator();
    if (ImGui::MenuItemEx("menu.playlist.add_files"_i18n, ICON_FA_FILE_UPLOAD))
      mpv->command("script-message-to implay playlist-add-files");
    if (ImGui::MenuItemEx("menu.playlist.add_folder"_i18n, ICON_FA_FOLDER_PLUS))
      mpv->command("script-message-to implay playlist-add-folder");
    ImGui::Separator();
    if (ImGui::MenuItem("menu.playlist.clear"_i18n)) mpv->command("playlist-clear");
    if (ImGui::MenuItem("menu.playlist.shuffle"_i18n)) mpv->command("playlist-shuffle");
    if (ImGui::MenuItem("menu.playlist.loop"_i18n, "L")) mpv->command("cycle-values loop-playlist inf no");
    ImGui::Separator();
    if (ImGui::MenuItem("menu.quickview"_i18n)) mpv->command("script-message-to implay quickview playlist");
    if (items.size() > 0) ImGui::Separator();
    int i = 0;
    for (auto &item : items) {
      if (i == 10) break;
      std::string title = item.title;
      if (title.empty() && !item.filename.empty()) title = item.filename;
      if (title.empty()) title = format("menu.playlist.item"_i18n, item.id + 1);
      if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, item.id == pos))
        mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", item.id);
      i++;
    }
    if (items.size() > 10) {
      if (ImGui::MenuItem(format("{} ({})", "menu.playlist.all"_i18n, items.size()).c_str()))
        mpv->command("script-message-to implay command-palette playlist");
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawProfilelist() {
  if (ImGui::BeginMenuEx("menu.tools.profiles"_i18n, ICON_FA_USER)) {
    for (auto &profile : mpv->profiles) {
      if (ImGui::MenuItem(profile.c_str()))
        mpv->command(format("show-text {}; apply-profile {}", profile, profile).c_str());
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawRecentFiles() {
  if (ImGui::BeginMenu("menu.open.recent"_i18n)) {
    auto files = config->getRecentFiles();
    int i = 0;
    for (auto &file : files) {
      if (i == 10) break;
      if (ImGui::MenuItem(file.title.c_str())) mpv->commandv("loadfile", file.path.c_str(), nullptr);
      i++;
    }
    if (files.size() > 10) {
      if (ImGui::MenuItem(format("{} ({})", "menu.open.recent.all"_i18n, files.size()).c_str()))
        mpv->command("script-message-to implay command-palette history");
    }
    if (ImGui::MenuItem("menu.open.recent.clear"_i18n)) config->clearRecentFiles();
    ImGui::EndMenu();
  }
}
}  // namespace ImPlay::Views