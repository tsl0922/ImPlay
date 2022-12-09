#include <imgui.h>
#include <imgui_internal.h>
#include "views/about.h"

namespace ImPlay::Views {
About::About() {}

About::~About() {}

void About::draw() {
  if (open) ImGui::OpenPopup("About");
  ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_Always);
  if (ImGui::BeginPopupModal("About", &open, ImGuiWindowFlags_NoResize)) {
    ImGui::Text("ImPlay");
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::TextWrapped("ImPlay is a cross-platform desktop media player.");
    ImGui::EndPopup();
  }
}

void About::show() { open = true; }
}  // namespace ImPlay::Views