// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <map>
#include <algorithm>
#include <filesystem>
#include "helpers.h"
#include "theme.h"
#include "command.h"

namespace ImPlay {
Command::Command(Config *config, Mpv *mpv, GLFWwindow *window) : View(config, mpv), window(window) {
  about = new Views::About();
  debug = new Views::Debug(config, mpv);
  quickview = new Views::Quickview(config, mpv);
  settings = new Views::Settings(config, mpv);
  contextMenu = new Views::ContextMenu(config, mpv);
  commandPalette = new Views::CommandPalette(config, mpv);
}

Command::~Command() {
  delete about;
  delete debug;
  delete quickview;
  delete settings;
  delete contextMenu;
  delete commandPalette;
}

void Command::init() { debug->init(); }

void Command::draw() {
  about->draw();
  debug->draw();
  quickview->draw();
  settings->draw();
  contextMenu->draw();
  commandPalette->draw();
  drawOpenURL();
  drawDialog();
}

void Command::execute(int n_args, const char **args_) {
  if (n_args == 0) return;

  static std::map<std::string, std::function<void(int, const char **)>> commands = {
      {"open", [&](int n, const char **args) { openFilesDlg(mediaFilters); }},
      {"open-folder", [&](int n, const char **args) { openFolderDlg(); }},
      {"open-disk", [&](int n, const char **args) { openFolderDlg(false, true); }},
      {"open-iso", [&](int n, const char **args) { openFileDlg(isoFilters); }},
      {"open-clipboard", [&](int n, const char **args) { openClipboard(); }},
      {"open-url", [&](int n, const char **args) { openURL(); }},
      {"load-sub", [&](int n, const char **args) { openFilesDlg(subtitleFilters); }},
      {"load-conf",
       [&](int n, const char **args) {
         if (n > 0) mpv->loadConfig(args[0]);
       }},
      {"quickview", [&](int n, const char **args) { quickview->show(n > 0 ? args[0] : nullptr); }},
      {"playlist-add-files", [&](int n, const char **args) { openFilesDlg(mediaFilters, true); }},
      {"playlist-add-folder", [&](int n, const char **args) { openFolderDlg(true); }},
      {"play-pause",
       [&](int n, const char **args) {
         auto count = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-count");
         if (count > 0)
           mpv->command("cycle pause");
         else if (config->getRecentFiles().size() > 0) {
           for (auto &file : config->getRecentFiles()) {
             if (fileExists(file.path) || file.path.find("://") != std::string::npos) {
               mpv->commandv("loadfile", file.path.c_str(), nullptr);
               break;
             }
           }
         }
       }},
      {"about", [&](int n, const char **args) { about->show(); }},
      {"settings", [&](int n, const char **args) { settings->show(); }},
      {"metrics", [&](int n, const char **args) { debug->show(); }},
      {"command-palette", [&](int n, const char **args) { commandPalette->show(n, args); }},
      {"context-menu", [&](int n, const char **args) { contextMenu->show(); }},
      {"show-message",
       [&](int n, const char **args) {
         if (n > 1) messageBox(args[0], args[1]);
       }},
      {"theme",
       [&](int n, const char **args) {
         if (n > 0) ImGui::SetTheme(args[0]);
       }},
  };

  const char *cmd = args_[0];
  auto it = commands.find(cmd);
  try {
    if (it != commands.end()) it->second(n_args - 1, args_ + 1);
  } catch (const std::exception &e) {
    messageBox("Error", format("{}: {}", cmd, e.what()));
  }
}

void Command::openFileDlg(std::vector<std::pair<std::string, std::string>> filters, bool append) {
  mpv->command("set pause yes");
  if (const auto [path, ok] = openFile(filters); ok) load({path}, append);
  mpv->command("set pause no");
}

void Command::openFilesDlg(std::vector<std::pair<std::string, std::string>> filters, bool append) {
  mpv->command("set pause yes");
  if (const auto [paths, ok] = openFiles(filters); ok) load(paths, append);
  mpv->command("set pause no");
}

void Command::openFolderDlg(bool append, bool disk) {
  mpv->command("set pause yes");
  if (const auto [path, ok] = openFolder(); ok) load({path}, append, disk);
  mpv->command("set pause no");
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

void Command::openDvd(std::filesystem::path path) {
  mpv->property("dvd-device", path.string().c_str());
  mpv->commandv("loadfile", "dvd://", nullptr);
}

void Command::openBluray(std::filesystem::path path) {
  mpv->property("bluray-device", path.string().c_str());
  mpv->commandv("loadfile", "bd://", nullptr);
}

void Command::load(std::vector<std::filesystem::path> files, bool append, bool disk) {
  int i = 0;
  for (auto &file : files) {
    if (std::filesystem::is_directory(file)) {
      if (disk) {
        if (std::filesystem::exists(file / u8"BDMV"))
          openBluray(file);
        else
          openDvd(file);
        break;
      }
      for (const auto &entry : std::filesystem::recursive_directory_iterator(file)) {
        auto path = entry.path().string();
        if (isMediaFile(path)) {
          mpv->commandv("loadfile", path.c_str(), append || i > 0 ? "append-play" : "replace", nullptr);
          i++;
        }
      }
    } else {
      if (file.extension() == ".iso") {
        if ((double)std::filesystem::file_size(file) / 1000 / 1000 / 1000 > 4.7)
          openBluray(file);
        else
          openDvd(file);
        break;
      } else if (isSubtitleFile(file.string())) {
        mpv->commandv("sub-add", file.string().c_str(), append ? "auto" : "select", nullptr);
      } else {
        mpv->commandv("loadfile", file.string().c_str(), append || i > 0 ? "append-play" : "replace", nullptr);
      }
      i++;
    }
  }
}

void Command::drawOpenURL() {
  if (!m_openURL) return;
  ImGui::OpenPopup("views.dialog.open_url.title"_i18n);

  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(wSize.x * 0.8f, scaled(50)), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("views.dialog.open_url.title"_i18n, &m_openURL,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_openURL = false;
    static char url[256] = {0};
    bool loadfile = false;
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Input URL", "views.dialog.open_url.hint"_i18n, url, IM_ARRAYSIZE(url),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
      if (url[0] != '\0') loadfile = true;
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(10));
    if (ImGui::Button("views.dialog.open_url.cancel"_i18n, ImVec2(scaled(5), 0))) m_openURL = false;
    ImGui::SameLine();
    if (url[0] == '\0') ImGui::BeginDisabled();
    if (ImGui::Button("views.dialog.open_url.ok"_i18n, ImVec2(scaled(5), 0))) loadfile = true;
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

bool Command::isMediaFile(std::string file) {
  auto ext = std::filesystem::path(file).extension().string();
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(videoTypes.begin(), videoTypes.end(), ext) != videoTypes.end()) return true;
  if (std::find(audioTypes.begin(), audioTypes.end(), ext) != audioTypes.end()) return true;
  if (std::find(imageTypes.begin(), imageTypes.end(), ext) != imageTypes.end()) return true;
  return false;
}

bool Command::isSubtitleFile(std::string file) {
  auto ext = std::filesystem::path(file).extension().string();
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(subtitleTypes.begin(), subtitleTypes.end(), ext) != subtitleTypes.end()) return true;
  return false;
}
}  // namespace ImPlay