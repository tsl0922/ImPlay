// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <codecvt>
#include <locale>
#include <algorithm>
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <utility>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.hpp>

namespace ImPlay {
struct OptionParser {
  std::map<std::string, std::string> options;
  std::vector<std::string> paths;

  void parse(int argc, char** argv);
  bool check(std::string key, std::string value);
};

inline float scaled(float n) { return n * ImGui::GetFontSize(); }
inline ImVec2 scaled(const ImVec2& vector) { return vector * ImGui::GetFontSize(); }

void openFile(std::vector<std::pair<std::string, std::string>> filters, std::function<void(std::string)> callback);
void openFiles(std::vector<std::pair<std::string, std::string>> filters,
               std::function<void(std::string, int)> callback);
void openFolder(std::function<void(std::string)> callback);

int openUrl(std::string url);
void revealInFolder(std::string path);

std::string datadir(std::string subdir = "implay");
}  // namespace ImPlay

namespace ImGui {
void HalignCenter(const char* text);
void TextCentered(const char* text, bool disabled = false);
void TextEllipsis(const char* text, float maxWidth = 0);
void Hyperlink(const char* label, const char* url);
void HelpMarker(const char* desc);
ImTextureID LoadTexture(const char* path, int* width = nullptr, int* height = nullptr);
void SetTheme(const char* theme);
void StyleColorsSpectrum(ImGuiStyle* dst);
void StyleColorsDracula(ImGuiStyle* dst);
}  // namespace ImGui

namespace {
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
}  // namespace