// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <map>
#include <imgui.h>
#include <inipp.h>

namespace ImPlay {
struct ConfigData {
  struct Interface_ {
    std::string Theme = "light";
    float Scale = 0;
    bool operator==(const Interface_&) const = default;
  } Interface;
  struct Mpv_ {
    bool UseConfig = false;
    bool UseWid = false;
    bool WatchLater = false;
    bool operator==(const Mpv_&) const = default;
  } Mpv;
  struct Window_ {
    bool Save = false;
    int X = 0, Y = 0;
    int W = 0, H = 0;
    bool operator==(const Window_&) const = default;
  } Window;
  struct Font_ {
    bool Reload = false;
    std::string Path;
    int Size = 13;
    int GlyphRange = 0;
    bool operator==(const Font_&) const = default;
  } Font;
  struct Debug_ {
    std::string LogLevel = "no";
    int LogLimit = 500;
    bool operator==(const Debug_&) const = default;
  } Debug;
  bool operator==(const ConfigData&) const = default;
};

class Config {
 public:
  Config();
  ~Config() = default;

  enum GlyphRange_ {
    GlyphRange_Default = 0,
    GlyphRange_Chinese = 1 << 0,
    GlyphRange_Cyrillic = 1 << 1,
    GlyphRange_Japanese = 1 << 2,
    GlyphRange_Korean = 1 << 3,
    GlyphRange_Thai = 1 << 4,
    GlyphRange_Vietnamese = 1 << 5,
  };

  void load();
  void save();

  const ImWchar* buildGlyphRanges();

  ConfigData Data;

 private:
  inipp::Ini<char> ini;
  std::string configFile;
};
}  // namespace ImPlay