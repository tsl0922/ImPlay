#pragma once
#include <map>
#include <imgui.h>
#include <inipp.h>

namespace ImPlay {
class Config {
 public:
  Config();
  ~Config() = default;

  void load();
  void save();

  const ImWchar* buildGlyphRanges();

  enum GlyphRange_ {
    GlyphRange_Default = 0,
    GlyphRange_Chinese = 1 << 0,
    GlyphRange_Cyrillic = 1 << 1,
    GlyphRange_Japanese = 1 << 2,
    GlyphRange_Korean = 1 << 3,
    GlyphRange_Thai = 1 << 4,
    GlyphRange_Vietnamese = 1 << 5,
  };

  std::string Theme = "light";
  float Scale = 0;
  std::string FontPath;
  int FontSize = 13;
  int glyphRange = GlyphRange_Default;
  bool mpvConfig = false;

 private:
  inipp::Ini<char> ini;
  std::string configFile;
};
}  // namespace ImPlay