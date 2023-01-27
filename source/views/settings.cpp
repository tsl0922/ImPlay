// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <string>
#include <fonts/fontawesome.h>
#include "views/settings.h"
#include "helpers.h"

namespace ImPlay::Views {
Settings::Settings(Config *config, Mpv *mpv) : View() {
  this->config = config;
  this->mpv = mpv;
}

void Settings::show() {
  data = config->Data;
  m_open = true;
}

void Settings::draw() {
  if (!m_open) return;
  ImGui::OpenPopup("Settings");
  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  float w = std::min(wSize.x * 0.8f, scaled(50));
  ImGui::SetNextWindowSize(ImVec2(w, 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Settings", &m_open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_open = false;
    ImGui::TextWrapped("* Changes will take effect after restart.");
    if (ImGui::BeginTabBar("Settings")) {
      drawGeneralTab();
      drawInterfaceTab();
      drawFontTab();
      ImGui::EndTabBar();
    }
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::Spacing();
    drawButtons();
    ImGui::EndPopup();
  }
}

void Settings::drawButtons() {
  bool apply = false;
  bool same = data == config->Data;

  ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (scaled(15) + 3 * ImGuiStyle().ItemSpacing.x));
  if (ImGui::Button("OK", ImVec2(scaled(5), 0))) {
    apply = true;
    m_open = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel", ImVec2(scaled(5), 0))) m_open = false;
  ImGui::SameLine();
  if (same) ImGui::BeginDisabled();
  if (ImGui::Button("Apply", ImVec2(scaled(5), 0))) apply = true;
  if (same) ImGui::EndDisabled();

  if (!same && apply) {
    if (data.Font != config->Data.Font || data.Interface.Scale != config->Data.Interface.Scale)
      config->FontReload = true;
    config->Data = data;
    for (auto &fn : appliers) fn();
    config->save();
  }
}

void Settings::drawGeneralTab() {
  if (ImGui::BeginTabItem("General")) {
    ImGui::Text("Core");
    ImGui::Indent();
    ImGui::Checkbox("Use mpv's config dir*", &data.Mpv.UseConfig);
    ImGui::SameLine();
    ImGui::HelpMarker("ImPlay will use it's own config dir for libmpv by default.");
#ifdef _WIN32
    ImGui::Checkbox("Enable --wid for libmpv* (DO NOT USE)", &data.Mpv.UseWid);
    ImGui::SameLine();
    ImGui::HelpMarker(
        "Experimental, Windows only, still have issues.\n"
        "This allow using DirectX, Which is usually faster than OpenGL.");
#endif
    ImGui::Checkbox("Remember playback progress on exit", &data.Mpv.WatchLater);
    ImGui::SameLine();
    ImGui::HelpMarker("Exit mpv with the quit-watch-later command.");
    ImGui::Checkbox("Remember window position and size on exit", &data.Window.Save);
    ImGui::Unindent();
    ImGui::Text("Debug");
    ImGui::SameLine();
    ImGui::HelpMarker(
        "Controls the debug settings used on startup.\n"
        "It can be changed later in debug window, but won't be saved.");
    ImGui::Indent();
    const char *items[] = {"fatal", "error", "warn", "info", "v", "debug", "trace", "no"};
    static int current;
    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
      if (strcmp(items[i], data.Debug.LogLevel.c_str()) == 0) current = i;
    }
    if (ImGui::Combo("Log Level*", &current, items, IM_ARRAYSIZE(items))) data.Debug.LogLevel = items[current];
    ImGui::InputInt("Log Limit*", &data.Debug.LogLimit, 0);
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}

void Settings::drawInterfaceTab() {
  if (ImGui::BeginTabItem("Interface")) {
    const char *t_items[] = {"Dark", "Light", "Classic"};
    static int t_current;
    for (int i = 0; i < IM_ARRAYSIZE(t_items); i++) {
      if (tolower(t_items[i]) == data.Interface.Theme) t_current = i;
    }
    ImGui::Text("Theme");
    ImGui::SameLine();
    ImGui::HelpMarker("Color theme of the interface.");
    ImGui::Indent();
    if (ImGui::Combo("##Theme", &t_current, t_items, IM_ARRAYSIZE(t_items))) {
      data.Interface.Theme = tolower(t_items[t_current]);
      appliers.push_back(
          [&]() { mpv->commandv("script-message-to", "implay", "theme", data.Interface.Theme.c_str(), nullptr); });
    }
    ImGui::Unindent();

    const char *s_items[] = {"Native", "0.5x", "1.0x", "1.5x", "2.0x", "3.0x", "4.0x"};
    static int s_current;
    static auto s_to_value = [](const char *s) -> float {
      if (strcmp(s, "Native") == 0) return 0;
      return std::stof(s);
    };
    for (int i = 0; i < IM_ARRAYSIZE(s_items); i++) {
      if (s_to_value(s_items[i]) == data.Interface.Scale) s_current = i;
    }
    ImGui::Text("Scale");
    ImGui::Indent();
    if (ImGui::Combo("##Scale", &s_current, s_items, IM_ARRAYSIZE(s_items)))
      data.Interface.Scale = s_to_value(s_items[s_current]);
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}

void Settings::drawFontTab() {
  static char fontPath[256] = {0};
  strncpy(fontPath, data.Font.Path.c_str(), IM_ARRAYSIZE(fontPath));
  if (ImGui::BeginTabItem("Font")) {
    ImGui::Text("Path");
    ImGui::SameLine();
    ImGui::HelpMarker("An embedded font will be used if not specified.");
    ImGui::Indent();
    if (ImGui::InputText("##Path", fontPath, IM_ARRAYSIZE(fontPath))) data.Font.Path = fontPath;
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN)) {
      openFile({{"Font Files", "ttf,ttc,otf"}}, [&](const char *path) {
        strncpy(fontPath, path, IM_ARRAYSIZE(fontPath));
        data.Font.Path = path;
      });
    }
    ImGui::Unindent();
    ImGui::Text("Size");
    ImGui::Indent();
    ImGui::SliderInt("##Size", &data.Font.Size, 8, 72);
    ImGui::Unindent();
    ImGui::Text("Glyph Ranges");
    ImGui::SameLine();
    ImGui::HelpMarker("Required for displaying non-English characters on interface.");
    ImGui::Indent();
    ImGui::CheckboxFlags("Chinese", &data.Font.GlyphRange, Config::GlyphRange_Chinese);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Cyrillic", &data.Font.GlyphRange, Config::GlyphRange_Cyrillic);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Japanese", &data.Font.GlyphRange, Config::GlyphRange_Japanese);
    ImGui::CheckboxFlags("Korean", &data.Font.GlyphRange, Config::GlyphRange_Korean);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Thai", &data.Font.GlyphRange, Config::GlyphRange_Thai);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Vietnamese", &data.Font.GlyphRange, Config::GlyphRange_Vietnamese);
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}
}  // namespace ImPlay::Views