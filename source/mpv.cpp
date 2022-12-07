#include "mpv.h"
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>
#include <cassert>
#include <stdexcept>
#include <codecvt>
#include <locale>
#include <string>
#include <cstring>
#include <cstdarg>
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

namespace ImPlay {
Mpv::Mpv() {
  mpv = mpv_create();
  if (!mpv) throw std::runtime_error("could not create mpv context");
}

Mpv::~Mpv() {
  if (renderCtx != nullptr) mpv_render_context_free(renderCtx);
  if (mpv != nullptr) mpv_terminate_destroy(mpv);
}

void Mpv::OptionParser::parse(int argc, char **argv) {
  bool optEnd = false;
#ifdef _WIN32
  int wideArgc;
  wchar_t **wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
  for (int i = 1; i < wideArgc; i++) {
    std::string arg = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(wideArgv[i]);
#else
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
#endif
    if (arg[0] == '-' && !optEnd) {
      if (arg[1] == '\0') continue;
      if (arg[1] == '-') {
        if (arg[2] == '\0') {
          optEnd = true;
          continue;
        } else {
          arg = arg.substr(2);
        }
      } else {
        arg = arg.substr(1);
      }
      if (arg.starts_with("no-")) {
        if (arg[3] == '\0') continue;
        options.emplace_back(arg.substr(3), "no");
      } else if (auto n = arg.find_first_of('='); n != std::string::npos) {
        options.emplace_back(arg.substr(0, n), arg.substr(n + 1));
      } else {
        options.emplace_back(arg, "yes");
      }
    } else {
      paths.emplace_back(arg);
    }
  }
}

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
  assert(node->format == MPV_FORMAT_NODE_ARRAY);
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

std::vector<Mpv::ChapterItem> Mpv::toChapterlist(mpv_node *node) {
  std::vector<Mpv::ChapterItem> chapters;
  assert(node->format == MPV_FORMAT_NODE_ARRAY);
  for (int i = 0; i < node->u.list->num; i++) {
    auto item = node->u.list->values[i];
    assert(item.format == MPV_FORMAT_NODE_MAP);
    Mpv::ChapterItem t{0};
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
    if (t.time != 0) chapters.push_back(t);
  }
  return chapters;
}

std::vector<Mpv::BindingItem> Mpv::toBindinglist(mpv_node *node) {
  std::vector<Mpv::BindingItem> bindings;
  assert(node->format == MPV_FORMAT_NODE_ARRAY);
  for (int i = 0; i < node->u.list->num; i++) {
    auto item = node->u.list->values[i];
    assert(item.format == MPV_FORMAT_NODE_MAP);
    Mpv::BindingItem t{0};
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
    if (t.cmd != nullptr) bindings.push_back(t);
  }
  return bindings;
}

std::vector<std::string> Mpv::toProfilelist(const char *payload) {
  using json = nlohmann::json;
  std::vector<std::string> profiles;
  auto j = json::parse(payload);
  for (auto &elm : j) {
    auto name = elm["name"].get_ref<const std::string &>();
    if (name != "builtin-pseudo-gui" && name != "encoding" && name != "libmpv" && name != "pseudo-gui")
      profiles.push_back(name);
  }
  return profiles;
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
}  // namespace ImPlay