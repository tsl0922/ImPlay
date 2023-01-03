#pragma once
#include <map>
#include <utility>
#include <functional>
#include "view.h"
#include "../mpv.h"

namespace ImPlay::Views {
class ContextMenu : public View {
 public:
  enum class Action {
    ABOUT,
    PALETTE,
  };

  explicit ContextMenu(Mpv *mpv);

  void draw() override;
  void setAction(Action action, std::function<void()> callback) { actionHandlers[action] = std::move(callback); }

 private:
  enum class Theme { DARK, LIGHT, CLASSIC };

  void drawAudioDeviceList();
  void drawTracklist(const char *type, const char *prop);
  void drawChapterlist();
  void drawPlaylist();
  void drawProfilelist();

  void action(Action action);
  void setTheme(Theme theme);

  Mpv *mpv = nullptr;
  Theme theme;
  bool metrics = false;

  std::map<Action, std::function<void()>> actionHandlers;
};
}  // namespace ImPlay::Views