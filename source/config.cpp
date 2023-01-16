#include <filesystem>
#include <fstream>
#include "config.h"
#include "helpers.h"

namespace ImPlay {
Config::Config() {
  const char* dataDir = datadir();
  if (dataDir[0] != '\0') {
    std::filesystem::create_directories(dataDir);
    configFile = format("{}/implay.conf", dataDir);
  } else
    configFile = "implay.conf";
}

void Config::load() {
  std::ifstream file(configFile);
  ini.parse(file);
  ini.interpolate();

  inipp::get_value(ini.sections["interface"], "theme", Theme);
  inipp::get_value(ini.sections["interface"], "scale", Scale);
  inipp::get_value(ini.sections["font"], "path", FontPath);
  inipp::get_value(ini.sections["font"], "size", FontSize);
  inipp::get_value(ini.sections["font"], "glyph-range", glyphRange);
  inipp::get_value(ini.sections["mpv"], "config", mpvConfig);
  inipp::get_value(ini.sections["mpv"], "wid", mpvWid);
  inipp::get_value(ini.sections["mpv"], "watch-later", watchLater);
  inipp::get_value(ini.sections["debug"], "log-level", logLevel);
  inipp::get_value(ini.sections["debug"], "log-limit", logLimit);
}

void Config::save() {
  ini.sections["interface"]["theme"] = Theme;
  ini.sections["interface"]["scale"] = format("{}", Scale);
  ini.sections["font"]["path"] = FontPath;
  ini.sections["font"]["size"] = std::to_string(FontSize);
  ini.sections["font"]["glyph-range"] = std::to_string(glyphRange);
  ini.sections["mpv"]["config"] = format("{}", mpvConfig);
  ini.sections["mpv"]["wid"] = format("{}", mpvWid);
  ini.sections["mpv"]["watch-later"] = format("{}", watchLater);
  ini.sections["debug"]["log-level"] = logLevel;
  ini.sections["debug"]["log-limit"] = std::to_string(logLimit);

  std::ofstream file(configFile);
  ini.generate(file);
}

const ImWchar* Config::buildGlyphRanges() {
  ImFontAtlas* fonts = ImGui::GetIO().Fonts;
  ImFontGlyphRangesBuilder glyphRangesBuilder;
  static ImVector<ImWchar> glyphRanges;
  if (glyphRange & GlyphRange_Default) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesDefault());
  if (glyphRange & GlyphRange_Chinese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesChineseFull());
  if (glyphRange & GlyphRange_Japanese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesJapanese());
  if (glyphRange & GlyphRange_Cyrillic) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesCyrillic());
  if (glyphRange & GlyphRange_Korean) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesKorean());
  if (glyphRange & GlyphRange_Thai) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesThai());
  if (glyphRange & GlyphRange_Vietnamese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesVietnamese());
  glyphRangesBuilder.BuildRanges(&glyphRanges);
  return &glyphRanges[0];
}
}  // namespace ImPlay