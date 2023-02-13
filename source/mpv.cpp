// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <stdexcept>
#include <string>
#include <thread>
#include <cstdarg>
#include <cstring>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>
#include "mpv.h"

namespace ImPlay {
Mpv::Mpv(int64_t wid) : wid(wid) {
  main = mpv_create();
  if (!main) throw std::runtime_error("could not create mpv handle");
  mpv = mpv_create_client(main, "implay");
  if (!mpv) throw std::runtime_error("could not create mpv client");
  if (mpv_set_property(mpv, "wid", MPV_FORMAT_INT64, &wid) < 0) throw std::runtime_error("could not set mpv wid");
}

Mpv::~Mpv() {
  if (renderCtx != nullptr) mpv_render_context_free(renderCtx);
  mpv_unobserve_property(mpv, 0);
  mpv_destroy(main);
  mpv_destroy(mpv);
}

int Mpv::commandv(const char *arg, ...) {
  std::vector<const char *> args;
  va_list ap;
  va_start(ap, arg);
  for (const char *s = arg; s != nullptr; s = va_arg(ap, const char *)) args.push_back(s);
  va_end(ap);
  args.push_back(nullptr);
  return mpv_command(mpv, args.data());
}

void Mpv::waitEvent(double timeout) {
  while (mpv) {
    mpv_event *event = mpv_wait_event(mpv, timeout);
    if (event->event_id == MPV_EVENT_NONE) break;
    switch (event->event_id) {
      case MPV_EVENT_PROPERTY_CHANGE: {
        auto *prop = (mpv_event_property *)event->data;
        for (const auto &[name, format, handler] : propertyEvents)
          if (name == prop->name && format == prop->format) handler(prop->data);
        break;
      }
      case MPV_EVENT_LOG_MESSAGE: {
        mpv_event_log_message *msg = (mpv_event_log_message *)event->data;
        if (logHandler) logHandler(msg->prefix, msg->level, msg->text);
      } break;
      default:
        for (const auto &[event_id, handler] : events)
          if (event_id == event->event_id) handler(event->data);
        break;
    }
  }
}

void Mpv::requestLog(const char *level, LogHandler handler) {
  this->logHandler = handler;
  mpv_request_log_messages(mpv, level);
}

int Mpv::loadConfig(const char *path) { return mpv_load_config_file(mpv, path); }

void Mpv::eventLoop() {
  while (main) {
    mpv_event *event = mpv_wait_event(main, -1);
    if (event->event_id == MPV_EVENT_SHUTDOWN) break;
  }
}

void Mpv::render(int w, int h) {
  if (renderCtx == nullptr) return;

  int flip_y{1};
  mpv_opengl_fbo mpfbo{0, w, h};
  mpv_render_param params[]{
      {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
      {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };
  mpv_render_context_render(renderCtx, params);
}

bool Mpv::wantRender() {
  return renderCtx != nullptr && (mpv_render_context_update(renderCtx) & MPV_RENDER_UPDATE_FRAME);
}

void Mpv::init() {
  if (mpv_initialize(mpv) < 0) throw std::runtime_error("could not initialize mpv context");
  if (wid == 0) initRender();

  mpv_request_log_messages(main, "no");

  mpv_set_wakeup_callback(
      mpv,
      [](void *ctx) {
        Mpv *mpv = static_cast<Mpv *>(ctx);
        if (mpv->wakeupCb_) mpv->wakeupCb_(mpv);
      },
      this);
  std::thread(&Mpv::eventLoop, this).detach();

  forceWindow = property<int, MPV_FORMAT_FLAG>("force-window");
  observeProperties();
}

void Mpv::initRender() {
  mpv_opengl_init_params gl_init_params{
      [](void *ctx, const char *name) { return (void *)glfwGetProcAddress(name); },
      nullptr,
  };
  mpv_render_param params[]{
      {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };

  if (mpv_render_context_create(&renderCtx, mpv, params) < 0)
    throw std::runtime_error("failed to initialize mpv GL context");

  mpv_render_context_set_update_callback(
      renderCtx,
      [](void *ctx) {
        Mpv *mpv = static_cast<Mpv *>(ctx);
        if (mpv->updateCb_) mpv->updateCb_(mpv);
      },
      this);
}

void Mpv::observeProperties() {
  observeProperty("playlist", MPV_FORMAT_NODE, [this](void *data) {
    auto node = static_cast<mpv_node>(*(mpv_node *)data);
    if (node.format == MPV_FORMAT_NODE_ARRAY) initPlaylist(node);
  });

  observeProperty("chapter-list", MPV_FORMAT_NODE, [this](void *data) {
    auto node = static_cast<mpv_node>(*(mpv_node *)data);
    if (node.format == MPV_FORMAT_NODE_ARRAY) initChapters(node);
  });

  observeProperty("track-list", MPV_FORMAT_NODE, [this](void *data) {
    auto node = static_cast<mpv_node>(*(mpv_node *)data);
    if (node.format == MPV_FORMAT_NODE_ARRAY) initTracks(node);
  });

  observeProperty("audio-device-list", MPV_FORMAT_NODE, [this](void *data) {
    auto node = static_cast<mpv_node>(*(mpv_node *)data);
    if (node.format == MPV_FORMAT_NODE_ARRAY) initAudioDevices(node);
  });

  observeProperty("input-bindings", MPV_FORMAT_NODE, [this](void *data) {
    auto node = static_cast<mpv_node>(*(mpv_node *)data);
    if (node.format == MPV_FORMAT_NODE_ARRAY) initBindings(node);
  });

  observeProperty("profile-list", MPV_FORMAT_STRING, [this](void *data) {
    if (data != nullptr) initProfiles(static_cast<char *>(*(char **)data));
  });

  observeProperty("aid", MPV_FORMAT_STRING, [this](void *data) { aid = static_cast<char *>(*(char **)data); });
  observeProperty("vid", MPV_FORMAT_STRING, [this](void *data) { vid = static_cast<char *>(*(char **)data); });
  observeProperty("sid", MPV_FORMAT_STRING, [this](void *data) { sid = static_cast<char *>(*(char **)data); });
  observeProperty("audio-device", MPV_FORMAT_STRING, [this](void *data) { audioDevice = static_cast<char *>(*(char **)data); });

  observeProperty("pause", MPV_FORMAT_FLAG, [this](void *data) { pause = static_cast<bool>(*(int *)data); });
  observeProperty("mute", MPV_FORMAT_FLAG, [this](void *data) { mute = static_cast<bool>(*(int *)data); });
  observeProperty("fullscreen", MPV_FORMAT_FLAG, [this](void *data) { fullscreen = static_cast<bool>(*(int *)data); });
  observeProperty("window-dragging", MPV_FORMAT_FLAG,
                  [this](void *data) { windowDragging = static_cast<bool>(*(int *)data); });

  observeProperty("volume", MPV_FORMAT_INT64, [this](void *data) { volume = static_cast<int64_t>(*(int64_t *)data); });
  observeProperty("chapter", MPV_FORMAT_INT64,
                  [this](void *data) { chapter = static_cast<int64_t>(*(int64_t *)data); });
  observeProperty("playlist-pos", MPV_FORMAT_INT64,
                  [this](void *data) { playlistPos = static_cast<int64_t>(*(int64_t *)data); });
  observeProperty("playlist-playing-pos", MPV_FORMAT_INT64,
                  [this](void *data) { playlistPlayingPos = static_cast<int64_t>(*(int64_t *)data); });
}

void Mpv::initPlaylist(mpv_node &node) {
  playlist.clear();
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    Mpv::PlayItem t;
    t.id = i;
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "title") == 0) {
        t.title = value.u.string;
      } else if (strcmp(key, "filename") == 0) {
        t.path = value.u.string;
        t.filename = std::filesystem::path(reinterpret_cast<char8_t *>(t.path.data())).filename().string();
      }
    }
    playlist.emplace_back(t);
  }
}

