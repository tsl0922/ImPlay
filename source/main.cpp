#include "window.h"

int main(int, char**) {
  auto window = new mpv::Window;
  window->loop();
  delete window;
  return 0;
}