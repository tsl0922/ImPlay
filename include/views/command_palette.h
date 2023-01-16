// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <functional>
#include <vector>
#include "view.h"
#include "../mpv.h"

namespace ImPlay::Views {
class CommandPalette : public View {
 public:
  struct CommandItem {
    std::string title;
    std::string tooltip;
    std::string label;
    std::function<void()> callback;
  };

  explicit CommandPalette(Mpv *mpv);

  void draw() override;

  std::vector<CommandItem> &items() { return items_; }

 private:
  void drawInput();
  void drawList(float width);
  void match(const std::string &input);

  Mpv *mpv = nullptr;
  std::vector<char> buffer = std::vector<char>(1024, 0x00);
  std::vector<CommandItem> items_;
  std::vector<CommandItem> matches;
  bool filtered = false;
  bool focusInput = false;
  bool justOpened = false;
};
}  // namespace ImPlay::Views