#include "window.h"

int main(int, char**) {
  auto window = new ImPlay::Window();
  window->loop();
  delete window;
  return 0;
}