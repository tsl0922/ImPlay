#include <imgui.h>
#include <imgui_internal.h>
#include <fonts/fontawesome.h>
#include <fmt/format.h>
#include "views/debug.h"
#include "mpv.h"

namespace ImPlay::Views {
Debug::Debug(Mpv* mpv) : View() { this->mpv = mpv; }

void Debug::draw() {
  ImGui::SetNextWindowSize(ImVec2(1000, 550), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Metrics/Debug", &m_open)) {
    drawHeader();
    drawProperties("Options", "options");
    drawProperties("Properties", "property-list");
    drawBindings();
    ImGui::End();
  }
}

void Debug::drawHeader() {
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

  ImGui::TextWrapped("NOTE: playback may become very slow when Properties / Options are expanded.");
  ImGui::Spacing();
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

  int mask = 1 << MPV_FORMAT_NONE | 1 << MPV_FORMAT_STRING | 1 << MPV_FORMAT_OSD_STRING | 1 << MPV_FORMAT_FLAG |
             1 << MPV_FORMAT_INT64 | 1 << MPV_FORMAT_DOUBLE | 1 << MPV_FORMAT_NODE | 1 << MPV_FORMAT_NODE_ARRAY |
             1 << MPV_FORMAT_NODE_MAP | 1 << MPV_FORMAT_BYTE_ARRAY;
  static int format = mask;
  static char buf[256] = "";
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Format:");
  ImGui::SameLine();
  ImGui::CheckboxFlags("ALL", &format, mask);
  ImGui::SameLine();
  ImGui::CheckboxFlags("NONE", &format, 1 << MPV_FORMAT_NONE);
  ImGui::Indent();
  ImGui::CheckboxFlags("STRING", &format, 1 << MPV_FORMAT_STRING);
  ImGui::SameLine();
  ImGui::CheckboxFlags("OSD_STRNG", &format, 1 << MPV_FORMAT_OSD_STRING);
  ImGui::SameLine();
  ImGui::CheckboxFlags("FLAG", &format, 1 << MPV_FORMAT_FLAG);
  ImGui::SameLine();
  ImGui::CheckboxFlags("INT64", &format, 1 << MPV_FORMAT_INT64);
  ImGui::SameLine();
  ImGui::CheckboxFlags("DOUBLE", &format, 1 << MPV_FORMAT_DOUBLE);
  ImGui::SameLine();
  ImGui::CheckboxFlags("NODE_ARRAY", &format, 1 << MPV_FORMAT_NODE_ARRAY);
  ImGui::SameLine();
  ImGui::CheckboxFlags("NODE_MAP", &format, 1 << MPV_FORMAT_NODE_MAP);
  ImGui::SameLine();
  ImGui::CheckboxFlags("BYTE_ARRAY", &format, 1 << MPV_FORMAT_BYTE_ARRAY);
  ImGui::Unindent();
  ImGui::Text("Filter:");
  ImGui::SameLine();
  ImGui::PushItemWidth(-1);
  ImGui::InputText("Filter", buf, IM_ARRAYSIZE(buf));
  ImGui::PopItemWidth();

  if (format > 0 && ImGui::BeginListBox("##mpv-prop-list", ImVec2(-FLT_MIN, -FLT_MIN))) {
    for (int i = 0; i < node.u.list->num; i++) {
      auto item = node.u.list->values[i];
      if (buf[0] != '\0' && !strstr(item.u.string, buf)) continue;
      auto prop = mpv->property<mpv_node, MPV_FORMAT_NODE>(item.u.string);
      if (format & 1 << prop.format) drawPropNode(item.u.string, prop);
      mpv_free_node_contents(&prop);
    }
    ImGui::EndListBox();
  }

  mpv_free_node_contents(&node);
}

void Debug::drawPropNode(const char* name, mpv_node& node, int depth) {
  constexpr static auto drawSimple = [&](const char* title, mpv_node prop) {
    std::string value;
    ImVec4 color = ImVec4(0, 0, 1.0f, 1.0f);
    switch (prop.format) {
      case MPV_FORMAT_NONE:
        value = "Invalid";
        color = ImVec4(1.0f, 0, 0, 1.0f);
        break;
      case MPV_FORMAT_OSD_STRING:
      case MPV_FORMAT_STRING:
        value = prop.u.string;
        break;
      case MPV_FORMAT_FLAG:
        value = prop.u.flag ? "yes" : "no";
        break;
      case MPV_FORMAT_INT64:
        value = fmt::format("{}", prop.u.int64);
        break;
      case MPV_FORMAT_DOUBLE:
        value = fmt::format("{}", prop.u.double_);
        break;
      default:
        value = fmt::format("Unknown format: {}", (int)prop.format);
        color = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
        break;
    }
    ImGui::PushID(&prop);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4.0f));
    ImGui::Selectable("", false);
    if (ImGui::BeginPopupContextItem(fmt::format("##context_menu_{}", title).c_str())) {
      if (ImGui::MenuItem("Copy")) ImGui::SetClipboardText(fmt::format("{}={}", title, value).c_str());
      if (ImGui::MenuItem("Copy Name")) ImGui::SetClipboardText(title);
      if (ImGui::MenuItem("Copy value")) ImGui::SetClipboardText(value.c_str());
      ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::BulletText("%s", title);
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.4f);
    ImGui::TextColored(color, "%s", value.c_str());
    ImGui::PopStyleVar();
    ImGui::PopID();
  };

  switch (node.format) {
    case MPV_FORMAT_NONE:
    case MPV_FORMAT_STRING:
    case MPV_FORMAT_OSD_STRING:
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