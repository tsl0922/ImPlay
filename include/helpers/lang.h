// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <map>
#include <imgui.h>

namespace ImPlay {
struct LangFont {
  std::string path;
  int size = 0;
  int glyph_range = 0;
};

struct LangData {
  std::string code;
  std::string title;
  std::vector<LangFont> fonts;
  std::map<std::string, std::string> entries;

  std::string get(std::string& key);
};

class LangStr {
 public:
  explicit LangStr(std::string key);

  operator std::string() const;
  operator std::string_view() const;
  operator const char*() const;

 private:
  std::string m_str;
};

inline std::string format_as(LangStr s) { return s; }

const ImWchar* getLangGlyphRanges();

std::map<std::string, LangData>& getLangs();
std::string& getLangFallback();
std::string& getLang();

std::string i18n(std::string key);
template <typename... T>
inline std::string i18n_a(std::string key, T... args) {
  return fmt::vformat(i18n(key), fmt::make_format_args(args...));
}
inline LangStr operator""_i18n(const char* key, size_t) { return LangStr(key); }
}  // namespace ImPlay