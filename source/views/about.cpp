// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "views/about.h"
#include "helpers.h"

namespace ImPlay::Views {
void About::draw() {
  if (m_open) ImGui::OpenPopup("views.about.title"_i18n);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, scaled(ImVec2(4.0f, 1.0f)));
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("views.about.title"_i18n, &m_open, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_open = false;
    ImGui::TextCentered("ImPlay");
#ifdef APP_VERSION
    ImGui::TextCentered(APP_VERSION, true);
#endif
    ImGui::Spacing();
    ImGui::TextCentered("views.about.desc"_i18n);
    ImGui::NewLine();
    const char *link = "https://github.com/tsl0922/ImPlay";
    ImGui::HalignCenter(link);
    ImGui::Hyperlink(nullptr, link);
    ImGui::Spacing();
    ImGui::TextCentered("views.about.copyright"_i18n);
    ImGui::Spacing();
    ImGui::TextCentered("GPL-2.0 License", true);
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
}
}  // namespace ImPlay::Views