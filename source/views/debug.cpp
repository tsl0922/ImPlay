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
    drawProperties("Properties", "property-list");
    drawProperties("Options", "options");
    ImGui::End();
  }
}

void Debug::drawVersion() {
  ImGuiIO& io = ImGui::GetIO();
  char* version = mpv->property("mpv-version");
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
  if (ImGui::CollapsingHeader(fmt::format("Bindings [{}]", bindings.size()).c_str())) {
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

void Debug::drawProperties(const char* title, const char* key) {
  mpv_node node = mpv->property<mpv_node, MPV_FORMAT_NODE>(key);
  if (!ImGui::CollapsingHeader(fmt::format("{} [{}]", title, node.u.list->num).c_str())) return;

  if (ImGui::BeginListBox("##mpv-prop-list", ImVec2(-FLT_MIN, -FLT_MIN))) {
    for (int i = 0; i < node.u.list->num; i++) {
      auto item = node.u.list->values[i];
      auto prop = mpv->property<mpv_node, MPV_FORMAT_NODE>(item.u.string);
      drawPropNode(item.u.string, prop);
      mpv_free_node_contents(&prop);
    }
    ImGui::EndListBox();
  }
  
  mpv_free_node_contents(&node);
}

void Debug::drawPropNode(const char* name, mpv_node& node, int depth) {
  static const ImVec4 color = ImVec4(0, 0, 1.0f, 1.0f);
  constexpr static auto drawSimple = [&](const char* title, mpv_node prop) {
    ImGui::PushID(&prop);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4.0f));
    ImGui::Selectable("", false);
    ImGui::SameLine();
    ImGui::BulletText("%s", title);
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.4f);
    switch (prop.format) {
      case MPV_FORMAT_NONE:
        ImGui::TextColored(ImVec4(1.0f, 0, 0, 1.0f), "Invalid");
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
      default:
        ImGui::TextDisabled("Unknown format: %d", prop.format);
        break;
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
  };

  switch (node.format) {
    case MPV_FORMAT_NONE:
    case MPV_FORMAT_STRING:
    case MPV_FORMAT_FLAG:
    case MPV_FORMAT_INT64:
    case MPV_FORMAT_DOUBLE:
      drawSimple(name, node);
      break;
    case MPV_FORMAT_NODE_ARRAY:
      if (ImGui::TreeNode(fmt::format("{} [{}]", name, node.u.list->num).c_str())) {
        for (int i = 0; i < node.u.list->num; i++)
          drawPropNode(fmt::format("#{}", i).c_str(), node.u.list->values[i], depth + 1);
        ImGui::TreePop();
      }
      break;
    case MPV_FORMAT_NODE_MAP:
      if (depth > 0) ImGui::SetNextItemOpen(true, ImGuiCond_Once);
      if (ImGui::TreeNode(fmt::format("{} ({})", name, node.u.list->num).c_str())) {
        for (int i = 0; i < node.u.list->num; i++) drawSimple(node.u.list->keys[i], node.u.list->values[i]);
        ImGui::TreePop();
      }
      break;
    case MPV_FORMAT_BYTE_ARRAY:
      ImGui::BulletText("byte array [%d]", node.u.ba->size);
      break;
    default:
      ImGui::BulletText("Unknown format: %d", node.format);
      break;
  }
}
}  // namespace ImPlay::Views