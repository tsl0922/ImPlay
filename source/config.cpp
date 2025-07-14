// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include "config.h"
#include "helpers/utils.h"

namespace ImPlay {
Config::Config() {
  auto path = dataPath();
  auto file = path / "implay.conf";

  std::filesystem::create_directories(path);

  configDir = path.string();
  configFile = file.string();
}

void Config::load() {
  std::ifstream file(configFile);
  ini.parse(file);

  inipp::get_value(ini.sections["interface"], "lang", Data.Interface.Lang);
  inipp::get_value(ini.sections["interface"], "theme", Data.Interface.Theme);
  inipp::get_value(ini.sections["interface"], "scale", Data.Interface.Scale);
  inipp::get_value(ini.sections["interface"], "fps", Data.Interface.Fps);
  inipp::get_value(ini.sections["interface"], "docking", Data.Interface.Docking);
  inipp::get_value(ini.sections["interface"], "viewports", Data.Interface.Viewports);
  inipp::get_value(ini.sections["interface"], "rounding", Data.Interface.Rounding);
  inipp::get_value(ini.sections["interface"], "shadow", Data.Interface.Shadow);
  inipp::get_value(ini.sections["font"], "path", Data.Font.Path);
  inipp::get_value(ini.sections["font"], "size", Data.Font.Size);
  inipp::get_value(ini.sections["font"], "glyph-range", Data.Font.GlyphRange);
  inipp::get_value(ini.sections["mpv"], "config", Data.Mpv.UseConfig);
  inipp::get_value(ini.sections["mpv"], "wid", Data.Mpv.UseWid);
  inipp::get_value(ini.sections["mpv"], "watch-later", Data.Mpv.WatchLater);
  inipp::get_value(ini.sections["mpv"], "volume", Data.Mpv.Volume);
  inipp::get_value(ini.sections["window"], "save", Data.Window.Save);
  inipp::get_value(ini.sections["window"], "single", Data.Window.Single);
  inipp::get_value(ini.sections["window"], "x", Data.Window.X);
  inipp::get_value(ini.sections["window"], "y", Data.Window.Y);
  inipp::get_value(ini.sections["window"], "w", Data.Window.W);
  inipp::get_value(ini.sections["window"], "h", Data.Window.H);
  inipp::get_value(ini.sections["debug"], "log-level", Data.Debug.LogLevel);
  inipp::get_value(ini.sections["debug"], "log-limit", Data.Debug.LogLimit);
  inipp::get_value(ini.sections["recent"], "limit", Data.Recent.Limit);
  inipp::get_value(ini.sections["recent"], "space-to-play-last", Data.Recent.SpaceToPlayLast);

  for (auto& [key, value] : ini.sections["recent"]) {
    if (key.find("file-") != 0 || value == "") continue;
    auto parts = split(value, "|");
    recentFiles.push_back({parts.front(), parts.back()});
  }

  getLang() = Data.Interface.Lang;

  ini.clear();
}

void Config::save() {
  ini.clear();

  ini.sections["interface"]["lang"] = Data.Interface.Lang;
  ini.sections["interface"]["theme"] = Data.Interface.Theme;
  ini.sections["interface"]["scale"] = fmt::format("{}", Data.Interface.Scale);
  ini.sections["interface"]["fps"] = fmt::format("{}", Data.Interface.Fps);
  ini.sections["interface"]["docking"] = fmt::format("{}", Data.Interface.Docking);
  ini.sections["interface"]["viewports"] = fmt::format("{}", Data.Interface.Viewports);
  ini.sections["interface"]["rounding"] = fmt::format("{}", Data.Interface.Rounding);
  ini.sections["interface"]["shadow"] = fmt::format("{}", Data.Interface.Shadow);
  ini.sections["font"]["path"] = Data.Font.Path;
  ini.sections["font"]["size"] = std::to_string(Data.Font.Size);
  ini.sections["font"]["glyph-range"] = std::to_string(Data.Font.GlyphRange);
  ini.sections["mpv"]["config"] = fmt::format("{}", Data.Mpv.UseConfig);
  ini.sections["mpv"]["wid"] = fmt::format("{}", Data.Mpv.UseWid);
  ini.sections["mpv"]["watch-later"] = fmt::format("{}", Data.Mpv.WatchLater);
  ini.sections["mpv"]["volume"] = std::to_string(Data.Mpv.Volume);
  ini.sections["window"]["save"] = fmt::format("{}", Data.Window.Save);
  ini.sections["window"]["single"] = fmt::format("{}", Data.Window.Single);
  ini.sections["window"]["x"] = std::to_string(Data.Window.X);
  ini.sections["window"]["y"] = std::to_string(Data.Window.Y);
  ini.sections["window"]["w"] = std::to_string(Data.Window.W);
  ini.sections["window"]["h"] = std::to_string(Data.Window.H);
  ini.sections["debug"]["log-level"] = Data.Debug.LogLevel;
  ini.sections["debug"]["log-limit"] = std::to_string(Data.Debug.LogLimit);
  ini.sections["recent"]["limit"] = std::to_string(Data.Recent.Limit);
  ini.sections["recent"]["space-to-play-last"] = fmt::format("{}", Data.Recent.SpaceToPlayLast);

  int index = 0;
  for (auto& file : recentFiles) {
    ini.sections["recent"][fmt::format("file-{}", index++)] =
        file.path == file.title ? file.path : fmt::format("{}|{}", file.path, file.title);
  }

  std::ofstream file(configFile);
  ini.generate(file);
}

const ImWchar* Config::buildGlyphRanges() {
  ImFontAtlas* fonts = ImGui::GetIO().Fonts;
  ImFontGlyphRangesBuilder glyphRangesBuilder;
  static ImVector<ImWchar> glyphRanges;
  glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesDefault());
  glyphRangesBuilder.AddRanges(getLangGlyphRanges());
  if (Data.Font.GlyphRange & GlyphRange_Chinese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesChineseFull());
  if (Data.Font.GlyphRange & GlyphRange_Japanese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesJapanese());
  if (Data.Font.GlyphRange & GlyphRange_Cyrillic) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesCyrillic());
  if (Data.Font.GlyphRange & GlyphRange_Korean) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesKorean());
  if (Data.Font.GlyphRange & GlyphRange_Thai) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesThai());
  if (Data.Font.GlyphRange & GlyphRange_Vietnamese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesVietnamese());
  glyphRangesBuilder.BuildRanges(&glyphRanges);
  return &glyphRanges[0];
}

void Config::addRecentFile(const std::string& path, const std::string& title) {
  if (Data.Recent.Limit == 0) {
    if (recentFiles.size() > 0) recentFiles.clear();
    return;
  }
  auto item = Config::RecentItem{path, title == "" ? path : title};
  auto it = std::find(recentFiles.begin(), recentFiles.end(), item);
  if (it != recentFiles.end()) recentFiles.erase(it);
  recentFiles.insert(recentFiles.begin(), item);
  if (recentFiles.size() > Data.Recent.Limit) recentFiles.pop_back();
}

void Config::clearRecentFiles() { recentFiles.clear(); }

std::vector<Config::RecentItem>& Config::getRecentFiles() { return recentFiles; }

std::string Config::ipcSocket() {
#ifdef _WIN32
  return "\\\\.\\pipe\\mpv-ipc-socket";
#else
  return configDir + "/mpv-ipc-socket";
#endif
}
}  // namespace ImPlay