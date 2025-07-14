// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <mpv/client.h>
#include <mpv/render_gl.h>

namespace ImPlay {
typedef void *(*GLAddrLoadFunc)(const char *name);
class Mpv {
 public:
  Mpv();
  ~Mpv();

  using EventHandler = std::function<void(void *)>;
  using LogHandler = std::function<void(const char *, const char *, const char *)>;
  using Callback = std::function<void(Mpv *)>;

  void init(GLAddrLoadFunc load, int64_t wid = 0);
  void render(int w, int h, int fbo = 0, bool flip = true);
  bool wantRender();
  void reportSwap();
  void waitEvent(double timeout = 0);
  void requestLog(const char *level, LogHandler handler);
  int loadConfig(const char *path);

  bool playing() { return playlistPlayingPos != -1; }
  bool allowDrag() { return windowDragging && !fullscreen; }

  Callback &wakeupCb() { return wakeupCb_; }
  Callback &updateCb() { return updateCb_; }

  inline int command(std::string args) { return mpv_command_string(mpv, args.c_str()); }
  inline int command(const char *args) { return mpv_command_string(mpv, args); }
  inline int command(const char *args[]) { return mpv_command_async(mpv, 0, args); }
  int commandv(const char *arg, ...);

  std::string property(const char *name) {
    char *data = mpv_get_property_string(mpv, name);
    std::string ret = data ? data : "";
    mpv_free(data);
    return ret;
  }
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
  template <typename T, mpv_format format>
  void observeProperty(const std::string &name, const std::function<void(T data)> &handler) {
    propertyEvents.emplace_back(name, format, [=](void *data) { handler(*(T *)data); });
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
    std::filesystem::path path;

    inline std::string filename() const { return path.filename().string(); }
  };

  struct ChapterItem {
    int64_t id = -1;
    std::string title;
    double time;
  };

  struct BindingItem {
    std::string section;
    std::string key;
    std::string cmd;
    std::string comment;
    int64_t priority;
    bool weak;
  };

  struct AudioDevice {
    std::string name;
    std::string description;
  };

  // cached mpv properties
  std::vector<PlayItem> playlist;
  std::vector<ChapterItem> chapters;
  std::vector<TrackItem> tracks;
  std::vector<AudioDevice> audioDevices;
  std::vector<BindingItem> bindings;
  std::vector<std::string> profiles;
  std::string aid, vid, sid, sid2, audioDevice, cursorAutohide;
  int64_t chapter, volume, playlistPos, playlistPlayingPos, timePos;
  int64_t brightness, contrast, saturation, gamma, hue;
  double audioDelay, subDelay, subScale;
  bool pause, mute, fullscreen, sidv, sidv2, forceWindow, ontop;
  bool keepaspect, keepaspectWindow, windowDragging, autoResize;

 private:
  void eventLoop();

  void observeProperties();
  void initPlaylist(mpv_node &node);
  void initChapters(mpv_node &node);
  void initTracks(mpv_node &node);
  void initAudioDevices(mpv_node &node);
  void initBindings(mpv_node &node);
  void initProfiles(const char *payload);

  int64_t wid = 0;
  mpv_handle *main = nullptr;
  mpv_handle *mpv = nullptr;
  mpv_render_context *renderCtx = nullptr;
  LogHandler logHandler = nullptr;
  Callback wakeupCb_, updateCb_;

  std::vector<std::tuple<mpv_event_id, EventHandler>> events;
  std::vector<std::tuple<std::string, mpv_format, EventHandler>> propertyEvents;
};
}  // namespace ImPlay