#include <iostream>
#include <cstring>
#include <stdexcept>
#include <fmt/printf.h>
#include <fmt/color.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "window.h"

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

static int run_headless(ImPlay::Mpv::OptionParser parser) {
  mpv_handle* ctx = mpv_create();
  if (!ctx) throw std::runtime_error("could not create mpv handle");

  for (const auto& option : parser.options) {
    auto key = option.first;
    auto value = option.second;
    if (int err = mpv_set_option_string(ctx, key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return 1;
    }
  }
  if (mpv_initialize(ctx) < 0) throw std::runtime_error("could not initialize mpv context");

  for (auto& path : parser.paths) {
    const char* cmd[] = {"loadfile", path.c_str(), "append-play", NULL};
    mpv_command(ctx, cmd);
  }

  while (ctx) {
    mpv_event* event = mpv_wait_event(ctx, -1);
    if (event->event_id == MPV_EVENT_SHUTDOWN) break;
  }

  mpv_terminate_destroy(ctx);

  return 0;
}

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

  ImPlay::Mpv::OptionParser parser;
  parser.parse(argc, argv);
  if (parser.options.contains("help")) {
    std::cout << usage << std::endl;
    return 0;
  }

  if (parser.options.contains("o")) return run_headless(parser);

  auto window = new ImPlay::Window("ImPlay", 1280, 720);
  int ret = window->run(parser) ? 0 : 1;

  delete window;

  return ret;
}