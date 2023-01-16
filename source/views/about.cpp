// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "views/about.h"
#include "helpers.h"

namespace ImPlay::Views {
void About::draw() {
  if (m_open) ImGui::OpenPopup("About");

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 1.0f) * ImGui::GetFontSize());
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("About", &m_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    textCentered("ImPlay");
#ifdef APP_VERSION
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    textCentered(APP_VERSION);
    ImGui::PopStyleColor();
#endif
    ImGui::Spacing();
    textCentered("A Cross-Platform Desktop Media Player");
    ImGui::NewLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    textCentered("https://github.com/tsl0922/ImPlay");
    if (ImGui::IsItemClicked()) openUri("https://github.com/tsl0922/ImPlay");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    textCentered("Copyright (C) 2022 tsl0922");
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    textCentered("GPL-2.0 License");
    ImGui::PopStyleColor();
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
}

void About::textCentered(const char* text) {
  auto size = ImGui::CalcTextSize(text);
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - size.x) / 2);
  ImGui::Text("%s", text);
}
}  // namespace ImPlay::Views