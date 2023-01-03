#include <imgui.h>
#include "views/about.h"

namespace ImPlay::Views {
void About::draw() {
  if (m_open) ImGui::OpenPopup("About");

  ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Always);
  if (ImGui::BeginPopupModal("About", &m_open, ImGuiWindowFlags_NoResize)) {
    ImGui::Text("ImPlay");
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::TextWrapped("ImPlay is a cross-platform desktop media player.");
    ImGui::EndPopup();
  }
}
}  // namespace ImPlay::Views