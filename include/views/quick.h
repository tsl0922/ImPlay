#pragma once
#include <functional>
#include <vector>
#include "view.h"
#include "../config.h"
#include "../mpv.h"

namespace ImPlay::Views {
class Quick : public View {
 public:
  Quick(Config *config, Mpv *mpv);

  void draw() override;

  void drawTracks(const char *type, const char *prop);

  void drawPlaylistTabContent();
  void drawChaptersTabContent();
  void drawVideoTabContent();
  void drawAudioTabContent();
  void drawSubtitleTabContent();

  void setTab(const char *tab) {
    curTab = tab;
    tabSwitched = false;
  }

 private:
  struct Tab {
    const char *name;
    std::function<void()> draw;
    bool child;
  };

  void iconButton(const char *icon, const char* cmd, const char *tooltip = nullptr, bool sameline = true);
  void addTab(const char *name, std::function<void()> draw, bool child = false) { tabs.push_back({name, draw, child}); }

  Config *config;
  Mpv *mpv;
  bool tabSwitched = false;
  std::string curTab = "Video";
  std::vector<Tab> tabs;
};
}  // namespace ImPlay::Views