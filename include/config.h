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
  int glyphRange = GlyphRange_Default;
  
  // mpv
  bool mpvConfig = false;
  bool mpvWid = false;
  bool watchLater = false;
  
  // window
  bool winSave = false;
  int winX = 0;
  int winY = 0;
  int winW = 0;
  int winH = 0;

  // log
  std::string logLevel = "no";
  int logLimit = 500;

 private:
  inipp::Ini<char> ini;
  std::string configFile;
};
}  // namespace ImPlay