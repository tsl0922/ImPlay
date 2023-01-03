#pragma once
#include "view.h"

namespace ImPlay::Views {
class About : public View {
 public:
  void draw() override;

 private:
  void textCentered(const char* text);

  bool m_demo = false;
};
}  // namespace ImPlay::Views