// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <map>
#include <vector>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace ImGui {
void HelpMarker(const char* desc);
ImTextureID LoadTexture(const char* path, int* width = nullptr, int* height = nullptr);
}  // namespace ImGui

namespace ImPlay {
struct OptionParser {
  std::map<std::string, std::string> options;
  std::vector<std::string> paths;

  void parse(int argc, char** argv);
  bool check(std::string key, std::string value);
};

int openUri(const char* uri);
const char* datadir(const char* subdir = "implay");

inline std::string tolower(std::string s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

inline std::string toupper(std::string s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

inline std::string trim(std::string s) {
  std::string str = s;
  const char* ws = " \t\n\r\f\v";
  str.erase(0, str.find_first_not_of(ws));
  str.erase(str.find_last_not_of(ws) + 1);
  return str;
}

template <typename... T>
inline std::string format(std::string_view format, T... args) {
  return fmt::format(fmt::runtime(format), args...);
}

template <typename... T>
inline void print(std::string_view format, T... args) {
  fmt::print(fmt::runtime(format), args...);
}

inline std::string join(std::vector<std::string> v, std::string_view sep) {
  return fmt::format("{}", fmt::join(v, sep));
}
}  // namespace ImPlay
