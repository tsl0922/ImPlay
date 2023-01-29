// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "views/about.h"
#include "helpers.h"

namespace ImPlay::Views {
void About::draw() {
  if (m_open) ImGui::OpenPopup("About");

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scaled(ImVec2(4.0f, 1.0f)));
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("About", &m_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_open = false;
    ImGui::TextCentered("ImPlay");
#ifdef APP_VERSION
    ImGui::TextCentered(APP_VERSION, true);
#endif
    ImGui::Spacing();
    ImGui::TextCentered("A Cross-Platform Desktop Media Player");
    ImGui::NewLine();
    const char *link = "https://github.com/tsl0922/ImPlay";
    ImGui::HalignCenter(link);
    ImGui::Hyperlink(nullptr, link);
    ImGui::Spacing();
    ImGui::TextCentered("Copyright (C) 2022 tsl0922");
    ImGui::Spacing();
    ImGui::TextCentered("GPL-2.0 License", true);
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
}
}  // namespace ImPlay::Views