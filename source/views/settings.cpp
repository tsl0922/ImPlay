#include <string>
#include <fmt/format.h>
#include <fonts/fontawesome.h>
#include "views/settings.h"
#include "helpers.h"

namespace ImPlay::Views {
Settings::Settings(Config *config, Mpv *mpv) : View() {
  this->config = config;
  this->mpv = mpv;
}

void Settings::draw() {
  if (!m_open) return;
  ImGui::OpenPopup("Settings");
  ImGui::SetNextWindowSize(ImVec2(850, 400), ImGuiCond_FirstUseEver);
  if (ImGui::BeginPopupModal("Settings", &m_open)) {
    ImGui::TextWrapped("* Changes will take effect after restart.");
    if (ImGui::BeginTabBar("Settings")) {
      drawGeneralTab();
      drawInterfaceTab();
      drawFontTab();
      ImGui::EndTabBar();
    }
    ImGui::EndPopup();
  }
  if (!m_open) config->save();
}

void Settings::drawGeneralTab() {
  if (ImGui::BeginTabItem("General")) {
    ImGui::Text("MPV");
    ImGui::Indent();
    if (ImGui::Checkbox("Use mpv's config folder", &config->mpvConfig)) {
      mpv->property("config-dir", config->mpvConfig ? "" : Helpers::getDataDir());
      config->save();
    }
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}

void Settings::drawInterfaceTab() {
  if (ImGui::BeginTabItem("Interface")) {
    const char *t_items[] = {"Dark", "Light", "Classic"};
    static int t_current;
    for (int i = 0; i < IM_ARRAYSIZE(t_items); i++) {
      if (Helpers::tolower(t_items[i]) == config->Theme) t_current = i;
    }
    ImGui::Text("Theme");
    ImGui::Indent();
    if (ImGui::Combo("##Theme", &t_current, t_items, IM_ARRAYSIZE(t_items))) {
      config->Theme = Helpers::tolower(t_items[t_current]);
      config->save();
      mpv->commandv("script-message-to", "implay", "theme", config->Theme.c_str(), nullptr);
    }
    ImGui::Unindent();

    const char *s_items[] = {"Native", "0.5x", "1.0x", "1.5x", "2.0x", "3.0x", "4.0x"};
    static int s_current;
    static auto s_to_value = [](const char *s) -> float {
      if (strcmp(s, "Native") == 0) return 0;
      return std::stof(s);
    };
    for (int i = 0; i < IM_ARRAYSIZE(s_items); i++) {
      if (s_to_value(s_items[i]) == config->Scale) s_current = i;
    }
    ImGui::Text("Scale*");
    ImGui::Indent();
    if (ImGui::Combo("##Scale", &s_current, s_items, IM_ARRAYSIZE(s_items))) {
      config->Scale = s_to_value(s_items[s_current]);
    }
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}

void Settings::drawFontTab() {
  static char fontPath[256] = {0};
  strncpy(fontPath, config->FontPath.c_str(), IM_ARRAYSIZE(fontPath));
  if (ImGui::BeginTabItem("Font")) {
    ImGui::Text("Path*");
    ImGui::Indent();
    if (ImGui::InputText("##Path", fontPath, IM_ARRAYSIZE(fontPath))) config->FontPath = fontPath;
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FOLDER_OPEN)) {
      openFile({}, [&](const char *path) {
        config->FontPath = path;
        strncpy(fontPath, path, IM_ARRAYSIZE(fontPath));
      });
    }
    ImGui::Unindent();
    ImGui::Text("Size*");
    ImGui::Indent();
    ImGui::SliderInt("##Size", &config->FontSize, 8, 72);
    ImGui::Unindent();
    ImGui::Text("Glyph Ranges*");
    ImGui::Indent();
    ImGui::CheckboxFlags("Chinese", &config->glyphRange, Config::GlyphRange_Chinese);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Cyrillic", &config->glyphRange, Config::GlyphRange_Cyrillic);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Japanese", &config->glyphRange, Config::GlyphRange_Japanese);
    ImGui::CheckboxFlags("Korean", &config->glyphRange, Config::GlyphRange_Korean);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Thai", &config->glyphRange, Config::GlyphRange_Thai);
    ImGui::SameLine();
    ImGui::CheckboxFlags("Vietnamese", &config->glyphRange, Config::GlyphRange_Vietnamese);
    ImGui::Unindent();
    ImGui::EndTabItem();
  }
}
}  // namespace ImPlay::Views