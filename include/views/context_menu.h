// Copyright (c) 2022-2023 tsl0922. All rights reserved.
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
  enum ItemType { TYPE_NORMAL, TYPE_SEPARATOR, TYPE_SUBMENU, TYPE_CALLBACK };
  struct Item {
    ItemType type;
    std::string cmd;
    std::string label;
    std::string icon;
    std::string shortcut;
    bool enabled = true;
    bool selected = false;
    std::vector<Item> submenu;
    std::function<void()> callback;
  };

  void init();
  void draw() override;
  std::vector<Item> build();
  void update(std::vector<ContextMenu::Item> &items, mpv_node &node);

 private:
  void draw(std::vector<Item> items);

  void drawPlaylist(std::vector<Mpv::PlayItem> items);
  void drawChapterlist(std::vector<Mpv::ChapterItem> items);
  void drawTracklist(const char *type, const char *prop, std::string pos);
  void drawAudioDeviceList();
  void drawThemelist();
  void drawProfilelist();
  void drawRecentFiles();

  std::vector<Item> items;
};
}  // namespace ImPlay::Views