#pragma once
#include "view.h"
#include "mpv.h"

namespace ImPlay::Views {
class Debug : public View {
 public:
  explicit Debug(Mpv *mpv);

  void draw() override;

 private:

  void drawVersion();
  void drawBindings();
  void drawProperties();

  Mpv *mpv = nullptr;
};
}  // namespace ImPlay::Views