// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <functional>
#include <string>
#include <map>
#include <vector>
#include "view.h"

namespace ImPlay::Views {
class CommandPalette : public View {
 public:
  CommandPalette(Config *config, Mpv *mpv);

  struct CommandItem {
    std::string title;
    std::string tooltip;
    std::string label;
    int64_t id;
    std::function<void()> callback;
  };

  void show(int n, const char **args);
  void draw() override;

 private:
  void drawInput();
  void drawList(float width);
  void match(const std::string &input);

  std::map<std::string, std::function<void(const char*)>> providers;
  std::vector<char> buffer = std::vector<char>(1024, 0x00);
  std::vector<CommandItem> items;
  std::vector<CommandItem> matches;
  int64_t pos = -1;
  bool filtered = false;
  bool focusInput = false;
  bool justOpened = false;
};
}  // namespace ImPlay::Views