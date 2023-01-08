#include <imgui.h>
#include <imgui_internal.h>
#include "views/about.h"
#include "helpers.h"

namespace ImPlay::Views {
void About::draw() {
  if (m_open) ImGui::OpenPopup("About");

  ImGui::SetNextWindowSize(ImVec2(600, 200));
  if (ImGui::BeginPopupModal("About", &m_open, ImGuiWindowFlags_NoResize)) {
    textCentered("ImPlay");
    ImGui::Spacing();
    textCentered("A Cross-Platform Desktop Media Player");
    ImGui::NewLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    textCentered("https://github.com/tsl0922/ImPlay");
    if (ImGui::IsItemClicked()) Helpers::openUri("https://github.com/tsl0922/ImPlay");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    textCentered("Copyright (C) 2022-2023 Shuanglei Tao");
    ImGui::Spacing();
    textCentered("GPL-2.0 License");
    ImGui::EndPopup();
  }
}

void About::textCentered(const char* text) {
  auto size = ImGui::CalcTextSize(text);
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - size.x) / 2);
  ImGui::Text("%s", text);
}
}  // namespace ImPlay::Views