#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include "config.h"
#include "helpers.h"

namespace ImPlay {
Config::Config() {
  const char* dataDir = Helpers::getDataDir();
  if (dataDir[0] != '\0') {
    std::filesystem::create_directories(dataDir);
    configFile = fmt::format("{}/implay.conf", dataDir);
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
}

void Config::save() {
  ini.sections["interface"]["theme"] = Theme;
  ini.sections["interface"]["scale"] = fmt::format("{}", Scale);
  ini.sections["font"]["path"] = FontPath;
  ini.sections["font"]["size"] = std::to_string(FontSize);
  ini.sections["font"]["glyph-range"] = std::to_string(glyphRange);

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