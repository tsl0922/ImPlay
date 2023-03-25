// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <functional>
#include <string>
#include <vector>
#include "view.h"

namespace ImPlay::Views {
class ContextMenu : public View {
 public:
  using View::View;

  void draw() override;

 private:
  enum ItemType { TYPE_NORMAL, TYPE_SEPARATOR, TYPE_SUBMENU, TYPE_CALLBACK };
  struct MenuItem {
    ItemType type;
    std::string cmd;
    std::string label;
    std::string icon;
    std::string shortcut;
    bool enabled = true;
    bool selected = false;
    std::vector<MenuItem> submenu;
    std::function<void()> callback;
  };

  void draw(std::vector<MenuItem> items);
  std::vector<MenuItem> build();

  void drawPlaylist(std::vector<Mpv::PlayItem> items);
  void drawChapterlist(std::vector<Mpv::ChapterItem> items);
  void drawTracklist(const char *type, const char *prop, std::string pos);
  void drawAudioDeviceList();
  void drawThemelist();
  void drawProfilelist();
  void drawRecentFiles();
};
}  // namespace ImPlay::Views