// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <map>
#include <algorithm>
#include <filesystem>
#include <nfd.hpp>
#include "helpers.h"
#include "command.h"

namespace ImPlay {
Command::Command(Config *config, GLFWwindow *window, Mpv *mpv) : View(config, mpv) {
  this->window = window;

  about = new Views::About();
  debug = new Views::Debug(config, mpv);
  quick = new Views::Quick(config, mpv);
  settings = new Views::Settings(config, mpv);
  contextMenu = new Views::ContextMenu(config, mpv);
  commandPalette = new Views::CommandPalette(config, mpv);
}

Command::~Command() {
  delete about;
  delete debug;
  delete quick;
  delete settings;
  delete contextMenu;
  delete commandPalette;
}

void Command::init() {
  debug->init();
  if (NFD::Init() != NFD_OKAY)
    messageBox("Warning", "Failed to init NFD, opening files/folders from gui may not work properly.");
  else
    NFD::Quit();
}

void Command::draw() {
  ImGuiIO &io = ImGui::GetIO();
  if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyDown(ImGuiKey_P)) commandPalette->show();
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    contextMenu->show();

  about->draw();
  debug->draw();
  quick->draw();
  settings->draw();
  contextMenu->draw();
  commandPalette->draw();
  drawOpenURL();
  drawDialog();
}

void Command::execute(int n_args, const char **args_) {
  if (n_args == 0) return;

  static std::map<std::string, std::function<void(int, const char **)>> commands = {
      {"open",
       [&](int n, const char **args) {
         openMediaFiles([&](std::string path, int i) {
           mpv->commandv("loadfile", path.c_str(), i > 0 ? "append-play" : "replace", nullptr);
         });
       }},
      {"open-folder",
       [&](int n, const char **args) {
         openMediaFolder([&](std::string path, int i) {
           mpv->commandv("loadfile", path.c_str(), i > 0 ? "append-play" : "replace", nullptr);
         });
       }},
      {"open-disk", [&](int n, const char **args) { openDisk(); }},
      {"open-iso", [&](int n, const char **args) { openIso(); }},
      {"open-clipboard", [&](int n, const char **args) { openClipboard(); }},
      {"open-url", [&](int n, const char **args) { openURL(); }},
      {"load-sub", [&](int n, const char **args) { loadSubtitles(); }},
      {"load-conf",
       [&](int n, const char **args) {
         if (n > 0) mpv->loadConfig(args[0]);
       }},
      {"quickview", [&](int n, const char **args) { openQuickview(n > 0 ? args[0] : nullptr); }},
      {"playlist-add-files",
       [&](int n, const char **args) {
         openMediaFiles([&](std::string path, int i) { mpv->commandv("loadfile", path.c_str(), "append", nullptr); });
       }},
      {"playlist-add-folder",
       [&](int n, const char **args) {
         openMediaFolder([&](std::string path, int i) { mpv->commandv("loadfile", path.c_str(), "append", nullptr); });
       }},
      {"about", [&](int n, const char **args) { about->show(); }},
      {"settings", [&](int n, const char **args) { settings->show(); }},
      {"metrics", [&](int n, const char **args) { debug->show(); }},
      {"command-palette", [&](int n, const char **args) { openCommandPalette(n, args); }},
      {"theme",
       [&](int n, const char **args) {
         if (n > 0) ImGui::SetTheme(args[0]);
       }},
  };

  const char *cmd = args_[0];
  auto it = commands.find(cmd);
  if (it != commands.end()) it->second(n_args - 1, args_ + 1);
}

void Command::openMediaFiles(std::function<void(std::string, int)> callback) {
  openFiles(
      {
          {"Videos Files", format("{}", join(videoTypes, ","))},
          {"Audio Files", format("{}", join(audioTypes, ","))},
          {"Image Files", format("{}", join(imageTypes, ","))},
      },
      [&](std::string path, int i) { callback(path, i); });
}

void Command::openMediaFolder(std::function<void(std::string, int)> callback) {
  openFolder([&](std::string path) {
    auto fp = std::filesystem::path(reinterpret_cast<char8_t *>(path.data()));
    int i = 0;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(fp)) {
      if (isMediaType(entry.path().extension().string())) callback(entry.path().string(), i);
      i++;
    }
  });
}

void Command::openDisk() {
  openFolder([&](std::string path) {
    auto fp = std::filesystem::path(reinterpret_cast<char8_t *>(path.data()));
    if (std::filesystem::exists(fp / u8"BDMV"))
      openBluray(path);
    else
      openDvd(path);
  });
}

void Command::openIso() {
  openFile({{"ISO Image Files", "iso"}}, [&](std::string path) {
    auto fp = std::filesystem::path(reinterpret_cast<char8_t *>(path.data()));
    if ((double)std::filesystem::file_size(fp) / 1000 / 1000 / 1000 > 4.7)
      openBluray(path);
    else
      openDvd(path);
  });
}

void Command::loadSubtitles() {
  openFiles({{"Subtitle Files", format("{}", join(subtitleTypes, ","))}}, [&](std::string path, int i) {
    mpv->commandv("sub-add", path.c_str(), i > 0 ? "auto" : "select", nullptr);
  });
}

