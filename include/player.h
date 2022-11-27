#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <string>
#include <vector>
#include <functional>

using EventHandler = std::function<void(void *)>;

namespace mpv {
class Player {
 public:
  Player();
  ~Player();

  void command(std::string cmd);
  void command(const char **args);
  
  int property(const char* name, int &value) {
    return mpv_get_property(mpv, name, MPV_FORMAT_FLAG, &value);
  }

  int property(const char* name, int64_t &value) {
    return mpv_get_property(mpv, name, MPV_FORMAT_INT64, &value);
  }

  int property(const char* name, double &value) {
    return mpv_get_property(mpv, name, MPV_FORMAT_DOUBLE, &value);
  }

  int property(const char* name, char* &value) {
    return mpv_get_property(mpv, name, MPV_FORMAT_STRING, &value);
  }

  void render(int w, int h);
  void pollEvent();

  void observeEvent(mpv_event_id event, const EventHandler &handler) {
    events.emplace_back(event, handler);
  }

  void observeProperty(const std::string &name, mpv_format format,
                       const EventHandler &handler) {
    propertyEvents.emplace_back(name, format, handler);
    mpv_observe_property(mpv, 0, name.c_str(), format);
  }

 private:
  void initMpv();
  void exitMpv();

  mpv_handle *mpv = nullptr;
  mpv_render_context *renderCtx = nullptr;

  std::vector<std::tuple<mpv_event_id, EventHandler>> events;
  std::vector<std::tuple<std::string, mpv_format, EventHandler>> propertyEvents;
};
}