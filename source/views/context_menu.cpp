// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <fonts/fontawesome.h>
#include "helpers/utils.h"
#include "theme.h"
#include "views/context_menu.h"

namespace ImPlay::Views {
void ContextMenu::draw() {
  if (m_open) {
    ImGui::OpenPopup("##context_menu");
    m_open = false;
  }

  if (ImGui::BeginPopup("##context_menu", ImGuiWindowFlags_NoMove)) {
    if (ImGui::GetIO().AppFocusLost) ImGui::CloseCurrentPopup();
    draw(build());
    ImGui::EndPopup();
  }
}

void ContextMenu::draw(std::vector<ContextMenu::Item> items) {
  for (auto &item : items) {
    switch (item.type) {
      case TYPE_SEPARATOR:
        ImGui::Separator();
        break;
      case TYPE_NORMAL:
        if (ImGui::MenuItemEx(i18n(item.label).c_str(), item.icon.c_str(), item.shortcut.c_str(), item.selected,
                              item.enabled)) {
          if (item.cmd != "") mpv->command(item.cmd.c_str());
          if (item.callback) item.callback();
        }
        break;
      case TYPE_SUBMENU:
        if (ImGui::BeginMenuEx(i18n(item.label).c_str(), item.icon.c_str(), item.enabled)) {
          draw(item.submenu);
          ImGui::EndMenu();
        }
        break;
      case TYPE_CALLBACK:
        item.callback();
        break;
    }
  }
}

std::vector<ContextMenu::Item> ContextMenu::build() {
  bool stp = config->Data.Recent.SpaceToPlayLast;
  bool playing = mpv->playing();
  bool paused = (stp && !playing) || mpv->pause;
  auto playlist = mpv->playlist;
  auto chapters = mpv->chapters;
#ifdef __APPLE__
#define CTRL "Cmd"
#else
#define CTRL "Ctrl"
#endif
  // clang-format off
  std::vector<ContextMenu::Item> items = {
      {TYPE_NORMAL, stp ? "script-message-to implay play-pause" : "cycle pause",
        paused ? "menu.play" : "menu.pause", paused ? ICON_FA_PLAY : ICON_FA_PAUSE, "Space", stp || playing},
      {TYPE_NORMAL, "stop", "menu.stop", ICON_FA_STOP, "", playing},
      {TYPE_SEPARATOR},
      {TYPE_NORMAL, "script-message-to implay open", "menu.open.files", ICON_FA_FILE},
      {TYPE_SUBMENU, "", "menu.open", ICON_FA_FOLDER_OPEN, "", true, false, {
        {TYPE_NORMAL, "script-message-to implay open", "menu.open.files", ICON_FA_FILE},
        {TYPE_NORMAL, "script-message-to implay open-folder", "menu.open.folder", ICON_FA_FOLDER},
        {.type = TYPE_CALLBACK, .callback = [this](){ drawRecentFiles(); } },
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay open-url", "menu.open.url", ICON_FA_GLOBE},
        {TYPE_NORMAL, "script-message-to implay open-clipboard", "menu.open.clipboard"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay open-disk", "menu.open.disk"},
        {TYPE_NORMAL, "script-message-to implay open-iso", "menu.open.iso"},
      }},
      {TYPE_SEPARATOR},
      {TYPE_SUBMENU, "", "menu.playback", ICON_FA_PLAY_CIRCLE, "", true, false, {
        {TYPE_NORMAL, "seek 10", "menu.playback.seek_forward.10s", ICON_FA_FORWARD, "WHEEL_UP", playing},
        {TYPE_NORMAL, "seek 60", "menu.playback.seek_forward.1m", "", "UP", playing},
        {TYPE_NORMAL, "seek -10", "menu.playback.seek_back.10s", ICON_FA_BACKWARD, "WHEEL_DOWN", playing},
        {TYPE_NORMAL, "seek -60", "menu.playback.seek_back.1m", "", "DOWN", playing},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "frame-step", "menu.playback.next_frame", ICON_FA_ARROW_RIGHT, ".", playing},
        {TYPE_NORMAL, "frame-back-step", "menu.playback.previous_frame", ICON_FA_ARROW_LEFT, ",", playing},
        {TYPE_SUBMENU, "", "menu.playback.speed", "", "", true, false, {
          {TYPE_NORMAL, "set speed 2.0", "2x"},
          {TYPE_NORMAL, "set speed 1.5", "1.5x"},
          {TYPE_NORMAL, "set speed 1.25", "1.25x"},
          {TYPE_NORMAL, "set speed 1.0", "1.0x"},
          {TYPE_NORMAL, "set speed 0.75", "0.75x"},
          {TYPE_NORMAL, "set speed 0.5", "0.5x"},
        }},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "playlist-next", "menu.playback.next_media", ICON_FA_ARROW_RIGHT, ">", playlist.size() > 1},
        {TYPE_NORMAL, "playlist-prev", "menu.playback.previous_media", ICON_FA_ARROW_LEFT, "<", playlist.size() > 1},
        {TYPE_NORMAL, "script-message-to implay command-palette playlist", "menu.playlist"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "add chapter 1", "menu.playback.next_chapter", ICON_FA_FAST_FORWARD, "PGUP", chapters.size() > 1},
        {TYPE_NORMAL, "add chapter -1", "menu.playback.previous_chapter", ICON_FA_FAST_BACKWARD, "PGDWN", chapters.size() > 1},
        {TYPE_NORMAL, "script-message-to implay command-palette chapters", "menu.chapters"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "ab-loop", "menu.playback.ab_loop", "", "l", playing},
        {TYPE_NORMAL, "cycle-values loop-file inf no", "menu.playback.file_loop", "", "L", playing},
        {TYPE_NORMAL, "cycle-values loop-playlist inf no", "menu.playback.playlist_loop"},
      }},
      {TYPE_SUBMENU, "", "menu.audio", ICON_FA_VOLUME_UP, "", true, false, {
        {.type = TYPE_CALLBACK, .callback = [this](){ drawTracklist("audio", "aid", mpv->aid); }},
        {.type = TYPE_CALLBACK, .callback = [this](){ drawAudioDeviceList(); }},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "add volume 2", "menu.audio.inc_volume", ICON_FA_VOLUME_UP, "0", true},
        {TYPE_NORMAL, "add volume -2", "menu.audio.dec_volume", ICON_FA_VOLUME_DOWN, "9", true},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "cycle mute", "menu.audio.mute", ICON_FA_VOLUME_MUTE, "m", true},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "add audio-delay 0.1", "menu.audio.inc_delay", "", "Ctrl +", true},
        {TYPE_NORMAL, "add audio-delay -0.1", "menu.audio.dec_delay", "", "Ctrl -", true},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay quickview audio", "menu.quickview"},
      }},
      {TYPE_SUBMENU, "", "menu.video", ICON_FA_VIDEO, "", true, false, {
        {.type = TYPE_CALLBACK, .callback = [this](){ drawTracklist("video", "vid", mpv->vid); }},
        {TYPE_SUBMENU, "", "menu.video.rotate", ICON_FA_SPINNER, "", true, false, {
          {TYPE_NORMAL, "set video-rotate 90", "90°"},
          {TYPE_NORMAL, "set video-rotate 180", "180°"},
          {TYPE_NORMAL, "set video-rotate 270", "270°"},
          {TYPE_NORMAL, "set video-rotate 0", "0°"},
        }},
        {TYPE_SUBMENU, "", "menu.video.scale", ICON_FA_GLASSES, "", true, false, {
          {TYPE_NORMAL, "add window-scale -0.1", "menu.video.scale.in", ICON_FA_MINUS_CIRCLE},
          {TYPE_NORMAL, "add window-scale 0.1", "menu.video.scale.out", ICON_FA_PLUS_CIRCLE},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "set window-scale 0.25", "menu.video.scale.1_4"},
          {TYPE_NORMAL, "set window-scale 0.5", "menu.video.scale.1_2"},
          {TYPE_NORMAL, "set window-scale 1.0", "menu.video.scale.1_1"},
          {TYPE_NORMAL, "set window-scale 2.0", "menu.video.scale.2_1"},
        }},
        {TYPE_SUBMENU, "", "menu.video.panscan", "", "", true, false, {
          {TYPE_NORMAL, "add panscan 0.1", "menu.video.panscan.inc", "", "W"},
          {TYPE_NORMAL, "add panscan -0.1", "menu.video.panscan.dec", "", "w"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "add video-pan-x -0.1", "menu.video.panscan.move_left", "", "Alt+right"},
          {TYPE_NORMAL, "add video-pan-x 0.1", "menu.video.panscan.move_right", "", "Alt+left"},
          {TYPE_NORMAL, "add video-pan-y -0.1", "menu.video.panscan.move_down", "", "Alt+up"},
          {TYPE_NORMAL, "add video-pan-y 0.1", "menu.video.panscan.move_up", "", "Alt+down"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "add video-zoom 0.1", "menu.video.panscan.zoom_out", "", "Alt++"},
          {TYPE_NORMAL, "add video-zoom -0.1", "menu.video.panscan.zoom_in", "", "Alt+-"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "set video-zoom 0; set video-pan-x 0 ; set video-pan-y 0", "menu.video.panscan.reset", "", "Alt+BS"},
        }},
        {TYPE_SUBMENU, "", "menu.video.aspect_ratio", "", "", true, false, {
          {TYPE_NORMAL, "set video-aspect-override 16:9", "16:9"},
          {TYPE_NORMAL, "set video-aspect-override 16:10", "16:10"},
          {TYPE_NORMAL, "set video-aspect-override 4:3", "4:3"},
          {TYPE_NORMAL, "set video-aspect-override 2.35:1", "2.35:1"},
          {TYPE_NORMAL, "set video-aspect-override 1.85:1", "1.85:1"},
          {TYPE_NORMAL, "set video-aspect-override 1:1", "1:1"},
          {TYPE_NORMAL, "set video-aspect-override -1", "menu.video.aspect_ratio.reset"},
        }},
        {TYPE_SUBMENU, "", "menu.video.equalizer", "", "", true, false, {
          {TYPE_NORMAL, "add brightness 1", "menu.video.equalizer.inc_brightness", "", "4"},
          {TYPE_NORMAL, "add brightness -1", "menu.video.equalizer.dec_brightness", "", "3"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "add contrast 1", "menu.video.equalizer.inc_contrast", "", "2"},
          {TYPE_NORMAL, "add contrast -1", "menu.video.equalizer.dec_contrast", "", "1"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "add saturation 1", "menu.video.equalizer.inc_saturation", "", "8"},
          {TYPE_NORMAL, "add saturation -1", "menu.video.equalizer.dec_saturation", "", "7"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "add gamma 1", "menu.video.equalizer.inc_gamma", "", "6"},
          {TYPE_NORMAL, "add gamma -1", "menu.video.equalizer.dec_gamma", "", "5"},
          {TYPE_SEPARATOR},
          {TYPE_NORMAL, "add hue -1", "menu.video.equalizer.inc_hue"},
          {TYPE_NORMAL, "add hue 1", "menu.video.equalizer.dec_hue"},
        }},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "cycle-values hwdec auto no", "menu.video.hw_decoding", "", "Ctrl+h"},
        {TYPE_NORMAL, "cycle deinterlace", "menu.video.deinterlace", "", "d"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay quickview video", "menu.quickview"},
      }},
      {TYPE_SUBMENU, "", "menu.subtitle", ICON_FA_FONT, "", true, false, {
        {.type = TYPE_CALLBACK, .callback = [this](){ drawTracklist("sub", "sid", mpv->sid); }},
        {TYPE_NORMAL, "script-message-to implay load-sub", "menu.subtitle.load", ICON_FA_FOLDER_OPEN},
        {TYPE_NORMAL, "cycle sub-visibility", "menu.subtitle.show_hide", "", "v"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "add sub-pos -1", "menu.subtitle.move_up", "", "r"},
        {TYPE_NORMAL, "add sub-pos +1", "menu.subtitle.move_down", "", "R"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "add sub-delay 0.1", "menu.subtitle.inc_delay", "", "z"},
        {TYPE_NORMAL, "add sub-delay -0.1", "menu.subtitle.dec_delay", "", "Z"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "add sub-scale 0.1", "menu.subtitle.inc_scale", "", "F"},
        {TYPE_NORMAL, "add sub-scale -0.1", "menu.subtitle.dec_scale", "", "G"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay quickview subtitle", "menu.quickview"},
      }},
      {TYPE_NORMAL, "cycle fullscreen", "menu.fullscreen", ICON_FA_EXPAND, "f"},
      {TYPE_SEPARATOR},
      {TYPE_SUBMENU, "", "menu.playlist", ICON_FA_TASKS, "", true, false, {
        {TYPE_NORMAL, "playlist-next", "menu.playlist.next", ICON_FA_ARROW_RIGHT, ">", playlist.size() > 1},
        {TYPE_NORMAL, "playlist-prev", "menu.playlist.previous", ICON_FA_ARROW_LEFT, "<", playlist.size() > 1},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay playlist-add-files", "menu.playlist.add_files", ICON_FA_PLUS},
        {TYPE_NORMAL, "script-message-to implay playlist-add-folder", "menu.playlist.add_folder", ICON_FA_FOLDER_PLUS},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "playlist-shuffle", "menu.playlist.shuffle", ICON_FA_RANDOM},
        {TYPE_NORMAL, "cycle-values loop-playlist inf no", "menu.playlist.loop"},
        {TYPE_NORMAL, "playlist-clear", "menu.playlist.clear"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay quickview playlist", "menu.quickview"},
        {.type = TYPE_CALLBACK, .callback = [playlist, this](){ drawPlaylist(playlist); }},
      }},
      {TYPE_SUBMENU, "", "menu.chapters", ICON_FA_LIST_OL, "", true, false, {
        {TYPE_NORMAL, "add chapter 1", "menu.chapters.next", ICON_FA_FAST_FORWARD, "", chapters.size() > 1},
        {TYPE_NORMAL, "add chapter -1", "menu.chapters.previous", ICON_FA_FAST_BACKWARD, "", chapters.size() > 1},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay quickview chapters", "menu.quickview"},
        {.type = TYPE_CALLBACK, .callback = [chapters, this](){ drawChapterlist(chapters); }},
      }},
      {TYPE_NORMAL, "script-message-to implay quickview", "menu.quickview", ICON_FA_COGS},
      {TYPE_NORMAL, "script-message-to implay command-palette", "menu.command_palette", ICON_FA_SEARCH, CTRL"+Shift+p"},
      {TYPE_SEPARATOR},
      {TYPE_SUBMENU, "", "menu.tools", ICON_FA_TOOLS, "", true, false, {
        {TYPE_NORMAL, "async screenshot", "menu.tools.screenshot", ICON_FA_FILE_IMAGE, "s", playing},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "cycle border", "menu.tools.window_border", ICON_FA_BORDER_NONE},
        {TYPE_NORMAL, "cycle window-dragging", "menu.tools.window_dragging", ICON_FA_HAND_POINTER},
        {TYPE_NORMAL, "cycle ontop", "menu.tools.window_ontop", ICON_FA_ARROW_UP, "T"},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "show-progress", "menu.tools.show_progress", ICON_FA_SPINNER, "o", playing},
        {TYPE_NORMAL, "script-binding stats/display-stats-toggle", "menu.tools.show_stats", ICON_FA_CHART_BAR, "I"},
        {TYPE_NORMAL, "script-binding osc/visibility", "menu.tools.osc_visibility", "", "DEL"},
        {TYPE_SEPARATOR},
        {.type = TYPE_CALLBACK, .callback = [this](){ drawProfilelist(); }},
        {TYPE_SEPARATOR},
        {TYPE_NORMAL, "script-message-to implay metrics", "menu.tools.debug", "", "`"},
        {TYPE_NORMAL, "script-message-to implay open-config-dir", "menu.tools.open_config_dir"},
      }},
      {.type = TYPE_CALLBACK, .callback = [this](){ drawThemelist(); }},
      {TYPE_NORMAL, "script-message-to implay settings", "menu.settings", ICON_FA_COG},
      {TYPE_NORMAL, "script-message-to implay about", "menu.about", ICON_FA_INFO_CIRCLE},
      {TYPE_SEPARATOR},
      {TYPE_NORMAL, config->Data.Mpv.WatchLater ? "quit-watch-later" : "quit", "menu.quit", ICON_FA_WINDOW_CLOSE, "q"},
  };
