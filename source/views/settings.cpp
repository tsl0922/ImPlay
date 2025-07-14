// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <filesystem>
#include <string>
#include <fonts/fontawesome.h>
#include "helpers/utils.h"
#include "helpers/imgui.h"
#include "helpers/nfd.h"
#include "theme.h"
#include "views/settings.h"

namespace ImPlay::Views {
void Settings::show() {
  data = config->Data;
  m_open = true;
}

void Settings::draw() {
  if (!m_open) return;
  ImGui::OpenPopup("views.settings.title"_i18n);
  auto viewport = ImGui::GetMainViewport();
  float w = std::min(viewport->WorkSize.x * 0.8f, scaled(50));
  ImGui::SetNextWindowSize(ImVec2(w, 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("views.settings.title"_i18n, &m_open)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_open = false;
    ImGui::TextUnformatted("views.settings.hint"_i18n);
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

  ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - (scaled(15) + 3 * ImGuiStyle().ItemSpacing.x));
  if (ImGui::Button("views.settings.ok"_i18n, ImVec2(scaled(5), 0))) {
    apply = true;
    m_open = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("views.settings.cancel"_i18n, ImVec2(scaled(5), 0))) m_open = false;
  ImGui::SameLine();
  if (same) ImGui::BeginDisabled();
  if (ImGui::Button("views.settings.apply"_i18n, ImVec2(scaled(5), 0))) apply = true;
  if (same) ImGui::EndDisabled();

  if (!same && apply) {
    // use recommended font from lang if not set
    if (data.Interface.Lang != config->Data.Interface.Lang && data.Font.Path == "") {
      auto it = getLangs().find(data.Interface.Lang);
      if (it != getLangs().end()) {
        auto lang = it->second;
        for (auto &font : lang.fonts) {
          if (!fileExists(font.path)) continue;
          data.Font.Path = font.path;
          if (font.size > 0) data.Font.Size = font.size;
          if (font.glyph_range > 0) data.Font.GlyphRange = font.glyph_range;
          break;
        }
      }
    }
    if (data.Font != config->Data.Font || data.Interface.Scale != config->Data.Interface.Scale ||
        data.Interface.Rounding != config->Data.Interface.Rounding ||
        data.Interface.Shadow != config->Data.Interface.Shadow)
      config->FontReload = true;
    config->Data = data;
    for (auto &fn : appliers) fn();
    config->save();
  }
}

void Settings::drawGeneralTab() {
  if (ImGui::BeginTabItem("views.settings.general"_i18n)) {
    ImGui::TextUnformatted("views.settings.general.core"_i18n);
    ImGui::Indent();
    ImGui::Checkbox("views.settings.general.window.single"_i18n, &data.Window.Single);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.general.window.single.help"_i18n);
    ImGui::Checkbox("views.settings.general.mpv.config"_i18n, &data.Mpv.UseConfig);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.general.mpv.config.help"_i18n);
#ifdef _WIN32
    ImGui::Checkbox("views.settings.general.mpv.wid"_i18n, &data.Mpv.UseWid);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.general.mpv.wid.help"_i18n);
#endif
    ImGui::Checkbox("views.settings.general.recent.play_last"_i18n, &data.Recent.SpaceToPlayLast);
    ImGui::Checkbox("views.settings.general.mpv.watch_later"_i18n, &data.Mpv.WatchLater);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.general.mpv.watch_later.help"_i18n);
    ImGui::Checkbox("views.settings.general.window.save"_i18n, &data.Window.Save);
    ImGui::Spacing();
    ImGui::TextUnformatted("views.settings.general.recent.limit"_i18n);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(scaled(6));
    if (ImGui::InputInt("##recent_limit", &data.Recent.Limit, 1, 0, ImGuiInputTextFlags_CharsDecimal)) {
      if (data.Recent.Limit < 0) data.Recent.Limit = 0;
      appliers.push_back([this]() {
        if (data.Recent.Limit == 0) config->clearRecentFiles();
      });
    }
    ImGui::Unindent();
    ImGui::TextUnformatted("views.settings.general.debug"_i18n);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.general.debug.help"_i18n);
    ImGui::Indent();
    const char *items[] = {"fatal", "error", "warn", "info", "status", "v", "debug", "trace", "no"};
    static int current;
    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
      if (strcmp(items[i], data.Debug.LogLevel.c_str()) == 0) current = i;
    }
    if (ImGui::Combo("views.settings.general.debug.log_level"_i18n, &current, items, IM_ARRAYSIZE(items)))
      data.Debug.LogLevel = items[current];
    ImGui::InputInt("views.settings.general.debug.log_limit"_i18n, &data.Debug.LogLimit, 0);
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}

void Settings::drawInterfaceTab() {
  if (ImGui::BeginTabItem("views.settings.interface"_i18n)) {
    ImGui::TextUnformatted("views.settings.interface.gui"_i18n);
    ImGui::Indent();
#ifdef IMGUI_HAS_DOCK
    ImGui::Checkbox("views.settings.interface.docking"_i18n, &data.Interface.Docking);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.interface.docking.help"_i18n);
#endif
#ifdef IMGUI_HAS_VIEWPORT
    ImGui::SameLine(scaled(20));
    ImGui::Checkbox("views.settings.interface.viewports"_i18n, &data.Interface.Viewports);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.interface.viewports.help"_i18n);
#endif
    ImGui::Checkbox("views.settings.interface.rounding"_i18n, &data.Interface.Rounding);
#ifdef IMGUI_HAS_SHADOWS
    ImGui::SameLine(scaled(20));
    ImGui::Checkbox("views.settings.interface.shadow"_i18n, &data.Interface.Shadow);
#endif
    ImGui::SliderInt("views.settings.interface.fps"_i18n, &data.Interface.Fps, 15, 200);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.interface.fps.help"_i18n);
    ImGui::Unindent();

    std::vector<std::pair<std::string, std::string>> langCodes;
    std::vector<const char *> langs;
    for (auto &[code, lang] : getLangs()) {
      langCodes.push_back(std::make_pair(code, lang.title));
      langs.push_back(lang.title.c_str());
    }
    static int l_current;
    for (int i = 0; i < langCodes.size(); i++) {
      if (langCodes[i].first == data.Interface.Lang) l_current = i;
    }
    ImGui::TextUnformatted("views.settings.interface.language"_i18n);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.interface.language.hint"_i18n);
    ImGui::Indent();
    if (ImGui::Combo("##Language", &l_current, langs.data(), langs.size())) {
      data.Interface.Lang = langCodes[l_current].first;
      appliers.push_back([&]() { getLang() = data.Interface.Lang; });
    }
    ImGui::Unindent();

    auto themes = ImGui::Themes();
    static int t_current;
    for (int i = 0; i < themes.size(); i++) {
      if (tolower(themes[i]) == data.Interface.Theme) t_current = i;
    }
    ImGui::TextUnformatted("views.settings.interface.theme"_i18n);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.interface.theme.help"_i18n);
    ImGui::Indent();
    if (ImGui::Combo("##Theme", &t_current, themes.data(), themes.size())) {
      data.Interface.Theme = tolower(themes[t_current]);
      appliers.push_back(
          [&]() { mpv->commandv("script-message-to", "implay", "theme", data.Interface.Theme.c_str(), nullptr); });
    }
    ImGui::Unindent();

    const char *s_items[] = {"0.5x", "1.0x", "1.5x", "2.0x", "3.0x", "4.0x"};
    static int s_current = -1;
    for (int i = 0; i < IM_ARRAYSIZE(s_items); i++) {
      if (std::stof(s_items[i]) == data.Interface.Scale) s_current = i;
    }
    ImGui::TextUnformatted("views.settings.interface.scale"_i18n);
    ImGui::Indent();
    auto nativeStr = "views.settings.interface.scale.native"_i18n;
    if (ImGui::BeginCombo("##Scale", s_current >= 0 ? s_items[s_current] : nativeStr)) {
      bool is_selected = (s_current == -1);
      if (ImGui::Selectable(nativeStr, is_selected)) {
        s_current = -1;
        data.Interface.Scale = 0;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();
      for (int i = 0; i < IM_ARRAYSIZE(s_items); i++) {
        is_selected = (s_current == i);
        if (ImGui::Selectable(s_items[i], is_selected)) {
          s_current = i;
          data.Interface.Scale = std::stof(s_items[s_current]);
        }
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}

void Settings::drawFontTab() {
  static char fontPath[256] = {0};
  strncpy(fontPath, data.Font.Path.c_str(), IM_ARRAYSIZE(fontPath));
  if (ImGui::BeginTabItem("views.settings.font"_i18n)) {
    ImGui::TextUnformatted("views.settings.font.path"_i18n);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.font.path.help"_i18n);
    ImGui::Indent();
    if (ImGui::InputText("##Path", fontPath, IM_ARRAYSIZE(fontPath))) data.Font.Path = fontPath;
    ImGui::SameLine();
    static std::string error;
    if (ImGui::IsWindowAppearing()) error = "";
    if (ImGui::Button(ICON_FA_FOLDER_OPEN)) {
      try {
        if (auto res = NFD::openFile({{"Font Files", "ttf,ttc,otf"}})) {
          data.Font.Path = res->string();
          strncpy(fontPath, data.Font.Path.c_str(), IM_ARRAYSIZE(fontPath));
        }
      } catch (const std::exception &e) {
        error = e.what();
      }
    }
    if (error != "") ImGui::TextWrapped("Error: %s", error.c_str());
    ImGui::Unindent();
    ImGui::TextUnformatted("views.settings.font.size"_i18n);
    ImGui::Indent();
    ImGui::SliderInt("##Size", &data.Font.Size, 8, 72);
    ImGui::Unindent();
    ImGui::TextUnformatted("views.settings.font.glyph_ranges"_i18n);
    ImGui::SameLine();
    ImGui::HelpMarker("views.settings.font.glyph_ranges.help"_i18n);
    ImGui::Indent();
    ImGui::CheckboxFlags("views.settings.font.glyph_ranges.chinese"_i18n, &data.Font.GlyphRange,
                         Config::GlyphRange_Chinese);
    ImGui::SameLine();
    ImGui::CheckboxFlags("views.settings.font.glyph_ranges.cyrillic"_i18n, &data.Font.GlyphRange,
                         Config::GlyphRange_Cyrillic);
    ImGui::SameLine();
    ImGui::CheckboxFlags("views.settings.font.glyph_ranges.japanese"_i18n, &data.Font.GlyphRange,
                         Config::GlyphRange_Japanese);
    ImGui::CheckboxFlags("views.settings.font.glyph_ranges.korean"_i18n, &data.Font.GlyphRange,
                         Config::GlyphRange_Korean);
    ImGui::SameLine();
    ImGui::CheckboxFlags("views.settings.font.glyph_ranges.thai"_i18n, &data.Font.GlyphRange, Config::GlyphRange_Thai);
    ImGui::SameLine();
    ImGui::CheckboxFlags("views.settings.font.glyph_ranges.vietnamese"_i18n, &data.Font.GlyphRange,
                         Config::GlyphRange_Vietnamese);
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}
}  // namespace ImPlay::Views