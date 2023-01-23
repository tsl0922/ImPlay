// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include "config.h"
#include "helpers.h"

namespace ImPlay {
Config::Config() {
  const char* dataDir = datadir();
  if (dataDir[0] != '\0') {
    std::filesystem::create_directories(dataDir);
    configFile = format("{}/implay.conf", dataDir);
  } else
    configFile = "implay.conf";
}

void Config::load() {
  std::ifstream file(configFile);
  ini.parse(file);
  ini.interpolate();

  inipp::get_value(ini.sections["interface"], "theme", Theme);
  inipp::get_value(ini.sections["interface"], "scale", Scale);
  inipp::get_value(ini.sections["font"], "path", FontPath);
  inipp::get_value(ini.sections["font"], "size", FontSize);
  inipp::get_value(ini.sections["font"], "glyph-range", GlyphRange);
  inipp::get_value(ini.sections["mpv"], "config", UseConfig);
  inipp::get_value(ini.sections["mpv"], "wid", UseWid);
  inipp::get_value(ini.sections["mpv"], "watch-later", WatchLater);
  inipp::get_value(ini.sections["window"], "save", WinSave);
  inipp::get_value(ini.sections["window"], "x", WinX);
  inipp::get_value(ini.sections["window"], "y", WinY);
  inipp::get_value(ini.sections["window"], "w", WinW);
  inipp::get_value(ini.sections["window"], "h", WinH);
  inipp::get_value(ini.sections["debug"], "log-level", LogLevel);
  inipp::get_value(ini.sections["debug"], "log-limit", LogLimit);
}

void Config::save() {
  ini.sections["interface"]["theme"] = Theme;
  ini.sections["interface"]["scale"] = format("{}", Scale);
  ini.sections["font"]["path"] = FontPath;
  ini.sections["font"]["size"] = std::to_string(FontSize);
  ini.sections["font"]["glyph-range"] = std::to_string(GlyphRange);
  ini.sections["mpv"]["config"] = format("{}", UseConfig);
  ini.sections["mpv"]["wid"] = format("{}", UseWid);
  ini.sections["mpv"]["watch-later"] = format("{}", WatchLater);
  ini.sections["window"]["save"] = format("{}", WinSave);
  ini.sections["window"]["x"] = std::to_string(WinX);
  ini.sections["window"]["y"] = std::to_string(WinY);
  ini.sections["window"]["w"] = std::to_string(WinW);
  ini.sections["window"]["h"] = std::to_string(WinH);
  ini.sections["debug"]["log-level"] = LogLevel;
  ini.sections["debug"]["log-limit"] = std::to_string(LogLimit);

  std::ofstream file(configFile);
  ini.generate(file);
}

const ImWchar* Config::buildGlyphRanges() {
  ImFontAtlas* fonts = ImGui::GetIO().Fonts;
  ImFontGlyphRangesBuilder glyphRangesBuilder;
  static ImVector<ImWchar> glyphRanges;
  if (GlyphRange & GlyphRange_Default) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesDefault());
  if (GlyphRange & GlyphRange_Chinese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesChineseFull());
  if (GlyphRange & GlyphRange_Japanese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesJapanese());
  if (GlyphRange & GlyphRange_Cyrillic) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesCyrillic());
  if (GlyphRange & GlyphRange_Korean) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesKorean());
  if (GlyphRange & GlyphRange_Thai) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesThai());
  if (GlyphRange & GlyphRange_Vietnamese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesVietnamese());
  glyphRangesBuilder.BuildRanges(&glyphRanges);
  return &glyphRanges[0];
}
}  // namespace ImPlay