#undef CTRL
  // clang-format on
  return items;
}

void ContextMenu::drawPlaylist(std::vector<Mpv::PlayItem> items) {
  if (items.empty()) return;

  auto pos = mpv->playlistPos;
  int i = 0;

  ImGui::Separator();
  for (auto &item : items) {
    if (i == 10) break;
    std::string title = item.title;
    if (title.empty() && !item.filename().empty()) title = item.filename();
    if (title.empty()) title = i18n_a("menu.playlist.item", item.id + 1);
    if (ImGui::MenuItemEx(title.c_str(), nullptr, nullptr, item.id == pos))
      mpv->commandv("playlist-play-index", std::to_string(item.id).c_str(), nullptr);
    i++;
  }
  if (items.size() > 10) {
    if (ImGui::MenuItem(fmt::format("{} ({})", "menu.playlist.all"_i18n, items.size()).c_str()))
      mpv->command("script-message-to implay command-palette playlist");
  }
}

void ContextMenu::drawChapterlist(std::vector<Mpv::ChapterItem> items) {
  if (items.empty()) return;

  auto pos = mpv->chapter;
  int i = 0;

  ImGui::Separator();
  for (auto &chapter : items) {
    if (i == 10) break;
    auto title = chapter.title.empty() ? fmt::format("Chapter {}", chapter.id + 1) : chapter.title;
    title = fmt::format("{} [{:%H:%M:%S}]", title, std::chrono::duration<int>((int)chapter.time));
    if (ImGui::MenuItem(title.c_str(), nullptr, chapter.id == pos)) {
      mpv->commandv("seek", std::to_string(chapter.time).c_str(), "absolute", nullptr);
    }
    i++;
  }
  if (items.size() > 10) {
    if (ImGui::MenuItem(fmt::format("{} ({})", "menu.chapters.all"_i18n, items.size()).c_str()))
      mpv->command("script-message-to implay command-palette chapters");
  }
}

