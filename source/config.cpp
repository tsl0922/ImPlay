// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include "config.h"
#include "helpers.h"

namespace ImPlay {
Config::Config() {
  auto path = std::filesystem::path(datadir()) / "implay";
  auto file = path / "implay.conf";

  std::filesystem::create_directories(path);

  configDir = path.string();
  configFile = file.string();
}

void Config::load() {
  std::ifstream file(configFile);
  ini.parse(file);
  ini.interpolate();

  inipp::get_value(ini.sections["interface"], "lang", Data.Interface.Lang);
  inipp::get_value(ini.sections["interface"], "theme", Data.Interface.Theme);
  inipp::get_value(ini.sections["interface"], "scale", Data.Interface.Scale);
  inipp::get_value(ini.sections["interface"], "docking", Data.Interface.Docking);
  inipp::get_value(ini.sections["interface"], "viewports", Data.Interface.Viewports);
  inipp::get_value(ini.sections["font"], "path", Data.Font.Path);
  inipp::get_value(ini.sections["font"], "size", Data.Font.Size);
  inipp::get_value(ini.sections["font"], "glyph-range", Data.Font.GlyphRange);
  inipp::get_value(ini.sections["mpv"], "config", Data.Mpv.UseConfig);
  inipp::get_value(ini.sections["mpv"], "wid", Data.Mpv.UseWid);
  inipp::get_value(ini.sections["mpv"], "watch-later", Data.Mpv.WatchLater);
  inipp::get_value(ini.sections["mpv"], "volume", Data.Mpv.Volume);
  inipp::get_value(ini.sections["window"], "save", Data.Window.Save);
  inipp::get_value(ini.sections["window"], "x", Data.Window.X);
  inipp::get_value(ini.sections["window"], "y", Data.Window.Y);
  inipp::get_value(ini.sections["window"], "w", Data.Window.W);
  inipp::get_value(ini.sections["window"], "h", Data.Window.H);
  inipp::get_value(ini.sections["debug"], "log-level", Data.Debug.LogLevel);
  inipp::get_value(ini.sections["debug"], "log-limit", Data.Debug.LogLimit);
}

void Config::save() {
  ini.sections["interface"]["lang"] = Data.Interface.Lang;
  ini.sections["interface"]["theme"] = Data.Interface.Theme;
  ini.sections["interface"]["scale"] = format("{}", Data.Interface.Scale);
  ini.sections["interface"]["docking"] = format("{}", Data.Interface.Docking);
  ini.sections["interface"]["viewports"] = format("{}", Data.Interface.Viewports);
  ini.sections["font"]["path"] = Data.Font.Path;
  ini.sections["font"]["size"] = std::to_string(Data.Font.Size);
  ini.sections["font"]["glyph-range"] = std::to_string(Data.Font.GlyphRange);
  ini.sections["mpv"]["config"] = format("{}", Data.Mpv.UseConfig);
  ini.sections["mpv"]["wid"] = format("{}", Data.Mpv.UseWid);
  ini.sections["mpv"]["watch-later"] = format("{}", Data.Mpv.WatchLater);
  ini.sections["mpv"]["volume"] = std::to_string(Data.Mpv.Volume);
  ini.sections["window"]["save"] = format("{}", Data.Window.Save);
  ini.sections["window"]["x"] = std::to_string(Data.Window.X);
  ini.sections["window"]["y"] = std::to_string(Data.Window.Y);
  ini.sections["window"]["w"] = std::to_string(Data.Window.W);
  ini.sections["window"]["h"] = std::to_string(Data.Window.H);
  ini.sections["debug"]["log-level"] = Data.Debug.LogLevel;
  ini.sections["debug"]["log-limit"] = std::to_string(Data.Debug.LogLimit);

  std::ofstream file(configFile);
  ini.generate(file);
}

const ImWchar* Config::buildGlyphRanges() {
  ImFontAtlas* fonts = ImGui::GetIO().Fonts;
  ImFontGlyphRangesBuilder glyphRangesBuilder;
  static ImVector<ImWchar> glyphRanges;
  if (Data.Font.GlyphRange & GlyphRange_Default) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesDefault());
  if (Data.Font.GlyphRange & GlyphRange_Chinese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesChineseFull());
  if (Data.Font.GlyphRange & GlyphRange_Japanese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesJapanese());
  if (Data.Font.GlyphRange & GlyphRange_Cyrillic) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesCyrillic());
  if (Data.Font.GlyphRange & GlyphRange_Korean) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesKorean());
  if (Data.Font.GlyphRange & GlyphRange_Thai) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesThai());
  if (Data.Font.GlyphRange & GlyphRange_Vietnamese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesVietnamese());
  glyphRangesBuilder.BuildRanges(&glyphRanges);
  return &glyphRanges[0];
}
}  // namespace ImPlay