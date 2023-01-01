#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/printf.h>
#include <fmt/color.h>
#include <nfd.hpp>
#include <filesystem>
#include "player.h"
#include "dispatch.h"

namespace ImPlay {
Player::Player(GLFWwindow* window, const char* title) {
  this->window = window;
  this->title = title;

  mpv = new Mpv();
  about = new Views::About();
  commandPalette = new Views::CommandPalette(mpv);
  contextMenu = new Views::ContextMenu(mpv);

  initMenu();
}

Player::~Player() {
  delete about;
  delete commandPalette;
  delete contextMenu;
  delete mpv;
}

void Player::initMenu() {
  contextMenu->setAction(Views::ContextMenu::Action::ABOUT, [this]() { about->show(); });
  contextMenu->setAction(Views::ContextMenu::Action::PALETTE, [this]() { commandPalette->show(); });
  contextMenu->setAction(Views::ContextMenu::Action::OPEN_FILE,
                         [this]() { dispatch_async([](void* data) { ((Player*)data)->openFile(); }, this); });
  contextMenu->setAction(Views::ContextMenu::Action::OPEN_DISK,
                         [this]() { dispatch_async([](void* data) { ((Player*)data)->openDisk(); }, this); });
  contextMenu->setAction(Views::ContextMenu::Action::OPEN_CLIPBOARD,
                         [this]() { dispatch_async([](void* data) { ((Player*)data)->openClipboard(); }, this); });
  contextMenu->setAction(Views::ContextMenu::Action::OPEN_SUB,
                         [this]() { dispatch_async([](void* data) { ((Player*)data)->loadSub(); }, this); });
}

bool Player::init(int argc, char* argv[]) {
  mpv->option("config", "yes");
  mpv->option("osc", "yes");
  mpv->option("idle", "yes");
  mpv->option("force-window", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");

  Mpv::OptionParser optionParser;
  optionParser.parse(argc, argv);
  for (auto& [key, value] : optionParser.options) {
    if (int err = mpv->option(key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return false;
    }
  }

  mpv->init();

  initMpv();

  for (auto& path : optionParser.paths) mpv->commandv("loadfile", path.c_str(), "append-play", nullptr);

  mpv->command("keybind MBTN_RIGHT ignore");

  return true;
}

void Player::draw() {
  ImGuiIO& io = ImGui::GetIO();
  if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyDown(ImGuiKey_P)) commandPalette->show();

  about->draw();
  commandPalette->draw();
  contextMenu->draw();
}

void Player::render(int w, int h) { mpv->render(w, h); }

bool Player::wantRender() { return mpv->wantRender(); }

void Player::waitEvent() { mpv->waitEvent(); }

void Player::shutdown() { mpv->command("quit"); }

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

  mpv->observeEvent(MPV_EVENT_END_FILE, [this](void* data) {
    glfwSetWindowTitle(window, title);
    glfwSetWindowAspectRatio(window, GLFW_DONT_CARE, GLFW_DONT_CARE);
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

void Player::openFile() {
  NFD::Init();
  const nfdpathset_t* outPaths;
  nfdfilteritem_t filterItem[] = {
      {"Videos Files", "mkv,mp4,avi,mov,flv,mpg,webm,wmv,ts,vob,264,265,asf,avc,avs,dav,h264,h265,hevc,m2t,m2ts"},
      {"Audio Files", "mp3,flac,m4a,mka,mp2,ogg,opus,aac,ac3,dts,dtshd,dtshr,dtsma,eac3,mpa,mpc,thd,w64"},
      {"Image Files", "jpg,bmp,png,gif,webp"},
  };
  if (NFD::OpenDialogMultiple(outPaths, filterItem, 3) == NFD_OKAY) {
    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);
    for (auto i = 0; i < numPaths; i++) {
      nfdchar_t* path;
      NFD::PathSet::GetPath(outPaths, i, path);
      mpv->commandv("loadfile", path, i > 0 ? "append-play" : "replace", nullptr);
    }
    NFD::PathSet::Free(outPaths);
  }
  NFD::Quit();
}

void Player::openDisk() {
  NFD::Init();
  nfdchar_t* outPath;
  if (NFD::PickFolder(outPath) == NFD_OKAY) {
    std::filesystem::path path(outPath);
    if (std::filesystem::exists(path / "BDMV")) {
      mpv->property("bluray-device", outPath);
      mpv->commandv("loadfile", "bd://", nullptr);
    } else {
      mpv->property("dvd-device", outPath);
      mpv->commandv("loadfile", "dvd://", nullptr);
    }
    NFD::FreePath(outPath);
  }
  NFD::Quit();
}

void Player::openClipboard() {
  const char* text = glfwGetClipboardString(window);
  if (text) {
    mpv->commandv("loadfile", text, nullptr);
    mpv->commandv("show-text", text, nullptr);
  }
}

void Player::loadSub() {
  NFD::Init();
  const nfdpathset_t* outPaths;
  nfdfilteritem_t filterItem[] = {
      {"Subtitle Files", "srt,ass,idx,sub,sup,ttxt,txt,ssa,smi,mks"},
  };
  if (NFD::OpenDialogMultiple(outPaths, filterItem, 1) == NFD_OKAY) {
    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);
    for (auto i = 0; i < numPaths; i++) {
      nfdchar_t* path;
      NFD::PathSet::GetPath(outPaths, i, path);
      mpv->commandv("sub-add", path, i > 0 ? "auto" : "select", nullptr);
      NFD::PathSet::FreePath(path);
    }
    NFD::PathSet::Free(outPaths);
  }
  NFD::Quit();
}
}  // namespace ImPlay