#include "window.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
  char* console = getenv("_started_from_console");
  if (console != nullptr && strcmp(console, "yes") == 0) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
      freopen("CONIN$", "r", stdin);
      freopen("CONOUT$", "w", stdout);
      freopen("CONOUT$", "w", stderr);
    }
  }
#endif

  auto window = new ImPlay::Window("ImPlay", 1280, 720);
  int ret = 0;

  if (!window->run(argc, argv)) {
    std::cerr << "Failed to run ImPlay, check if the command line options are valid for mpv." << std::endl;
    ret = 1;
  }
  delete window;

  return ret;
}