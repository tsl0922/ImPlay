#include "player.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

namespace mpv {
Player::Player() { initMpv(); }

Player::~Player() { exitMpv(); }

void Player::command(std::string cmd) { mpv_command_string(mpv, cmd.c_str()); }

void Player::command(const char **args) { mpv_command(mpv, args); }

void Player::pollEvent() {
  mpv_event *event = mpv_wait_event(mpv, 0);
  if (event->event_id == MPV_EVENT_NONE) return;
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

void Player::render(int w, int h) {
  int flip_y{1};
  mpv_opengl_fbo mpfbo{0, w, h};

  mpv_render_param params[]{
      {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
      {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };
  mpv_render_context_render(renderCtx, params);
}

void Player::initMpv() {
  mpv = mpv_create();
  if (!mpv) throw std::runtime_error("could not create mpv context");

  mpv_set_option_string(mpv, "config", "yes");
  mpv_set_option_string(mpv, "osc", "yes");
  mpv_set_option_string(mpv, "idle", "yes");
  mpv_set_option_string(mpv, "force-window", "yes");
  mpv_set_option_string(mpv, "input-default-bindings", "yes");
  mpv_set_option_string(mpv, "input-vo-keyboard", "yes");

  if (mpv_initialize(mpv) < 0)
    throw std::runtime_error("could not initialize mpv context");

  mpv_opengl_init_params gl_init_params{
      [](void *ctx, const char *name) {
        return (void *)glfwGetProcAddress(name);
      },
      nullptr,
  };
  mpv_render_param params[]{
      {MPV_RENDER_PARAM_API_TYPE,
       const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
      {MPV_RENDER_PARAM_INVALID, nullptr},
  };

  if (mpv_render_context_create(&renderCtx, mpv, params) < 0)
    throw std::runtime_error("failed to initialize mpv GL context");
}

void Player::exitMpv() {
  mpv_render_context_free(renderCtx);
  mpv_terminate_destroy(mpv);
}
}  // namespace mpv