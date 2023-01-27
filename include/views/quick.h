#pragma once
#include "view.h"
#include "../config.h"
#include "../mpv.h"

namespace ImPlay::Views {
class Quick : public View {
 public:
  Quick(Config *config, Mpv *mpv);

  void draw() override;
  void drawPlaylist();
  void drawChapterList();
  void drawTracks(const char *type, const char *prop);
  void drawVideo();
  void drawAudio();
  void drawSubtitle();

 private:
  Config *config;
  Mpv *mpv;
};
}  // namespace ImPlay::Views