// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <functional>
#include <vector>
#include "view.h"
#include "config.h"
#include "mpv.h"

namespace ImPlay::Views {
class Settings : public View {
 public:
  Settings(Config *config, Mpv *mpv);

  void show() override;
  void draw() override;
  void drawButtons();
  void drawGeneralTab();
  void drawInterfaceTab();
  void drawFontTab();

 private:
  Config *config;
  ConfigData data;
  Mpv *mpv;
  std::vector<std::function<void()>> appliers;
};
}  // namespace ImPlay::Views