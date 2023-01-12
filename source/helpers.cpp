#include <algorithm>
#include <codecvt>
#include <locale>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <string>
#include <fmt/format.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <romfs/romfs.hpp>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif
#include "helpers.h"
#include <imgui_impl_opengl3_loader.h>

namespace ImPlay::Helpers {
void OptionParser::parse(int argc, char** argv) {
  bool optEnd = false;
#ifdef _WIN32
  int wideArgc;
  wchar_t** wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
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
        options.emplace(arg.substr(3), "no");
      } else if (auto n = arg.find_first_of('='); n != std::string::npos) {
        options.emplace(arg.substr(0, n), arg.substr(n + 1));
      } else {
        options.emplace(arg, "yes");
      }
    } else {
      paths.emplace_back(arg);
    }
  }
}

bool OptionParser::check(std::string key, std::string value) {
  auto it = options.find(key);
  return it != options.end() && it->second == value;
}

void marker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

bool loadTexture(const char* path, ImTextureID* out_texture, int* out_width, int* out_height) {
  int width, height;
  auto icon = romfs::get(path);
  unsigned char* m_data =
      stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(icon.data()), icon.size(), &width, &height, NULL, 4);
  if (m_data == NULL) return false;

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, height, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
  stbi_image_free(m_data);

  *out_texture = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
  *out_width = width;
  *out_height = height;

  return true;
}

std::string tolower(std::string s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

std::string toupper(std::string s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

std::string trim(std::string s) {
  std::string str = s;
  const char* ws = " \t\n\r\f\v";
  str.erase(0, str.find_first_not_of(ws));
  str.erase(str.find_last_not_of(ws) + 1);
  return str;
}

int openUri(const char* uri) {
#ifdef __APPLE__
  return system(fmt::format("open {}", uri).c_str());
#elif defined(_WIN32) || defined(__CYGWIN__)
  return ShellExecute(0, 0, uri, 0, 0, SW_SHOW) > (HINSTANCE)32 ? 0 : 1;
#else
  char command[256];
  return system(fmt::format("xdg-open {}", uri).c_str());
#endif
}

const char* getDataDir(const char* subdir) {
  static char dataDir[256] = {0};
#ifdef _WIN32
  wchar_t* dir = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &dir))) {
    wcstombs(dataDir, dir, 256);
    CoTaskMemFree(dir);
  }
#else
  char* home = getenv("HOME");
  char* xdg_dir = getenv("XDG_CONFIG_HOME");
  if (xdg_dir != nullptr) {
    strncpy(dataDir, xdg_dir, 256);
  } else if (home != nullptr) {
    strncpy(dataDir, home, 256);
    strncat(dataDir, "/.config", 9);
  }
#endif
  if (dataDir[0] != '\0' && subdir != nullptr && subdir[0] != '\0') {
    strncat(dataDir, "/", 2);
    strncat(dataDir, subdir, 20);
  }
  return &dataDir[0];
}
}  // namespace ImPlay::Helpers