void Mpv::initChapters(mpv_node &node) {
  chapters.clear();
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    Mpv::ChapterItem t;
    t.id = i;
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "title") == 0) {
        t.title = value.u.string;
      } else if (strcmp(key, "time") == 0) {
        t.time = value.u.double_;
      }
    }
    chapters.emplace_back(t);
  }
}

void Mpv::initTracks(mpv_node &node) {
  tracks.clear();
  for (int i = 0; i < node.u.list->num; i++) {
    auto track = node.u.list->values[i];
    Mpv::TrackItem t;
    for (int j = 0; j < track.u.list->num; j++) {
      auto key = track.u.list->keys[j];
      auto value = track.u.list->values[j];
      if (strcmp(key, "id") == 0) {
        t.id = value.u.int64;
      } else if (strcmp(key, "type") == 0) {
        t.type = value.u.string;
      } else if (strcmp(key, "title") == 0) {
        t.title = value.u.string;
      } else if (strcmp(key, "lang") == 0) {
        t.lang = value.u.string;
      } else if (strcmp(key, "selected") == 0) {
        t.selected = value.u.flag;
      }
    }
    tracks.emplace_back(t);
  }
}

void Mpv::initAudioDevices(mpv_node &node) {
  audioDevices.clear();
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    Mpv::AudioDevice t;
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "name") == 0) {
        t.name = value.u.string;
      } else if (strcmp(key, "description") == 0) {
        t.description = value.u.string;
      }
    }
    audioDevices.emplace_back(t);
  }
}

void Mpv::initBindings(mpv_node &node) {
  bindings.clear();
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    Mpv::BindingItem t;
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "key") == 0) {
        t.key = value.u.string;
      } else if (strcmp(key, "cmd") == 0) {
        t.cmd = value.u.string;
      } else if (strcmp(key, "comment") == 0) {
        t.comment = value.u.string;
      }
    }
    bindings.emplace_back(t);
  }
}

void Mpv::initProfiles(const char *payload) {
  profiles.clear();
  auto j = nlohmann::json::parse(payload);
  for (auto &elm : j) {
    auto name = elm["name"].get_ref<const std::string &>();
    if (name != "builtin-pseudo-gui" && name != "encoding" && name != "libmpv" && name != "pseudo-gui")
      profiles.emplace_back(name);
  }
}
}  // namespace ImPlay