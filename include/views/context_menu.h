// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include "view.h"

namespace ImPlay::Views {
class ContextMenu : public View {
 public:
  ContextMenu(Config *config, Mpv *mpv);

  void draw() override;

 private:
  void drawAudioDeviceList();
  void drawTracklist(const char *type, const char *prop);
  void drawChapterlist(std::vector<Mpv::ChapterItem> items);
  void drawPlaylist(std::vector<Mpv::PlayItem> items);
  void drawProfilelist();
};
}  // namespace ImPlay::Views