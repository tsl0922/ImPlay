#include <map>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "command.h"
#include "helpers.h"

namespace ImPlay {
Command::Command(Config *config, GLFWwindow *window, Mpv *mpv) : View() {
  this->config = config;
  this->window = window;
  this->mpv = mpv;

  about = new Views::About();
  debug = new Views::Debug(mpv);
  settings = new Views::Settings(config, mpv);
  contextMenu = new Views::ContextMenu(config, mpv);
  commandPalette = new Views::CommandPalette(mpv);
}

Command::~Command() {
  delete about;
  delete debug;
  delete settings;
  delete contextMenu;
  delete commandPalette;
}

void Command::init() {
  setTheme(config->Theme.c_str());
  debug->init();
}

void Command::draw() {
  ImGuiIO &io = ImGui::GetIO();
  if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyDown(ImGuiKey_P)) commandPalette->show();
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
      ImGui::GetTopMostPopupModal() == nullptr)
    contextMenu->show();

  about->draw();
  debug->draw();
  settings->draw();
  contextMenu->draw();
  commandPalette->draw();
}

void Command::execute(int n_args, const char **args_) {
  if (n_args == 0) return;

  static std::map<std::string, std::function<void(int, const char **)>> commands = {
      {"open", [&](int n, const char **args) { open(); }},
      {"open-disk", [&](int n, const char **args) { openDisk(); }},
      {"open-iso", [&](int n, const char **args) { openIso(); }},
      {"open-clipboard", [&](int n, const char **args) { openClipboard(); }},
      {"load-sub", [&](int n, const char **args) { loadSubtitles(); }},
      {"load-conf",
       [&](int n, const char **args) {
         if (n > 0) mpv->loadConfig(args[0]);
       }},
      {"playlist-add-files", [&](int n, const char **args) { playlistAddFiles(); }},
      {"playlist-add-folder", [&](int n, const char **args) { playlistAddFolder(); }},
      {"about", [&](int n, const char **args) { about->show(); }},
      {"settings", [&](int n, const char **args) { settings->show(); }},
      {"metrics", [&](int n, const char **args) { debug->show(); }},
      {"command-palette", [&](int n, const char **args) { openCommandPalette(n, args); }},
      {"theme",
       [&](int n, const char **args) {
         if (n > 0) setTheme(args[0]);
       }},
  };

  const char *cmd = args_[0];
  auto it = commands.find(cmd);
  if (it != commands.end()) it->second(n_args - 1, args_ + 1);
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
      if (title.empty()) title = fmt::format("Item {}", item.id + 1);
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
      auto title = item.title.empty() ? fmt::format("Chapter {}", item.id + 1) : item.title;
      auto time = fmt::format("{:%H:%M:%S}", std::chrono::duration<int>((int)item.time));
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
      auto title = item.title.empty() ? fmt::format("Track {}", item.id) : item.title;
      if (!item.lang.empty()) title += fmt::format(" [{}]", item.lang);
      items.push_back({
          title,
          "",
          Helpers::toupper(item.type),
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

void Command::setTheme(const char *theme) {
  ImGuiStyle style;
  if (strcmp(theme, "dark") == 0)
    ImGui::StyleColorsDark(&style);
  else if (strcmp(theme, "light") == 0)
    ImGui::StyleColorsLight(&style);
  else if (strcmp(theme, "classic") == 0)
    ImGui::StyleColorsClassic(&style);
  else
    return;

  style.PopupRounding = 5.0f;
  style.WindowRounding = 5.0f;
  style.WindowShadowSize = 50.0f;
  style.Colors[ImGuiCol_WindowShadow] = ImVec4(0, 0, 0, 1.0f);

  ImGui::GetStyle() = style;
}

bool Command::isMediaType(std::string ext) {
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(videoTypes.begin(), videoTypes.end(), ext) != videoTypes.end()) return true;
  if (std::find(audioTypes.begin(), audioTypes.end(), ext) != audioTypes.end()) return true;
  return false;
}
}  // namespace ImPlay