// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <string>
#include <filesystem>
#include <fmt/format.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <romfs/romfs.hpp>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <limits.h>
#include <sysdir.h>
#include <glob.h>
#endif
#include "helpers.h"

void ImGui::HalignCenter(const char* text) {
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * 0.5f);
}

void ImGui::TextCentered(const char* text, bool disabled) {
  ImGui::HalignCenter(text);
  if (disabled)
    ImGui::TextDisabled("%s", text);
  else
    ImGui::Text("%s", text);
}

void ImGui::TextEllipsis(const char* text, float maxWidth) {
  if (maxWidth == 0) maxWidth = ImGui::GetContentRegionAvail().x;
  ImGuiStyle style = ImGui::GetStyle();
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImVec2 textSize = ImGui::CalcTextSize(text);
  ImVec2 min = ImGui::GetCursorScreenPos();
  ImVec2 max = min + ImVec2(maxWidth - style.FramePadding.x, textSize.y);
  ImRect textRect(min, max);
  ImGui::ItemSize(textRect);
  if (ImGui::ItemAdd(textRect, window->GetID(text)))
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), min, max, max.x, max.x, text, nullptr, &textSize);
}

void ImGui::Hyperlink(const char* label, const char* url) {
  auto style = ImGui::GetStyle();
  ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_ButtonActive]);
  ImGui::Text("%s", label ? label : url);
  if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  if (ImGui::IsItemClicked()) ImPlay::openUrl(url);
  ImGui::PopStyleColor();
}

void ImGui::HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(scaled(35));
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

ImTextureID ImGui::LoadTexture(const char* path, int* width, int* height) {
  int w, h;
  auto icon = romfs::get(path);
  auto data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(icon.data()), icon.size(), &w, &h, NULL, 4);
  if (data == nullptr) return nullptr;

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);

  if (width != nullptr) *width = w;
  if (height != nullptr) *height = h;

  return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
}

void ImGui::SetTheme(const char* theme) {
  ImGuiStyle style;
  if (strcmp(theme, "dark") == 0)
    ImGui::StyleColorsDark(&style);
  else if (strcmp(theme, "light") == 0)
    ImGui::StyleColorsLight(&style);
  else if (strcmp(theme, "classic") == 0)
    ImGui::StyleColorsClassic(&style);
  else if (strcmp(theme, "dracula") == 0)
    ImGui::StyleColorsDracula(&style);
  else
    return;

  style.TabRounding = 4;
  style.ScrollbarRounding = 9;
  style.WindowRounding = 7;
  style.GrabRounding = 3;
  style.FrameRounding = 3;
  style.PopupRounding = 4;
  style.ChildRounding = 4;
  style.WindowShadowSize = 50.0f;
  style.ScrollbarSize = 10.0f;
  style.Colors[ImGuiCol_WindowShadow] = ImVec4(0, 0, 0, 1.0f);

  ImGui::GetStyle() = style;
}

