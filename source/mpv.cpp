// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <stdexcept>
#include <string>
#include <thread>
#include <cstdarg>
#include <cstring>
#include <nlohmann/json.hpp>
#include "mpv.h"

namespace ImPlay {
Mpv::Mpv() {
  main = mpv_create();
  if (!main) throw std::runtime_error("could not create mpv handle");
  mpv = mpv_create_client(main, "implay");
  if (!mpv) throw std::runtime_error("could not create mpv client");
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
  return mpv_command_async(mpv, 0, args.data());
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

void Mpv::render(int w, int h, int fbo, bool flip) {
  if (renderCtx == nullptr) return;

  int flip_y{flip ? 1 : 0};
  mpv_opengl_fbo mpfbo{fbo, w, h};
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

void Mpv::reportSwap() {
  if (renderCtx != nullptr) mpv_render_context_report_swap(renderCtx);
}

static void *get_proc_address(void *ctx, const char *name) { return ((GLAddrLoadFunc)ctx)(name); }

void Mpv::init(GLAddrLoadFunc load, int64_t wid) {
  if (mpv_set_property(mpv, "wid", MPV_FORMAT_INT64, &wid) < 0) throw std::runtime_error("could not set mpv wid");
  if (mpv_initialize(mpv) < 0) throw std::runtime_error("could not initialize mpv context");
  if (wid == 0) {
    mpv_opengl_init_params gl_init_params{get_proc_address, (void *)load};
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

void Mpv::observeProperties() {
  observeProperty<mpv_node, MPV_FORMAT_NODE>("playlist", [this](mpv_node node) { initPlaylist(node); });
  observeProperty<mpv_node, MPV_FORMAT_NODE>("chapter-list", [this](mpv_node node) { initChapters(node); });
  observeProperty<mpv_node, MPV_FORMAT_NODE>("track-list", [this](mpv_node node) { initTracks(node); });
  observeProperty<mpv_node, MPV_FORMAT_NODE>("audio-device-list", [this](mpv_node node) { initAudioDevices(node); });
  observeProperty<mpv_node, MPV_FORMAT_NODE>("input-bindings", [this](mpv_node node) { initBindings(node); });
  observeProperty<char *, MPV_FORMAT_STRING>("profile-list", [this](char *data) { initProfiles(data); });

  observeProperty<char *, MPV_FORMAT_STRING>("aid", [this](char *data) { aid = data; });
  observeProperty<char *, MPV_FORMAT_STRING>("vid", [this](char *data) { vid = data; });
  observeProperty<char *, MPV_FORMAT_STRING>("sid", [this](char *data) { sid = data; });
  observeProperty<char *, MPV_FORMAT_STRING>("secondary-sid", [this](char *data) { sid2 = data; });
  observeProperty<char *, MPV_FORMAT_STRING>("audio-device", [this](char *data) { audioDevice = data; });
  observeProperty<char *, MPV_FORMAT_STRING>("cursor-autohide", [this](char *data) { cursorAutohide = data; });

  observeProperty<int, MPV_FORMAT_FLAG>("pause", [this](int flag) { pause = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("mute", [this](int flag) { mute = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("fullscreen", [this](int flag) { fullscreen = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("sub-visibility", [this](int flag) { sidv = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("secondary-sub-visibility", [this](int flag) { sidv2 = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("window-dragging", [this](int flag) { windowDragging = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("keepaspect", [this](int flag) { keepaspect = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("ontop", [this](int flag) { ontop = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("keepaspect-window", [this](int flag) { keepaspectWindow = flag; });
  observeProperty<int, MPV_FORMAT_FLAG>("auto-window-resize", [this](int flag) { autoResize = flag; });

  observeProperty<int64_t, MPV_FORMAT_INT64>("volume", [this](int64_t val) { volume = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("chapter", [this](int64_t val) { chapter = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("playlist-pos", [this](int64_t val) { playlistPos = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("playlist-playing-pos", [this](int64_t val) { playlistPlayingPos = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("time-pos", [this](int64_t val) { timePos = val; });

  observeProperty<int64_t, MPV_FORMAT_INT64>("brightness", [this](int64_t val) { brightness = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("contrast", [this](int64_t val) { contrast = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("saturation", [this](int64_t val) { saturation = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("gamma", [this](int64_t val) { gamma = val; });
  observeProperty<int64_t, MPV_FORMAT_INT64>("hue", [this](int64_t val) { hue = val; });

  observeProperty<double, MPV_FORMAT_DOUBLE>("audio-delay", [this](double val) { audioDelay = val; });
  observeProperty<double, MPV_FORMAT_DOUBLE>("sub-delay", [this](double val) { subDelay = val; });
  observeProperty<double, MPV_FORMAT_DOUBLE>("sub-scale", [this](double val) { subScale = val; });
}

void Mpv::initPlaylist(mpv_node &node) {
  if (node.format != MPV_FORMAT_NODE_ARRAY) return;
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
        t.path = reinterpret_cast<char8_t *>(value.u.string);
      }
    }
    playlist.emplace_back(t);
  }
}

void Mpv::initChapters(mpv_node &node) {
  if (node.format != MPV_FORMAT_NODE_ARRAY) return;
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
  if (node.format != MPV_FORMAT_NODE_ARRAY) return;
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
  if (node.format != MPV_FORMAT_NODE_ARRAY) return;
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
  if (node.format != MPV_FORMAT_NODE_ARRAY) return;
  bindings.clear();
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    Mpv::BindingItem t;
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "section") == 0) {
        t.section = value.u.string;
      } else if (strcmp(key, "key") == 0) {
        t.key = value.u.string;
      } else if (strcmp(key, "cmd") == 0) {
        t.cmd = value.u.string;
      } else if (strcmp(key, "comment") == 0) {
        t.comment = value.u.string;
      } else if (strcmp(key, "priority") == 0) {
        t.priority = value.u.int64;
      } else if (strcmp(key, "is_weak") == 0) {
        t.weak = value.u.flag;
      }
    }
    bindings.emplace_back(t);
  }
}

void Mpv::initProfiles(const char *payload) {
  if (payload == nullptr) return;
  profiles.clear();
  auto j = nlohmann::json::parse(payload);
  for (auto &elm : j) {
    auto name = elm["name"].get_ref<const std::string &>();
    if (name != "builtin-pseudo-gui" && name != "encoding" && name != "libmpv" && name != "pseudo-gui")
      profiles.emplace_back(name);
  }
}
}  // namespace ImPlay