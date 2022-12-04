#include "mpv.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <stdexcept>
#include <string>
#include <cstring>

namespace ImPlay {
Mpv::Mpv() { init(); }

Mpv::~Mpv() { exit(); }

std::vector<Mpv::TrackItem> Mpv::toTracklist(mpv_node *node) {
  std::vector<Mpv::TrackItem> tracks;
  assert(node->format == MPV_FORMAT_NODE_ARRAY);
  for (int i = 0; i < node->u.list->num; i++) {
    auto track = node->u.list->values[i];
    assert(track.format == MPV_FORMAT_NODE_MAP);
    Mpv::TrackItem t{0};
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
    if (t.id != 0) tracks.push_back(t);
  }
  return tracks;
}

std::vector<Mpv::PlayItem> Mpv::toPlaylist(mpv_node *node) {
  std::vector<Mpv::PlayItem> playlist;
  for (int i = 0; i < node->u.list->num; i++) {
    auto item = node->u.list->values[i];
    assert(item.format == MPV_FORMAT_NODE_MAP);
    Mpv::PlayItem t{0};
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "id") == 0) {
        t.id = value.u.int64;
      } else if (strcmp(key, "title") == 0) {
        t.title = value.u.string;
      } else if (strcmp(key, "filename") == 0) {
        t.filename = value.u.string;
      } else if (strcmp(key, "current") == 0) {
        t.current = value.u.flag;
      } else if (strcmp(key, "playing") == 0) {
        t.playing = value.u.flag;
      }
    }
    if (t.id != 0) playlist.push_back(t);
  }
  return playlist;
}

void Mpv::pollEvent() {
  while (mpv) {
    mpv_event *event = mpv_wait_event(mpv, 0);
    if (event->event_id == MPV_EVENT_NONE) break;
    switch (event->event_id) {
      case MPV_EVENT_PROPERTY_CHANGE: {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        for (const auto &[name, format, handler] : propertyEvents)
          if (name == prop->name && format == prop->format) handler(prop->data);
        break;
      }
      default:
        for (const auto &[event_id, handler] : events)
          if (event_id == event->event_id) handler(event->data);
        break;
    }
  }
}

void Mpv::render(int w, int h) {
  int flip_y{1};
  mpv_opengl_fbo mpfbo{0, w, h};

  mpv_render_param params[]{
      {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
      {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };
  mpv_render_context_render(renderCtx, params);
}

void Mpv::init() {
  mpv = mpv_create();
  if (!mpv) throw std::runtime_error("could not create mpv context");

  mpv_set_option_string(mpv, "config", "yes");
  mpv_set_option_string(mpv, "osc", "yes");
  mpv_set_option_string(mpv, "idle", "yes");
  mpv_set_option_string(mpv, "force-window", "yes");
  mpv_set_option_string(mpv, "input-default-bindings", "yes");
  mpv_set_option_string(mpv, "input-vo-keyboard", "yes");

  if (mpv_initialize(mpv) < 0) throw std::runtime_error("could not initialize mpv context");

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
}

void Mpv::exit() {
  mpv_render_context_free(renderCtx);
  mpv_terminate_destroy(mpv);
}
}  // namespace ImPlay