// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <map>
#include <vector>
#include <string>
#include <imgui.h>
#include <inipp.h>

namespace ImPlay {
struct ConfigData {
  struct Interface_ {
    std::string Lang = "en-US";
    std::string Theme = "light";
    float Scale = 0;
    int Fps = 30;
    bool Docking = false;
    bool Viewports = false;
    bool Rounding = true;
    bool Shadow = true;
    bool operator==(const Interface_&) const = default;
  } Interface;
  struct Mpv_ {
    bool UseConfig = false;
    bool UseWid = false;
    bool WatchLater = false;
    int Volume = 100;
    bool operator==(const Mpv_&) const = default;
  } Mpv;
  struct Window_ {
    bool Save = false;
    bool Single = false;
    int X = 0, Y = 0;
    int W = 0, H = 0;
    bool operator==(const Window_&) const = default;
  } Window;
  struct Font_ {
    std::string Path;
    int Size = 13;
    int GlyphRange = 0;
    bool operator==(const Font_&) const = default;
  } Font;
  struct Debug_ {
    std::string LogLevel = "status";
    int LogLimit = 500;
    bool operator==(const Debug_&) const = default;
  } Debug;
  struct Recent_ {
    int Limit = 10;
    bool SpaceToPlayLast = false;
    bool operator==(const Recent_&) const = default;
  } Recent;
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

  struct RecentItem {
    std::string path;
    std::string title;
    bool operator==(const RecentItem&) const = default;
  };

  void load();
  void save();

  std::string dir() const { return configDir; }
  std::string ipcSocket();
  std::vector<RecentItem> &getRecentFiles();
  void addRecentFile(const std::string& path, const std::string& title);
  void clearRecentFiles();

  const ImWchar* buildGlyphRanges();

  ConfigData Data;
  bool FontReload = false;

 private:
  inipp::Ini<char> ini;
  std::string configFile;
  std::string configDir;

  std::vector<RecentItem> recentFiles;
};
}  // namespace ImPlay