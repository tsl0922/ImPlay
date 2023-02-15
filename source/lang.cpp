// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <romfs/romfs.hpp>
#include <nlohmann/json.hpp>
#include "lang.h"

namespace ImPlay {
std::string LangData::get(std::string& key) {
  auto it = entries.find(key);
  if (it != entries.end()) return it->second;
  return key;
}

LangStr::LangStr(std::string key) { m_str = i18n(key); }

LangStr::operator std::string() const { return m_str; }

LangStr::operator std::string_view() const { return m_str; }

LangStr::operator const char*() const { return m_str.c_str(); }

const ImWchar* getLangGlyphRanges() {
  static ImVector<ImWchar> glyphRanges;
  if (glyphRanges.empty()) {
    ImFontGlyphRangesBuilder builder;
    for (auto& [code, lang] : getLangs()) {
      builder.AddText(lang.title.c_str());
      for (auto& [key, value] : lang.entries) builder.AddText(value.c_str());
    }
    builder.BuildRanges(&glyphRanges);
  }
  return &glyphRanges[0];
}

std::map<std::string, LangData>& getLangs() {
  static std::map<std::string, LangData> langs;
  static bool loaded = false;
  if (loaded) return langs;

  for (auto& path : romfs::list("lang")) {
    auto file = romfs::get(path);
    auto j = nlohmann::json::parse(file.data(), file.data() + file.size());
    const auto& code = j["code"];
    const auto& title = j["title"];
    const auto& fonts = j["fonts"];
    const auto& entries = j["entries"];
    if (!code.is_string() && !title.is_string() && !entries.is_object()) continue;
    if (j.contains("fallback")) {
      const auto& fallback = j["fallback"];
      if (fallback.is_boolean() && fallback.get<bool>()) getLangFallback() = code.get<std::string>();
    }
    auto lang = LangData{code.get<std::string>(), title.get<std::string>()};
    if (fonts.is_array()) {
      for (auto& [key, value] : fonts.items()) {
        auto& path = value["path"];
        auto& size = value["size"];
        if (path.is_string() && size.is_number_integer())
          lang.fonts.push_back({path.get<std::string>(), size.get<int>()});
      }
    }
    for (auto& [key, value] : entries.items()) {
      if (!value.is_string()) continue;
      lang.entries[key] = value.get<std::string>();
    }
    langs.insert({lang.code, lang});
  }
  loaded = true;
  return langs;
}

std::string& getLangFallback() {
  static std::string fallback = "en-US";
  return fallback;
}

std::string& getLang() {
  static std::string lang = "en-US";
  return lang;
}

std::string i18n(std::string key) {
  static std::string lang;
  static std::map<std::string, std::string> entries;
  if (entries.empty() || lang != getLang()) {
    entries.clear();
    auto langs = getLangs();
    auto it = langs.find(getLang());
    if (it != langs.end()) {
      for (auto& [key, value] : it->second.entries) entries.insert({key, value});
    }
    it = langs.find(getLangFallback());
    if (it != langs.end()) {
      for (auto& [key, value] : it->second.entries) entries.insert({key, value});
    }
    lang = getLang();
  }
  auto it = entries.find(key);
  if (it != entries.end()) return it->second;
  return key;
}
}  // namespace ImPlay