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

  struct TrackItem {
    int64_t id;
    char *type;
    char *title;
    char *lang;
    bool selected;
  };

  struct PlayItem {
    int64_t id;
    char *title;
    char *filename;
    bool current;
    bool playing;
  };

  void render(int w, int h);
  void pollEvent();

  std::vector<TrackItem> toTracklist(mpv_node *node);
  std::vector<PlayItem> toPlaylist(mpv_node *node);
  int command(const char *args) { return mpv_command_string(mpv, args); }
  int command(const char *args[]) { return mpv_command(mpv, args); }
  template <typename T>
  T property(const char *name, mpv_format format) {
    T data;
    mpv_get_property(mpv, name, format, &data);
    return data;
  }
  template <typename T>
  void property(const char *name, mpv_format format, T &data) {
    mpv_set_property(mpv, name, format, &data);
  }

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