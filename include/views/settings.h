// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include "view.h"
#include "config.h"
#include "mpv.h"

namespace ImPlay::Views {
class Settings : public View {
 public:
  Settings(Config *config, Mpv *mpv);

  void draw() override;
  void drawGeneralTab();
  void drawInterfaceTab();
  void drawFontTab();

 private:
  Config *config;
  Mpv *mpv;
};
}  // namespace ImPlay::Views