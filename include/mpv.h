#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <string>
#include <vector>
#include <functional>

namespace ImPlay {
class Mpv {
 public:
  explicit Mpv(int64_t wid);
  Mpv();
  ~Mpv();

  using EventHandler = std::function<void(void *)>;

  struct OptionParser {
    std::vector<std::pair<std::string, std::string>> options;
    std::vector<std::string> paths;

    void parse(int argc, char **argv);
  };

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
  };

  struct ChapterItem {
    int64_t id;
    char *title;
    double time;
  };

  struct BindingItem {
    char *key;
    char *cmd;
    char *comment;
  };

  void init();
  void render(int w, int h);
  bool wantRender();
  void waitEvent(double timeout = 0);

  std::vector<TrackItem> trackList(const char *type);
  std::vector<PlayItem> playlist();
  std::vector<ChapterItem> chapterList();
  std::vector<BindingItem> bindingList();
  std::vector<std::string> profileList();

  int command(const char *args) { return mpv_command_string(mpv, args); }
  int command(const char *args[]) { return mpv_command(mpv, args); }
  int commandv(const char *arg, ...);

  char *property(const char *name) { return mpv_get_property_string(mpv, name); }
  int property(const char *name, const char *data) { return mpv_set_property_string(mpv, name, data); }
  template <typename T, mpv_format format>
  T property(const char *name) {
    T data{0};
    mpv_get_property(mpv, name, format, &data);
    return data;
  }
  template <typename T, mpv_format format>
  int property(const char *name, T &data) {
    return mpv_set_property(mpv, name, format, static_cast<void *>(&data));
  }

  int option(const char *name, const char *data) { return mpv_set_option_string(mpv, name, data); }
  template <typename T, mpv_format format>
  int option(const char *name, T &data) {
    return mpv_set_option(mpv, name, format, static_cast<void *>(&data));
  }

  void observeEvent(mpv_event_id event, const EventHandler &handler) { events.emplace_back(event, handler); }
  void observeProperty(const std::string &name, mpv_format format, const EventHandler &handler) {
    propertyEvents.emplace_back(name, format, handler);
    mpv_observe_property(mpv, 0, name.c_str(), format);
  }

 private:
  void initRender();

  int64_t wid = 0;
  mpv_handle *mpv = nullptr;
  mpv_render_context *renderCtx = nullptr;

  std::vector<std::tuple<mpv_event_id, EventHandler>> events;
  std::vector<std::tuple<std::string, mpv_format, EventHandler>> propertyEvents;
};
}  // namespace ImPlay