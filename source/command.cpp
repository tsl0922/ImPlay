#include <filesystem>
#include <fmt/format.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "command.h"

namespace ImPlay {
Command::Command(GLFWwindow *window, Mpv *mpv) : View() {
  this->window = window;
  this->mpv = mpv;

  about = new Views::About();
  contextMenu = new Views::ContextMenu(mpv);
  commandPalette = new Views::CommandPalette(mpv);
}

Command::~Command() {
  delete about;
  delete contextMenu;
  delete commandPalette;
}

void Command::draw() {
  ImGuiIO &io = ImGui::GetIO();
  if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyDown(ImGuiKey_P)) commandPalette->show();
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    contextMenu->show();

  about->draw();
  contextMenu->draw();
  commandPalette->draw();
}

void Command::execute(int num_args, const char **args) {
  if (num_args == 0) return;
  const char *cmd = args[0];
  if (strcmp(cmd, "open") == 0)
    open();
  else if (strcmp(cmd, "open-disk") == 0)
    openDisk();
  else if (strcmp(cmd, "open-iso") == 0)
    openIso();
  else if (strcmp(cmd, "open-clipboard") == 0)
    openClipboard();
  else if (strcmp(cmd, "load-sub") == 0)
    loadSubtitles();
  else if (strcmp(cmd, "playlist-add-files") == 0)
    playlistAddFiles();
  else if (strcmp(cmd, "playlist-add-folder") == 0)
    playlistAddFolder();
  else if (strcmp(cmd, "about") == 0)
    about->show();
  else if (strcmp(cmd, "command-palette") == 0)
    commandPalette->show();
}

void Command::open() {
  openFiles(
      {
          {"Videos Files", fmt::format("{}", fmt::join(videoTypes, ",")).c_str()},
          {"Audio Files", fmt::format("{}", fmt::join(audioTypes, ",")).c_str()},
      },
      [&](nfdu8char_t *path, int i) { mpv->commandv("loadfile", path, i > 0 ? "append-play" : "replace", nullptr); });
}

void Command::openDisk() {
  openFolder([&](nfdu8char_t *path) {
    auto fp = std::filesystem::path(reinterpret_cast<char8_t *>(path));
    if (std::filesystem::exists(fp / u8"BDMV"))
      openBluray(path);
    else
      openDvd(path);
  });
}

void Command::openIso() {
  openFile({{"ISO Image Files", "iso"}}, [&](nfdu8char_t *path) {
    auto fp = std::filesystem::path(reinterpret_cast<char8_t *>(path));
    if ((double)std::filesystem::file_size(fp) / 1000 / 1000 / 1000 > 4.7)
      openBluray(path);
    else
      openDvd(path);
  });
}

void Command::loadSubtitles() {
  openFiles({{"Subtitle Files", fmt::format("{}", fmt::join(subtitleTypes, ",")).c_str()}},
            [&](nfdu8char_t *path, int i) { mpv->commandv("sub-add", path, i > 0 ? "auto" : "select", nullptr); });
}

void Command::openClipboard() {
  auto content = glfwGetClipboardString(window);
  if (content != nullptr && content[0] != '\0') {
    mpv->commandv("loadfile", content, nullptr);
    mpv->commandv("show-text", content, nullptr);
  }
}

void Command::playlistAddFiles() {
  openFiles(
      {
          {"Videos Files", fmt::format("{}", fmt::join(videoTypes, ",")).c_str()},
          {"Audio Files", fmt::format("{}", fmt::join(audioTypes, ",")).c_str()},
      },
      [&](nfdu8char_t *path, int i) { mpv->commandv("loadfile", path, "append", nullptr); });
}

void Command::playlistAddFolder() {
  openFolder([&](nfdu8char_t *path) {
    auto fp = std::filesystem::path(reinterpret_cast<char8_t *>(path));
    for (const auto &entry : std::filesystem::recursive_directory_iterator(fp)) {
      if (isMediaType(entry.path().extension().string()))
        mpv->commandv("loadfile", entry.path().u8string().c_str(), "append", nullptr);
    }
  });
}

void Command::openDvd(const char *path) {
  mpv->property("dvd-device", path);
  mpv->commandv("loadfile", "dvd://", nullptr);
}

void Command::openBluray(const char *path) {
  mpv->property("bluray-device", path);
  mpv->commandv("loadfile", "bd://", nullptr);
}

bool Command::isMediaType(std::string ext) {
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(videoTypes.begin(), videoTypes.end(), ext) != videoTypes.end()) return true;
  if (std::find(audioTypes.begin(), audioTypes.end(), ext) != audioTypes.end()) return true;
  return false;
}
}  // namespace ImPlay