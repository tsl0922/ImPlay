#pragma once
#include "view.h"
#include "../config.h"
#include "../mpv.h"

namespace ImPlay::Views {
class ContextMenu : public View {
 public:
  ContextMenu(Config *config, Mpv *mpv);

  void draw() override;

 private:
  void drawAudioDeviceList();
  void drawTracklist(const char *type, const char *prop);
  void drawChapterlist();
  void drawPlaylist();
  void drawProfilelist();

  Config *config;
  Mpv *mpv;
};
}  // namespace ImPlay::Views