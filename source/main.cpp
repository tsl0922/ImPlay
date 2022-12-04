#include "window.h"

int main(int, char**) {
  auto window = new ImPlay::Window("ImPlay", 1280, 720);
  window->loop();
  delete window;
  return 0;
}