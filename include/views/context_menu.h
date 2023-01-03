#pragma once
#include "view.h"
#include "../mpv.h"

namespace ImPlay::Views {
class ContextMenu : public View {
 public:
  explicit ContextMenu(Mpv *mpv);

  void draw() override;

 private:
  enum class Theme { DARK, LIGHT, CLASSIC };

  void drawAudioDeviceList();
  void drawTracklist(const char *type, const char *prop);
  void drawChapterlist();
  void drawPlaylist();
  void drawProfilelist();

  void setTheme(Theme theme);

  Mpv *mpv = nullptr;
  Theme theme;
  bool metrics = false;
};
}  // namespace ImPlay::Views