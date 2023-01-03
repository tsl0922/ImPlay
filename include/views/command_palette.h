#pragma once
#include <vector>
#include "view.h"
#include "../mpv.h"

namespace ImPlay::Views {
class CommandPalette : public View {
 public:
  explicit CommandPalette(Mpv *mpv);

  void draw() override;

 private:
  struct CommandMatch {
    std::string key;
    std::string command;
    std::string comment;
    std::string input;
    int score;
  };

  void drawInput();
  void drawList(float width);
  void match(const std::string &input);

  Mpv *mpv = nullptr;
  std::vector<char> buffer = std::vector<char>(1024, 0x00);
  std::vector<CommandMatch> matches;
  bool filtered = false;
  bool focusInput = false;
  bool justOpened = false;
};
}  // namespace ImPlay::Views