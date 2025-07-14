// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <codecvt>
#include <locale>
#include <filesystem>
#include <string>
#include <map>
#include <vector>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "lang.h"

namespace ImPlay {
struct OptionParser {
  std::map<std::string, std::string> options;
  std::vector<std::string> paths;

  void parse(int argc, char** argv);
  bool check(std::string key, std::string value);
};

inline float scaled(float n) { return n * ImGui::GetFontSize(); }
inline ImVec2 scaled(const ImVec2& vector) { return vector * ImGui::GetFontSize(); }

bool fileExists(std::string path);

int openUrl(std::string url);
void revealInFolder(std::string path);

std::filesystem::path dataPath();

inline std::wstring UTF8ToWide(const std::string& str) {
  return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().from_bytes(str);
}
inline std::string WideToUTF8(const std::wstring& str) {
  return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(str);
}

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

inline bool iequals(std::string a, std::string b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                    [](char a, char b) { return ::tolower(a) == ::tolower(b); });
}

inline std::string trim(std::string s) {
  std::string str = s;
  const char* ws = " \t\n\r\f\v";
  str.erase(0, str.find_first_not_of(ws));
  str.erase(str.find_last_not_of(ws) + 1);
  return str;
}

inline std::string join(std::vector<std::string> v, std::string_view sep) {
  return fmt::format("{}", fmt::join(v, sep));
}

std::vector<std::string> split(const std::string& str, const std::string& sep);

bool findCase(std::string haystack, std::string needle);
}  // namespace ImPlay