void ContextMenu::drawTracklist(const char *type, const char *prop, std::string pos) {
  std::vector<Mpv::TrackItem> tracks;
  for (auto &track : mpv->tracks) {
    if (track.type == type) tracks.push_back(track);
  }
  if (ImGui::BeginMenuEx("menu.tracks"_i18n, ICON_FA_LIST, !tracks.empty())) {
    for (auto &track : tracks) {
      auto title = track.title.empty() ? i18n_a("menu.tracks.item", track.id) : track.title;
      if (!track.lang.empty()) title += fmt::format(" [{}]", track.lang);
      if (ImGui::MenuItem(title.c_str(), nullptr, track.selected))
        mpv->property<int64_t, MPV_FORMAT_INT64>(prop, track.id);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("menu.tracks.disable"_i18n, nullptr, pos == "no"))
      mpv->commandv("cycle-values", prop, "no", "auto", nullptr);
    ImGui::EndMenu();
  }
}

void ContextMenu::drawAudioDeviceList() {
  auto devices = mpv->audioDevices;
  if (ImGui::BeginMenuEx("menu.audio.devices"_i18n, ICON_FA_AUDIO_DESCRIPTION, !devices.empty())) {
    for (auto &device : devices) {
      auto title = fmt::format("[{}] {}", device.description, device.name);
      if (ImGui::MenuItem(title.c_str(), nullptr, device.name == mpv->audioDevice))
        mpv->property("audio-device", device.name.c_str());
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawThemelist() {
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
}

void ContextMenu::drawProfilelist() {
  if (ImGui::BeginMenuEx("menu.tools.profiles"_i18n, ICON_FA_USER_COG)) {
    for (auto &profile : mpv->profiles) {
      if (ImGui::MenuItem(profile.c_str()))
        mpv->command(fmt::format("show-text {}; apply-profile {}", profile, profile).c_str());
    }
    ImGui::EndMenu();
  }
}

void ContextMenu::drawRecentFiles() {
  if (ImGui::BeginMenuEx("menu.open.recent"_i18n, ICON_FA_HISTORY)) {
    auto files = config->getRecentFiles();
    auto size = files.size();
    int i = 0;
    for (auto &file : files) {
      if (i == 10) break;
      if (ImGui::MenuItem(file.title.c_str())) {
        mpv->commandv("loadfile", file.path.c_str(), nullptr);
        mpv->commandv("set", "force-media-title", file.title.c_str(), nullptr);
      }
      i++;
    }
    if (size > 10) {
      if (ImGui::MenuItem(fmt::format("{} ({})", "menu.open.recent.all"_i18n, files.size()).c_str()))
        mpv->command("script-message-to implay command-palette history");
    }
    if (size > 0) ImGui::Separator();
    if (ImGui::MenuItem("menu.open.recent.clear"_i18n)) config->clearRecentFiles();
    ImGui::EndMenu();
  }
}
}  // namespace ImPlay::Views