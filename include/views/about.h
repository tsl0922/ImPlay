#pragma once

namespace ImPlay::Views {
class About {
 public:
  About();
  ~About();

  void draw();
  void show();

 private:
  bool open = false;
};
}  // namespace ImPlay::Views