void Command::openClipboard() {
  auto content = glfwGetClipboardString(window);
  if (content != nullptr && content[0] != '\0') {
    auto str = trim(content);
    mpv->commandv("loadfile", str.c_str(), nullptr);
    mpv->commandv("show-text", str.c_str(), nullptr);
  }
}

void Command::openURL() { m_openURL = true; }

void Command::openDvd(std::string path) {
  mpv->property("dvd-device", path.c_str());
  mpv->commandv("loadfile", "dvd://", nullptr);
}

void Command::openBluray(std::string path) {
  mpv->property("bluray-device", path.c_str());
  mpv->commandv("loadfile", "bd://", nullptr);
}

void Command::openCommandPalette(int n, const char **args) {
  std::vector<Views::CommandPalette::CommandItem> items;
  std::string source = "bindings";
  if (n > 0) source = args[0];
  if (source == "bindings") {
    auto bindings = mpv->bindingList();
    for (auto &item : bindings)
      items.push_back({
          item.comment,
          item.cmd,
          item.key,
          [=, this]() { mpv->command(item.cmd); },
      });
  } else if (source == "playlist") {
    auto playlist = mpv->playlist();
    for (auto &item : playlist) {
      std::string title = item.title;
      if (title.empty() && !item.filename.empty()) title = item.filename;
      if (title.empty()) title = format("Item {}", item.id + 1);
      items.push_back({
          title,
          item.path,
          "",
          [=, this]() { mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", item.id); },
      });
    }
  } else if (source == "chapters") {
    auto chapters = mpv->chapterList();
    for (auto &item : chapters) {
      auto title = item.title.empty() ? format("Chapter {}", item.id + 1) : item.title;
      auto time = format("{:%H:%M:%S}", std::chrono::duration<int>((int)item.time));
      items.push_back({
          title,
          "",
          time,
          [=, this]() { mpv->commandv("seek", std::to_string(item.time).c_str(), "absolute", nullptr); },
      });
    }
  } else if (source == "tracks") {
    const char *type = n > 1 ? args[1] : "";
    auto tracks = mpv->trackList();
    for (auto &item : tracks) {
      if (type[0] != '\0' && item.type != type) continue;
      auto title = item.title.empty() ? format("Track {}", item.id) : item.title;
      if (!item.lang.empty()) title += format(" [{}]", item.lang);
      items.push_back({
          title,
          "",
          toupper(item.type),
          [=, this]() {
            if (item.type == "audio")
              mpv->property<int64_t, MPV_FORMAT_INT64>("aid", item.id);
            else if (item.type == "video")
              mpv->property<int64_t, MPV_FORMAT_INT64>("vid", item.id);
            else if (item.type == "sub")
              mpv->property<int64_t, MPV_FORMAT_INT64>("sid", item.id);
          },
      });
    }
  }
  commandPalette->items() = items;
  commandPalette->show();
}

void Command::openQuickview(const char *tab) {
  if (tab != nullptr) quick->setTab(tab);
  quick->show();
}

void Command::drawOpenURL() {
  if (!m_openURL) return;
  ImGui::OpenPopup("Open URL");

  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(wSize.x * 0.8f, scaled(50)), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Open URL", &m_openURL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_openURL = false;
    static char url[256] = {0};
    bool loadfile = false;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Input URL", "Input URL Here..", url, IM_ARRAYSIZE(url),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
      if (url[0] != '\0') loadfile = true;
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(10));
    if (ImGui::Button("Cancel", ImVec2(scaled(5), 0))) m_openURL = false;
    ImGui::SameLine();
    if (url[0] == '\0') ImGui::BeginDisabled();
    if (ImGui::Button("OK", ImVec2(scaled(5), 0))) loadfile = true;
    if (url[0] == '\0') ImGui::EndDisabled();
    if (loadfile) {
      m_openURL = false;
      mpv->commandv("loadfile", url, nullptr);
    }
    if (!m_openURL) url[0] = '\0';
    ImGui::EndPopup();
  }
}

void Command::drawDialog() {
  if (m_dialog) ImGui::OpenPopup(m_dialog_title.c_str());
  ImGui::SetNextWindowSize(ImVec2(scaled(30), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal(m_dialog_title.c_str(), &m_dialog, ImGuiWindowFlags_NoMove)) {
    ImGui::TextWrapped("%s", m_dialog_msg.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(5));
    if (ImGui::Button("OK", ImVec2(scaled(5), 0))) m_dialog = false;
    ImGui::EndPopup();
  }
}

void Command::messageBox(std::string title, std::string msg) {
  m_dialog_title = title;
  m_dialog_msg = msg;
  m_dialog = true;
}

bool Command::isMediaType(std::string ext) {
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(videoTypes.begin(), videoTypes.end(), ext) != videoTypes.end()) return true;
  if (std::find(audioTypes.begin(), audioTypes.end(), ext) != audioTypes.end()) return true;
  if (std::find(imageTypes.begin(), imageTypes.end(), ext) != imageTypes.end()) return true;
  return false;
}
}  // namespace ImPlay