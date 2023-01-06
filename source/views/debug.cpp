#include <chrono>
#include <map>
#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/format.h>
#include "views/debug.h"
#include "mpv.h"

namespace ImPlay::Views {
Debug::Debug(Mpv* mpv) : View() { this->mpv = mpv; }

void Debug::draw() {
  ImGui::SetNextWindowSize(ImVec2(800, 450), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Metrics/Debug", &m_open)) {
    drawVersion();
    drawBindings();
    drawProperties();
    ImGui::End();
  }
}

void Debug::drawVersion() {
  ImGuiIO& io = ImGui::GetIO();
  char *version = mpv->property("mpv-version");
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15.0f, 15.0f));
  ImGui::Text("%s", version);
  ImGui::SameLine(ImGui::GetWindowWidth() - 300);
  ImGui::Text("ImGui %s", ImGui::GetVersion());
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0, 0, 1.0f, 1.0f), "FPS: %.2f", io.Framerate);
  ImGui::PopStyleVar();
  mpv_free(version);
}

void Debug::drawBindings() {
  auto bindings = mpv->bindingList();
  if (ImGui::CollapsingHeader(fmt::format("Bindings ({})", bindings.size()).c_str())) {
    if (ImGui::BeginListBox("##mpv-bindings", ImVec2(-FLT_MIN, -FLT_MIN))) {
      for (auto& binding : bindings) {
        std::string title = binding.comment;
        if (title.empty()) title = binding.cmd;
        title = title.substr(0, 50);
        if (title.size() == 50) title += "...";

        ImGui::PushID(&binding);
        ImGui::Selectable("", false);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", binding.cmd.c_str());

        ImGui::SameLine();
        ImGui::Text("%s", title.c_str());

        ImGui::SameLine(ImGui::GetWindowWidth() * 0.75f);
        ImGui::BeginDisabled();
        ImGui::Button(binding.key.c_str());
        ImGui::EndDisabled();
        ImGui::PopID();
      }
      ImGui::EndListBox();
    }
  }
}

void Debug::drawProperties() {
  mpv_node node = mpv->property<mpv_node, MPV_FORMAT_NODE>("property-list");
  if (ImGui::CollapsingHeader(fmt::format("Properties ({})", node.u.list->num).c_str())) {
    if (ImGui::BeginListBox("##mpv-properties", ImVec2(-FLT_MIN, -FLT_MIN))) {
      auto color = ImVec4(0, 0, 1.0f, 1.0f);
      for (int i = 0; i < node.u.list->num; i++) {
        auto item = node.u.list->values[i];
        ImGui::PushID(&item);
        ImGui::Selectable("", false);
        ImGui::SameLine();
        ImGui::Text("%s", item.u.string);
        if (!ImGui::IsItemVisible()) {
          ImGui::PopID();
          continue;
        }

        ImGui::SameLine(ImGui::GetWindowWidth() * 0.4f);
        auto prop = mpv->property<mpv_node, MPV_FORMAT_NODE>(item.u.string);
        switch (prop.format) {
          case MPV_FORMAT_NONE:
            ImGui::TextColored(color, "Invalid");
            break;
          case MPV_FORMAT_STRING:
            ImGui::TextColored(color, "%s", prop.u.string);
            break;
          case MPV_FORMAT_FLAG:
            ImGui::TextColored(color, "%s", prop.u.flag ? "true" : "false");
            break;
          case MPV_FORMAT_INT64:
            ImGui::TextColored(color, "%lld", prop.u.int64);
            break;
          case MPV_FORMAT_DOUBLE:
            ImGui::TextColored(color, "%f", prop.u.double_);
            break;
          case MPV_FORMAT_NODE_ARRAY:
            ImGui::TextColored(color, "array: %d", prop.u.list->num);
            break;
          case MPV_FORMAT_NODE_MAP:
            ImGui::TextColored(color, "map: %d", prop.u.list->num);
            break;
          case MPV_FORMAT_BYTE_ARRAY:
            ImGui::TextColored(color, "byte array: %d", prop.u.ba->size);
            break;
          default:
            ImGui::TextColored(color, "Unknown format: %d", prop.format);
            break;
        }
        ImGui::PopID();
        mpv_free_node_contents(&prop);
      }
      ImGui::EndListBox();
    }
  }
  mpv_free_node_contents(&node);
}
}  // namespace ImPlay::Views