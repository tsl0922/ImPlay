// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <fstream>
#include <romfs/romfs.hpp>
#include <nlohmann/json.hpp>
#include "helpers/utils.h"
#include "helpers/lang.h"

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

static std::pair<LangData, bool> parseLang(nlohmann::json& j) {
  LangData lang;
  const auto& code = j["code"];
  const auto& title = j["title"];
  const auto& fonts = j["fonts"];
  const auto& entries = j["entries"];
  if (!code.is_string() && !title.is_string() && !entries.is_object()) return {lang, false};
  if (j.contains("fallback")) {
    const auto& fallback = j["fallback"];
    if (fallback.is_boolean() && fallback.get<bool>()) getLangFallback() = code.get<std::string>();
  }
  lang.code = code.get<std::string>();
  lang.title = title.get<std::string>();
  if (fonts.is_array()) {
    for (auto& [key, value] : fonts.items()) {
      auto& path = value["path"];
      if (!path.is_string()) continue;
      LangFont font{path.get<std::string>()};
      auto& size = value["size"];
      auto& glyph_range = value["glyph-range"];
      if (size.is_number_integer()) font.size = size.get<int>();
      if (glyph_range.is_number_integer()) font.glyph_range = glyph_range.get<int>();
      lang.fonts.emplace_back(font);
    }
  }
  for (auto& [key, value] : entries.items()) {
    if (key == "" || !value.is_string()) continue;
    lang.entries[key] = value.get<std::string>();
  }
  return {lang, true};
}

std::map<std::string, LangData>& getLangs() {
  static std::map<std::string, LangData> langs;
  static bool loaded = false;
  if (loaded) return langs;

  auto langDir = dataPath() / "lang";
  if (std::filesystem::exists(langDir)) {
    for (auto& entry : std::filesystem::directory_iterator(langDir)) {
      if (entry.is_directory() || entry.path().extension() != ".json") continue;
      std::ifstream f(entry.path());
      auto j = nlohmann::json::parse(f);
      if (auto [lang, ok] = parseLang(j); ok) langs.insert({lang.code, lang});
    }
  }

  for (auto& path : romfs::list("lang")) {
    auto file = romfs::get(path);
    auto j = nlohmann::json::parse(file.data(), file.data() + file.size());
    if (auto [lang, ok] = parseLang(j); ok) langs.insert({lang.code, lang});
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
      for (auto& [key, value] : it->second.entries) {
        if (value != "") entries.insert({key, value});
      }
    }
    it = langs.find(getLangFallback());
    if (it != langs.end()) {
      for (auto& [key, value] : it->second.entries) {
        if (value != "") entries.insert({key, value});
      }
    }
    lang = getLang();
  }
  auto it = entries.find(key);
  if (it != entries.end()) return it->second;
  return key;
}
}  // namespace ImPlay