// https://github.com/ocornut/imgui/issues/707#issuecomment-1372640066
void ImGui::StyleColorsDracula(ImGuiStyle* dst) {
  ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
  ImVec4* colors = style->Colors;

  colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
  colors[ImGuiCol_MenuBarBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_Border] = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
  colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.24f};
  colors[ImGuiCol_Text] = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
  colors[ImGuiCol_TextDisabled] = ImVec4{0.5f, 0.5f, 0.5f, 1.0f};
  colors[ImGuiCol_Header] = ImVec4{0.13f, 0.13f, 0.17, 1.0f};
  colors[ImGuiCol_HeaderHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_HeaderActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_Button] = ImVec4{0.13f, 0.13f, 0.17, 1.0f};
  colors[ImGuiCol_ButtonHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_ButtonActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_CheckMark] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};
  colors[ImGuiCol_PopupBg] = ImVec4{0.1f, 0.1f, 0.13f, 0.92f};
  colors[ImGuiCol_SliderGrab] = ImVec4{0.44f, 0.37f, 0.61f, 0.54f};
  colors[ImGuiCol_SliderGrabActive] = ImVec4{0.74f, 0.58f, 0.98f, 0.54f};
  colors[ImGuiCol_FrameBg] = ImVec4{0.13f, 0.13, 0.17, 1.0f};
  colors[ImGuiCol_FrameBgHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_FrameBgActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_Tab] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TabHovered] = ImVec4{0.24, 0.24f, 0.32f, 1.0f};
  colors[ImGuiCol_TabActive] = ImVec4{0.2f, 0.22f, 0.27f, 1.0f};
  colors[ImGuiCol_TabUnfocused] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TitleBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TitleBgActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_ScrollbarBg] = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
  colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{0.24f, 0.24f, 0.32f, 1.0f};
  colors[ImGuiCol_Separator] = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};
  colors[ImGuiCol_SeparatorHovered] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};
  colors[ImGuiCol_SeparatorActive] = ImVec4{0.84f, 0.58f, 1.0f, 1.0f};
  colors[ImGuiCol_ResizeGrip] = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
  colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.74f, 0.58f, 0.98f, 0.29f};
  colors[ImGuiCol_ResizeGripActive] = ImVec4{0.84f, 0.58f, 1.0f, 0.29f};
  colors[ImGuiCol_DockingPreview] = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};
}

void ImPlay::OptionParser::parse(int argc, char** argv) {
  bool optEnd = false;
#ifdef _WIN32
  int wideArgc;
  wchar_t** wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
  for (int i = 1; i < wideArgc; i++) {
    std::string arg = WideToUTF8(wideArgv[i]);
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

bool ImPlay::OptionParser::check(std::string key, std::string value) {
  auto it = options.find(key);
  return it != options.end() && it->second == value;
}

int ImPlay::openUrl(std::string url) {
#ifdef __APPLE__
  return system(format("open '{}'", url).c_str());
#elif defined(_WIN32) || defined(__CYGWIN__)
  return ShellExecuteW(0, 0, UTF8ToWide(url).c_str(), 0, 0, SW_SHOW) > (HINSTANCE)32 ? 0 : 1;
#else
  char command[256];
  return system(format("xdg-open '{}'", url).c_str());
#endif
}

void ImPlay::revealInFolder(std::string path) {
#ifdef __APPLE__
  system(format("open -R '{}'", path).c_str());
#elif defined(_WIN32) || defined(__CYGWIN__)
  std::string arg = format("/select,\"{}\"", path);
  ShellExecuteW(0, 0, L"explorer", UTF8ToWide(arg).c_str(), 0, SW_SHOW);
#else
  auto fp = std::filesystem::path(path);
  auto status = std::filesystem::status(fp);
  auto target = std::filesystem::is_directory(status) ? path : fp.parent_path().string();
  system(format("xdg-open '{}'", target).c_str());
#endif
}

std::string ImPlay::datadir(std::string subdir) {
  std::string dataDir;
#ifdef _WIN32
  wchar_t* dir = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &dir))) {
    dataDir = WideToUTF8(dir);
    CoTaskMemFree(dir);
  }
#elif defined(__APPLE__)
  char path[PATH_MAX];
  auto state = sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_APPLICATION_SUPPORT, SYSDIR_DOMAIN_MASK_USER);
  while ((state = sysdir_get_next_search_path_enumeration(state, path)) != 0) {
    glob_t g;
    if (glob(path, GLOB_TILDE, nullptr, &g) == 0) {
      dataDir = g.gl_pathv[0];
      globfree(&g);
    }
    break;
  }
#else
  char* home = getenv("HOME");
  char* xdg_dir = getenv("XDG_CONFIG_HOME");
  if (xdg_dir != nullptr)
    dataDir = xdg_dir;
  else if (home != nullptr)
    dataDir = format("{}/.config", home);
#endif
  if (!subdir.empty()) {
    if (!dataDir.empty()) dataDir += std::filesystem::path::preferred_separator;
    dataDir += subdir;
  }
  return dataDir;
}
