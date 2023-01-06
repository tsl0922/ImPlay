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
  void drawProperties(const char *title, const char *key);
  void drawPropNode(const char *name, mpv_node& node, int depth = 0);

  Mpv *mpv = nullptr;
};
}  // namespace ImPlay::Views