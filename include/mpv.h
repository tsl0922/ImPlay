#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <string>
#include <vector>
#include <functional>

using EventHandler = std::function<void(void *)>;

namespace ImPlay {
class Mpv {
 public:
  Mpv();
  ~Mpv();

  void render(int w, int h);
  void pollEvent();

  int command(const char *args) { return mpv_command_string(mpv, args); }
  int command(const char **args) { return mpv_command(mpv, args); }
  int property(const char *name, mpv_format format, void *data) { return mpv_get_property(mpv, name, format, data); }

  void observeEvent(mpv_event_id event, const EventHandler &handler) { events.emplace_back(event, handler); }
  void observeProperty(const std::string &name, mpv_format format, const EventHandler &handler) {
    propertyEvents.emplace_back(name, format, handler);
    mpv_observe_property(mpv, 0, name.c_str(), format);
  }

 private:
  void init();
  void exit();

  mpv_handle *mpv = nullptr;
  mpv_render_context *renderCtx = nullptr;

  std::vector<std::tuple<mpv_event_id, EventHandler>> events;
  std::vector<std::tuple<std::string, mpv_format, EventHandler>> propertyEvents;
};
}  // namespace ImPlay