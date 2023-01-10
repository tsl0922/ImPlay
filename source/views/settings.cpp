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
    if (ImGui::Checkbox("Use mpv's config dir", &config->mpvConfig)) {
      const char *configDir = config->mpvConfig ? Helpers::getDataDir("mpv") : Helpers::getDataDir();
      mpv->property("config-dir", configDir);
      mpv->loadConfig(fmt::format("{}/mpv.conf", configDir).c_str());
      config->save();
    }
    ImGui::SameLine();
    Helpers::marker("ImPlay will use it's own config dir for libmpv by default.");
    if (ImGui::Checkbox("Remember playback progress on exit", &config->watchLater)) config->save();
    ImGui::SameLine();
    Helpers::marker("Exit mpv with the quit-watch-later command.");
    ImGui::Unindent();
    ImGui::Text("Debug");
    ImGui::Indent();
    const char *items[] = {"fatal", "error", "warn", "info", "v", "debug", "trace", "no"};
    static int current;
    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
      if (strcmp(items[i], config->logLevel.c_str()) == 0) current = i;
    }
    if (ImGui::Combo("Log Level*", &current, items, IM_ARRAYSIZE(items))) {
      config->logLevel = items[current];
      config->save();
    }
    ImGui::SameLine();
    Helpers::marker(
        "Controls the log level used on startup.\n"
        "It can be changed later in debug window, but won't be saved.");
    if (ImGui::InputInt("Log Limit*", &config->logLimit, 0)) config->save();
    ImGui::SameLine();
    Helpers::marker(
        "Controls the log limit used on startup.\n"
        "It can be changed later in debug window, but won't be saved.");
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
    ImGui::SameLine();
    Helpers::marker("Color theme of the interface.");
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
    ImGui::SameLine();
    Helpers::marker("An embedded font will be used if not specified.");
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
    ImGui::SameLine();
    Helpers::marker("Required for displaying non-English characters on interface.");
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