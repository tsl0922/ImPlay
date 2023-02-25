// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include <romfs/romfs.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#ifdef IMGUI_IMPL_OPENGL_ES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#include "helpers.h"
#include "player.h"

namespace ImPlay {
Player::Player(Config* config, GLFWwindow* window, Mpv* mpv, const char* title) {
  this->config = config;
  this->window = window;
  this->mpv = mpv;
  this->title = title;

  cmd = new Command(config, mpv, window);
}

Player::~Player() { delete cmd; }

bool Player::init(OptionParser& parser) {
  logoTexture = ImGui::LoadTexture("icon.png");

  mpv->option("config", "yes");
  mpv->option("osc", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");
  mpv->option("load-osd-console", "no");
  mpv->option("osd-playing-msg", "${media-title}");
  mpv->option("screenshot-directory", "~~desktop/");

  if (!config->Data.Mpv.UseConfig) {
    writeMpvConf();
    mpv->option("config-dir", config->dir().c_str());
  }

  for (const auto& [key, value] : parser.options) {
    if (int err = mpv->option(key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return false;
    }
  }

  cmd->init();
  mpv->init();

  initObservers();
  mpv->property<int64_t, MPV_FORMAT_INT64>("volume", config->Data.Mpv.Volume);
  if (config->Data.Recent.SpaceToPlayLast) mpv->command("keybind SPACE 'script-message-to implay play-pause'");

  for (auto& path : parser.paths) {
    if (path == "-") mpv->property("input-terminal", "yes");
    mpv->commandv("loadfile", path.c_str(), "append-play", nullptr);
  }

  return true;
}

void Player::draw() {
  drawLogo();
  cmd->draw();
}

void Player::drawLogo() {
  if (logoTexture == nullptr || mpv->forceWindow || !idle) return;

  ImGuiViewport* vp = ImGui::GetMainViewport();
  const float width = std::min(vp->WorkSize.x, vp->WorkSize.y) * 0.1f;
  const ImVec2 delta(width, width);
  const ImVec2 center = vp->GetCenter();
  ImRect bb(center - delta, center + delta);

  ImGui::GetBackgroundDrawList(vp)->AddImage(logoTexture, bb.Min, bb.Max);
}

void Player::shutdown() { mpv->command(config->Data.Mpv.WatchLater ? "quit-watch-later" : "quit"); }

void Player::onCursorEvent(double x, double y) {
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  mpv->commandv("mouse", xs.c_str(), ys.c_str(), nullptr);
}

void Player::onMouseEvent(int button, int action, int mods) {
  auto s = actionMappings.find(action);
  if (s == actionMappings.end()) return;
  const std::string cmd = s->second;

  std::vector<std::string> keys;
  translateMod(keys, mods);
  s = mbtnMappings.find(button);
  if (s == actionMappings.end()) return;
  keys.push_back(s->second);
  const std::string arg = format("{}", join(keys, "+"));
  mpv->commandv(cmd.c_str(), arg.c_str(), nullptr);
}

void Player::onScrollEvent(double x, double y) {
  if (abs(x) > 0) {
    mpv->commandv("keypress", x > 0 ? "WHEEL_LEFT" : "WHEEL_RIGH", nullptr);
    mpv->commandv("keyup", x > 0 ? "WHEEL_LEFT" : "WHEEL_RIGH", nullptr);
  }
  if (abs(y) > 0) {
    mpv->commandv("keypress", y > 0 ? "WHEEL_UP" : "WHEEL_DOWN", nullptr);
    mpv->commandv("keyup", y > 0 ? "WHEEL_UP" : "WHEEL_DOWN", nullptr);
  }
}

void Player::onKeyEvent(int key, int scancode, int action, int mods) {
  auto s = actionMappings.find(action);
  if (s == actionMappings.end()) return;
  const std::string cmd = s->second;

  std::string name;
  if (mods & GLFW_MOD_SHIFT) {
    s = shiftMappings.find(key);
    if (s != shiftMappings.end()) {
      name = s->second;
      mods &= ~GLFW_MOD_SHIFT;
    }
  }
  if (name.empty()) {
    s = keyMappings.find(key);
    if (s == keyMappings.end()) return;
    name = s->second;
  }

  std::vector<std::string> keys;
  translateMod(keys, mods);
  keys.push_back(name);
  const std::string arg = format("{}", join(keys, "+"));
  mpv->commandv(cmd.c_str(), arg.c_str(), nullptr);
}

void Player::onDropEvent(int count, const char** paths) {
  std::sort(paths, paths + count, [](const auto& a, const auto& b) { return strcmp(a, b) < 0; });
  std::vector<std::filesystem::path> files;
  for (int i = 0; i < count; i++) files.emplace_back(reinterpret_cast<char8_t*>(const_cast<char*>(paths[i])));
  cmd->load(files);
}

void Player::initObservers() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [this](void* data) { glfwSetWindowShouldClose(window, GLFW_TRUE); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [this](void* data) {
    int width = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    int height = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (width > 0 && height > 0) {
      int x, y, w, h;
      glfwGetWindowPos(window, &x, &y);
      glfwGetWindowSize(window, &w, &h);

      glfwSetWindowSize(window, width, height);
      glfwSetWindowPos(window, x + (w - width) / 2, y + (h - height) / 2);
      if (mpv->property<int, MPV_FORMAT_FLAG>("keepaspect-window")) glfwSetWindowAspectRatio(window, width, height);
    }
  });

  mpv->observeEvent(MPV_EVENT_FILE_LOADED, [this](void* data) {
    auto path = mpv->property("path");
    if (path != "" && path != "bd://" && path != "dvd://") config->addRecentFile(path, mpv->property("media-title"));
  });

  mpv->observeEvent(MPV_EVENT_CLIENT_MESSAGE, [this](void* data) {
    ImGuiIO& io = ImGui::GetIO();
    io.SetAppAcceptingEvents(false);
    auto msg = static_cast<mpv_event_client_message*>(data);
    cmd->execute(msg->num_args, msg->args);
    io.SetAppAcceptingEvents(true);
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("idle-active", [this](int flag) {
    idle = static_cast<bool>(flag);
    if (idle) {
      glfwSetWindowTitle(window, title);
      glfwSetWindowAspectRatio(window, GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
  });

  mpv->observeProperty<char*, MPV_FORMAT_STRING>("media-title",
                                                 [this](char* data) { glfwSetWindowTitle(window, data); });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("border", [this](int flag) {
    bool enable = static_cast<bool>(flag);
    glfwSetWindowAttrib(window, GLFW_DECORATED, enable);
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("window-maximized", [this](int flag) {
    bool enable = static_cast<bool>(flag);
    if (enable)
      glfwMaximizeWindow(window);
    else
      glfwRestoreWindow(window);
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("window-minimized", [this](int flag) {
    bool enable = static_cast<bool>(flag);
    if (enable)
      glfwIconifyWindow(window);
    else
      glfwRestoreWindow(window);
  });

  mpv->observeProperty<double, MPV_FORMAT_DOUBLE>("window-scale", [this](double scale) {
    int w = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    int h = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (w > 0 && h > 0) glfwSetWindowSize(window, (int)(w * scale), (int)(h * scale));
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("fullscreen", [this](int flag) {
    bool enable = static_cast<bool>(flag);
    bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
    if (isFullscreen == enable) return;

    static int x, y, w, h;
    if (enable) {
      glfwGetWindowPos(window, &x, &y);
      glfwGetWindowSize(window, &w, &h);
      GLFWmonitor* monitor = getMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else
      glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("ontop", [this](int flag) {
    bool enable = static_cast<bool>(flag);
    glfwSetWindowAttrib(window, GLFW_FLOATING, enable);
  });
}

void Player::writeMpvConf() {
  auto path = dataPath();
  auto mpvConf = path / "mpv.conf";
  auto inputConf = path / "input.conf";

  if (!std::filesystem::exists(mpvConf)) {
    std::ofstream file(mpvConf);
    auto content = romfs::get("mpv/mpv.conf");
    file.write(reinterpret_cast<const char*>(content.data()), content.size()) << std::endl;
    file << "# use opengl-hq video output for high-quality video rendering." << std::endl;
    file << "profile=gpu-hq" << std::endl;
    file << "deband=no" << std::endl << std::endl;
    file << "# Enable hardware decoding if available." << std::endl;
    file << "hwdec=auto" << std::endl;
  }

  if (!std::filesystem::exists(inputConf)) {
    std::ofstream file(inputConf);
    auto content = romfs::get("mpv/input.conf");
    file.write(reinterpret_cast<const char*>(content.data()), content.size()) << std::endl;
    file << "MBTN_RIGHT   script-message-to implay context-menu    # show context menu" << std::endl;
#ifdef __APPLE__
    file << "Meta+Shift+p script-message-to implay command-palette # show command palette" << std::endl;
#else
    file << "Ctrl+Shift+p script-message-to implay command-palette # show command palette" << std::endl;
#endif
    file << "`            script-message-to implay metrics         # open console window" << std::endl;
  }

  auto scriptOpts = path / "script-opts";
  auto oscConf = scriptOpts / "osc.conf";
  float scale = 1.0f;
#ifndef __APPLE__
  glfwGetWindowContentScale(window, &scale, nullptr);
#endif

  std::filesystem::create_directories(scriptOpts);
  if (!std::filesystem::exists(oscConf)) {
    std::ofstream file(oscConf);
    file << format("scalewindowed={}", scale) << std::endl;
    file << "hidetimeout=2000" << std::endl;
    file << "idlescreen=no" << std::endl;
  }
}

GLFWmonitor* Player::getMonitor() {
  int n, wx, wy, ww, wh, mx, my;
  int bestoverlap = 0;

  glfwGetWindowPos(window, &wx, &wy);
  glfwGetWindowSize(window, &ww, &wh);
  GLFWmonitor* bestmonitor = nullptr;
  auto monitors = glfwGetMonitors(&n);

  for (int i = 0; i < n; i++) {
    const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
    glfwGetMonitorPos(monitors[i], &mx, &my);
    int overlap = std::max(0, std::min(wx + ww, mx + mode->width) - std::max(wx, mx)) *
                  std::max(0, std::min(wy + wh, my + mode->height) - std::max(wy, my));
    if (bestoverlap < overlap) {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }

  return bestmonitor != nullptr ? bestmonitor : glfwGetPrimaryMonitor();
}
}  // namespace ImPlay