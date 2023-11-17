#pragma once
#include <functional>
#include <vector>
#include "view.h"

namespace ImPlay::Views {
class Quickview : public View {
 public:
  Quickview(Config *config, Mpv *mpv);

  void show(const char *tab = nullptr);
  void draw() override;

 private:
#define FREQ_COUNT 10
  struct AudioEqItem {
    std::string name;
    int values[FREQ_COUNT];
    std::string toFilter(const char *name, int channels = 2);
  };

  struct Tab {
    std::string name;
    std::string title;
    std::function<void()> draw;
  };

  void drawWindow();
  void drawPopup();
  void drawTabBar();
  void drawTracks(const char *title, const char *type, const char *prop, std::string pos);
  void drawTracks(const char *type, const char *prop, std::string pos);
  void drawPlaylistTabContent();
  void drawChaptersTabContent();
  void drawVideoTabContent();
  void drawAudioTabContent();
  void drawSubtitleTabContent();

  void drawAudioEq();
  void applyAudioEq(bool osd = true);
  void toggleAudioEq();
  void selectAudioEq(int index);
  void setAudioEqValue(int freqIndex, float gain);
  void updateAudioEqChannels();

  void alignRight(const char *label);
  bool iconButton(const char *icon, const char *cmd, const char *tooltip = nullptr, bool sameline = true);
  bool toggleButton(const char *label, bool toggle, const char *tooltip = nullptr, ImGuiCol col = ImGuiCol_Button);
  bool toggleButton(bool toggle, const char *tooltip = nullptr, const char *id = nullptr);
  void emptyLabel();
  void addTab(std::string name, std::string title, std::function<void()> draw) { tabs.push_back({name, title, draw}); }

  bool winMode = false;
  bool tabSwitched = false;
  std::string curTab = "Video";
  std::vector<Tab> tabs;

  const char *audioEqFreqs[FREQ_COUNT] = {"31.25", "62.5", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
  std::vector<AudioEqItem> audioEqPresets = {
      {"views.quickview.audio.equalizer.presets.flat", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
      {"views.quickview.audio.equalizer.presets.classical", {0, 0, 0, 0, 0, 0, -41, -41, -41, -53}},
      {"views.quickview.audio.equalizer.presets.club", {0, 0, 47, 29, 29, 29, 17, 0, 0, 0}},
      {"views.quickview.audio.equalizer.presets.dance", {53, 41, 11, 0, 0, -29, -41, -41, 0, 0}},
      {"views.quickview.audio.equalizer.presets.full_bass", {53, 53, 53, 29, 5, -23, -47, -59, -65, -65}},
      {"views.quickview.audio.equalizer.presets.full_bass_and_treble", {41, 29, 0, -41, -23, 5, 47, 65, 71, 71}},
      {"views.quickview.audio.equalizer.presets.full_treble", {-53, -53, -53, -23, 11, 65, 95, 95, 95, 95}},
      {"views.quickview.audio.equalizer.presets.headphones", {23, 65, 29, -17, -11, 5, 23, 53, 71, 83}},
      {"views.quickview.audio.equalizer.presets.large_hall", {59, 59, 29, 29, 0, -23, -23, -23, 0, 0}},
      {"views.quickview.audio.equalizer.presets.live", {-23, 0, 23, 29, 29, 29, 23, 11, 11, 11}},
      {"views.quickview.audio.equalizer.presets.party", {41, 41, 0, 0, 0, 0, 0, 0, 41, 41}},
      {"views.quickview.audio.equalizer.presets.pop", {-5, 23, 41, 47, 29, 0, -11, -11, -5, -5}},
      {"views.quickview.audio.equalizer.presets.reggae", {0, 0, 0, -29, 0, 35, 35, 0, 0, 0}},
      {"views.quickview.audio.equalizer.presets.rock", {47, 23, 29, -47, -17, 23, 47, 65, 65, 65}},
      {"views.quickview.audio.equalizer.presets.ska", {-11, -23, -23, 0, 23, 29, 47, 53, 65, 53}},
      {"views.quickview.audio.equalizer.presets.soft", {23, 5, 0, -11, 0, 23, 47, 53, 65, 71}},
      {"views.quickview.audio.equalizer.presets.soft_rock", {23, 23, 11, 0, -23, -29, -17, 0, 11, 47}},
      {"views.quickview.audio.equalizer.presets.techno", {47, 29, 0, -29, -23, 0, 47, 53, 53, 47}},
      {"views.quickview.audio.equalizer.custom", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
  };
  int audioEqIndex = -1;
  int audioEqChannels = 2;
  bool audioEqEnabled = false;
};
}  // namespace ImPlay::Views