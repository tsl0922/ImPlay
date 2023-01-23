// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <map>
#include <imgui.h>
#include <inipp.h>

namespace ImPlay {
class Config {
 public:
  Config();
  ~Config() = default;

  void load();
  void save();

  const ImWchar* buildGlyphRanges();

  enum GlyphRange_ {
    GlyphRange_Default = 0,
    GlyphRange_Chinese = 1 << 0,
    GlyphRange_Cyrillic = 1 << 1,
    GlyphRange_Japanese = 1 << 2,
    GlyphRange_Korean = 1 << 3,
    GlyphRange_Thai = 1 << 4,
    GlyphRange_Vietnamese = 1 << 5,
  };

  // interface
  std::string Theme = "light";
  float Scale = 0;

  // font
  std::string FontPath;
  int FontSize = 13;
  int GlyphRange = GlyphRange_Default;

  // mpv
  bool UseConfig = false;
  bool UseWid = false;
  bool WatchLater = false;

  // window
  bool WinSave = false;
  int WinX = 0, WinY = 0;
  int WinW = 0, WinH = 0;

  // log
  std::string LogLevel = "no";
  int LogLimit = 500;

 private:
  inipp::Ini<char> ini;
  std::string configFile;
};
}  // namespace ImPlay