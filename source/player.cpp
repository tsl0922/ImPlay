#include <fmt/printf.h>
#include <fmt/color.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include "player.h"
#include "helpers.h"

namespace ImPlay {
Player::Player(Config* config, GLFWwindow* window, Mpv* mpv, const char* title) : Views::View() {
  this->config = config;
  this->window = window;
  this->mpv = mpv;
  this->title = title;

  cmd = new Command(config, window, mpv);
}

Player::~Player() { delete cmd; }

bool Player::init(Helpers::OptionParser& parser) {
  if (!Helpers::loadTexture("icon.png", &iconTexture, &iconWidth, &iconHeight)) return false;

  mpv->option("config", "yes");
  mpv->option("osc", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");
  mpv->option("osd-playing-msg", "${media-title}");
  mpv->option("screenshot-directory", "~~desktop/");
  if (!config->mpvConfig) mpv->option("config-dir", Helpers::getDataDir());

  for (const auto& option : parser.options) {
    auto key = option.first;
    auto value = option.second;
    if (int err = mpv->option(key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return false;
    }
  }

  cmd->init();
  mpv->init();
  initMpv();

  for (auto& path : parser.paths) mpv->commandv("loadfile", path.c_str(), "append-play", nullptr);

  mpv->command("keybind MBTN_RIGHT ignore");
  mpv->runLoop() = false;

  return true;
}

void Player::draw() {
  if (!mpv->property<int, MPV_FORMAT_FLAG>("force-window") && !fileOpen) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_WindowShadow, ImVec4(0, 0, 0, 0));
    ImGui::Begin("Logo", nullptr,
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoInputs);
    ImGui::Image(iconTexture, ImVec2(iconWidth * 0.5f, iconHeight * 0.5f));
    ImGui::End();
    ImGui::PopStyleColor();
  }
  cmd->draw();
}

void Player::render(int w, int h) { mpv->render(w, h); }

bool Player::wantRender() { return mpv->wantRender(); }

void Player::waitEvent() { mpv->waitEvent(); }

bool Player::playing() { return mpv->playing() && !mpv->paused(); }

void Player::shutdown() { mpv->command(config->watchLater ? "quit-watch-later" : "quit"); }

bool Player::allowDrag() {
  return mpv->property<int, MPV_FORMAT_FLAG>("window-dragging") && !mpv->property<int, MPV_FORMAT_FLAG>("fullscreen");
}

void Player::setCursor(double x, double y) {
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  mpv->commandv("mouse", xs.c_str(), ys.c_str(), nullptr);
}

void Player::setMouse(int button, int action, int mods) {
  auto s = actionMappings.find(action);
  if (s == actionMappings.end()) return;
  const std::string cmd = s->second;

  std::vector<std::string> keys;
  translateMod(keys, mods);
  s = mbtnMappings.find(button);
  if (s == actionMappings.end()) return;
  keys.push_back(s->second);
  const std::string arg = fmt::format("{}", fmt::join(keys, "+"));
  mpv->commandv(cmd.c_str(), arg.c_str(), nullptr);
}

void Player::setScroll(double x, double y) {
  if (abs(x) > 0) {
    mpv->command(x > 0 ? "keypress WHEEL_LEFT" : "keypress WHEEL_RIGH");
    mpv->command(x > 0 ? "keyup WHEEL_LEFT" : "keyup WHEEL_RIGH");
  }
  if (abs(y) > 0) {
    mpv->command(y > 0 ? "keypress WHEEL_UP" : "keypress WHEEL_DOWN");
    mpv->command(y > 0 ? "keyup WHEEL_UP" : "keyup WHEEL_DOWN");
  }
}

void Player::setKey(int key, int scancode, int action, int mods) {
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
  const std::string arg = fmt::format("{}", fmt::join(keys, "+"));
  mpv->commandv(cmd.c_str(), arg.c_str(), nullptr);
}

void Player::setDrop(int count, const char** paths) {
  std::sort(paths, paths + count, [](const auto& a, const auto& b) { return strcmp(a, b) < 0; });
  for (int i = 0; i < count; i++) {
    mpv->commandv("loadfile", paths[i], i > 0 ? "append-play" : "replace", nullptr);
  }
}

void Player::initMpv() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [this](void* data) { glfwSetWindowShouldClose(window, GLFW_TRUE); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [this](void* data) {
    int width = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    int height = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (width > 0 && height > 0) {
      glfwSetWindowSize(window, width, height);
      glfwSetWindowAspectRatio(window, width, height);
    }
  });

  mpv->observeEvent(MPV_EVENT_START_FILE, [this](void* data) { fileOpen = true; });

  mpv->observeEvent(MPV_EVENT_END_FILE, [this](void* data) {
    fileOpen = false;
    glfwSetWindowTitle(window, title);
    glfwSetWindowAspectRatio(window, GLFW_DONT_CARE, GLFW_DONT_CARE);
  });

  mpv->observeEvent(MPV_EVENT_CLIENT_MESSAGE, [this](void* data) {
    ImGuiIO& io = ImGui::GetIO();
    mpv->runLoop() = true;
    io.SetAppAcceptingEvents(false);

    auto msg = static_cast<mpv_event_client_message*>(data);
    cmd->execute(msg->num_args, msg->args);

    mpv->runLoop() = false;
    io.SetAppAcceptingEvents(true);
  });

  mpv->observeProperty("media-title", MPV_FORMAT_STRING, [this](void* data) {
    char* title = static_cast<char*>(*(char**)data);
    glfwSetWindowTitle(window, title);
  });

  mpv->observeProperty("border", MPV_FORMAT_FLAG, [this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    glfwSetWindowAttrib(window, GLFW_DECORATED, enable ? GLFW_TRUE : GLFW_FALSE);
  });

  mpv->observeProperty("window-maximized", MPV_FORMAT_FLAG, [this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    if (enable)
      glfwMaximizeWindow(window);
    else
      glfwRestoreWindow(window);
  });

  mpv->observeProperty("window-minimized", MPV_FORMAT_FLAG, [this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    if (enable)
      glfwIconifyWindow(window);
    else
      glfwRestoreWindow(window);
  });

  mpv->observeProperty("fullscreen", MPV_FORMAT_FLAG, [this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
    if (isFullscreen == enable) return;

    static int x, y, w, h;
    if (enable) {
      glfwGetWindowPos(window, &x, &y);
      glfwGetWindowSize(window, &w, &h);
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else
      glfwSetWindowMonitor(window, nullptr, x, y, w, h, 0);
  });

  mpv->observeProperty("ontop", MPV_FORMAT_FLAG, [this](void* data) {
    bool enable = static_cast<bool>(*(int*)data);
    glfwSetWindowAttrib(window, GLFW_FLOATING, enable ? GLFW_TRUE : GLFW_FALSE);
  });
}
}  // namespace ImPlay