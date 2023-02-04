#pragma once
#include <functional>
#include <vector>
#include "view.h"

namespace ImPlay::Views {
class Quickview : public View {
 public:
  Quickview(Config *config, Dispatch *dispatch, Mpv *mpv);

  void draw() override;

  void setTab(const char *tab) {
    curTab = tab;
    tabSwitched = false;
  }

 private:
  #define FREQ_COUNT 10
  struct AudioEqItem {
    const char *name;
    int values[FREQ_COUNT];
    std::string toFilter(const char *name, int channels = 2);
  };

  struct Tab {
    const char *name;
    std::function<void()> draw;
    bool child;
  };

  void drawTracks(const char *type, const char *prop);
  void drawPlaylistTabContent();
  void drawChaptersTabContent();
  void drawVideoTabContent();
  void drawAudioTabContent();
  void drawSubtitleTabContent();

  void drawAudioEq();
  void applyAudioEq();
  void toggleAudioEq();
  void selectAudioEq(int index);
  void setAudioEqValue(int freqIndex, float gain);
  void updateAudioEqChannels();

  void alignRight(const char* label);
  void iconButton(const char *icon, const char* cmd, const char *tooltip = nullptr, bool sameline = true);
  bool toggleButton(const char* label, bool toggle, const char *tooltip = nullptr, ImGuiCol col = ImGuiCol_Button);
  bool toggleButton(bool toggle, const char *tooltip = nullptr);
  void addTab(const char *name, std::function<void()> draw, bool child = false) { tabs.push_back({name, draw, child}); }

  bool tabSwitched = false;
  std::string curTab = "Video";
  std::vector<Tab> tabs;

  const char *audioEqFreqs[FREQ_COUNT] = {"31.25", "62.5", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
  std::vector<AudioEqItem> audioEqPresets = {
      {"Flat", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
      {"Classical", {0, 0, 0, 0, 0, 0, -41, -41, -41, -53}},
      {"Club", {0, 0, 47, 29, 29, 29, 17, 0, 0, 0}},
      {"Dance", {53, 41, 11, 0, 0, -29, -41, -41, 0, 0}},
      {"Full bass", {53, 53, 53, 29, 5, -23, -47, -59, -65, -65}},
      {"Full bass and treble", {41, 29, 0, -41, -23, 5, 47, 65, 71, 71}},
      {"Full treble", {-53, -53, -53, -23, 11, 65, 95, 95, 95, 95}},
      {"Headphones", {23, 65, 29, -17, -11, 5, 23, 53, 71, 83}},
      {"Large Hall", {59, 59, 29, 29, 0, -23, -23, -23, 0, 0}},
      {"Live", {-23, 0, 23, 29, 29, 29, 23, 11, 11, 11}},
      {"Party", {41, 41, 0, 0, 0, 0, 0, 0, 41, 41}},
      {"Pop", {-5, 23, 41, 47, 29, 0, -11, -11, -5, -5}},
      {"Reggae", {0, 0, 0, -29, 0, 35, 35, 0, 0, 0}},
      {"Rock", {47, 23, 29, -47, -17, 23, 47, 65, 65, 65}},
      {"Ska", {-11, -23, -23, 0, 23, 29, 47, 53, 65, 53}},
      {"Soft", {23, 5, 0, -11, 0, 23, 47, 53, 65, 71}},
      {"Soft rock", {23, 23, 11, 0, -23, -29, -17, 0, 11, 47}},
      {"Techno", {47, 29, 0, -29, -23, 0, 47, 53, 53, 47}},
      {"Custom", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  };
  int audioEqIndex = -1;
  int audioEqChannels = 2;
  bool audioEqEnabled = false;
};
}  // namespace ImPlay::Views