#include "window.h"
#include <iostream>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif

static const char* usage =
    "Usage:   ImPlay [options] [url|path/]filename\n"
    "\n"
    "Basic options:\n"
    " --start=<time>    seek to given (percent, seconds, or hh:mm:ss) position\n"
    " --no-audio        do not play sound\n"
    " --no-video        do not play video\n"
    " --fs              fullscreen playback\n"
    " --sub-file=<file> specify subtitle file to use\n"
    " --playlist=<file> specify playlist file\n"
    "\n"
    "Visit https://mpv.io/manual/stable to get full mpv options.\n";

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
  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    std::cout << usage << std::endl;
    return 0;
  }

  auto window = new ImPlay::Window("ImPlay", 1280, 720);
  int ret = window->run(argc, argv) ? 0 : 1;

  delete window;

  return ret;
}