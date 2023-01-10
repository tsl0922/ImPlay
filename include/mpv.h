#pragma once
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace ImPlay {
class Mpv {
 public:
  Mpv(GLFWwindow *window, int64_t wid = 0);
  Mpv();
  ~Mpv();

  using EventHandler = std::function<void(void *)>;
  using LogHandler = std::function<void(const char *, const char *, const char *)>;

  void init();
  void render(int w, int h);
  bool wantRender();
  void wakeupLoop();
  void waitEvent(double timeout = 0);
  void requestLog(const char *level, LogHandler handler);
  int loadConfig(const char *path);
  bool paused();
  bool playing();

  std::atomic_bool &runLoop() { return runLoop_; }
  GLFWwindow *&win() { return window; }

  int command(std::string args) { return mpv_command_string(mpv, args.c_str()); }
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
  int property(const char *name, T data) {
    return mpv_set_property(mpv, name, format, static_cast<void *>(&data));
  }

  int option(const char *name, const char *data) { return mpv_set_option_string(mpv, name, data); }
  template <typename T, mpv_format format>
  int option(const char *name, T data) {
    return mpv_set_option(mpv, name, format, static_cast<void *>(&data));
  }

  void observeEvent(mpv_event_id event, const EventHandler &handler) { events.emplace_back(event, handler); }
  void observeProperty(const std::string &name, mpv_format format, const EventHandler &handler) {
    propertyEvents.emplace_back(name, format, handler);
    mpv_observe_property(mpv, 0, name.c_str(), format);
  }

  struct TrackItem {
    int64_t id = -1;
    std::string type;
    std::string title;
    std::string lang;
    bool selected;
  };

  struct PlayItem {
    int64_t id = -1;
    std::string title;
    std::string path;
    std::string filename;
  };

  struct ChapterItem {
    int64_t id = -1;
    std::string title;
    double time;
  };

  struct BindingItem {
    std::string key;
    std::string cmd;
    std::string comment;
  };

  struct AudioDevice {
    std::string name;
    std::string description;
  };

  std::vector<TrackItem> trackList();
  std::vector<PlayItem> playlist();
  std::vector<ChapterItem> chapterList();
  std::vector<BindingItem> bindingList();
  std::vector<std::string> profileList();
  std::vector<AudioDevice> audioDeviceList();

 private:
  void initRender();
  void eventLoop();
  void renderLoop();

  int64_t wid = 0;
  int width, height;
  GLFWwindow *window = nullptr;
  mpv_handle *main = nullptr;
  mpv_handle *mpv = nullptr;
  mpv_render_context *renderCtx = nullptr;
  LogHandler logHandler = nullptr;

  std::atomic_bool shutdown = false;
  std::atomic_bool runLoop_ = false;
  std::mutex mutex;
  std::condition_variable cond;
  std::vector<std::tuple<mpv_event_id, EventHandler>> events;
  std::vector<std::tuple<std::string, mpv_format, EventHandler>> propertyEvents;
};
}  // namespace ImPlay