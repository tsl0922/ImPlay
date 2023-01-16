// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <cstring>
#include "views/command_palette.h"
#include "helpers.h"

namespace ImPlay::Views {
CommandPalette::CommandPalette(Mpv* mpv) : View() { this->mpv = mpv; }

void CommandPalette::draw() {
  if (items_.empty()) return;
  if (m_open) {
    ImGui::OpenPopup("##command_palette");
    m_open = false;
    justOpened = true;
  }

  auto pos = ImGui::GetMainViewport()->Pos;
  auto size = ImGui::GetMainViewport()->Size;
  ImVec2 popupSize = ImVec2(size.x * 0.5f, size.y * 0.45f);
  ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.1), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
  if (ImGui::BeginPopup("##command_palette")) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape) || ImGui::GetIO().AppFocusLost ||
        ImGui::GetWindowViewport()->Flags & ImGuiViewportFlags_Minimized)
      ImGui::CloseCurrentPopup();

    if (justOpened) {
      focusInput = true;
      match("");
      std::memset(buffer.data(), 0x00, buffer.size());
      justOpened = false;
    }

    drawInput();
    ImGui::Separator();
    drawList(popupSize.x);

    ImGui::EndPopup();
  }
}

void CommandPalette::drawInput() {
  if (focusInput) {
    auto textState = ImGui::GetInputTextState(ImGui::GetID("##command_input"));
    if (textState != nullptr) textState->Stb.cursor = (int)strlen(buffer.data());
    ImGui::SetKeyboardFocusHere(0);
    focusInput = false;
  }

  ImGui::PushItemWidth(-1);
  ImGui::InputTextWithHint(
      "##command_input", "TIP: Press SPACE to select result", buffer.data(), buffer.size(),
      ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_EnterReturnsTrue,
      [](ImGuiInputTextCallbackData* data) -> int {
        auto cp = static_cast<CommandPalette*>(data->UserData);
        cp->match(data->Buf);
        cp->filtered = true;
        return 0;
      },
      this);
  ImGui::PopItemWidth();

  if (ImGui::IsItemFocused() && ImGui::IsKeyReleased(ImGuiKey_UpArrow)) focusInput = true;
}

void CommandPalette::drawList(float width) {
  ImGui::BeginChild("##command_matches", ImVec2(0, 0), false, ImGuiWindowFlags_NavFlattened);
  long maxWidth = 0;
  for (const auto& match : matches) {
    auto width = ImGui::CalcTextSize(match.label.c_str()).x;
    if (width > maxWidth) maxWidth = width;
  }
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImGuiStyle style = ImGui::GetStyle();
  for (const auto& match : matches) {
    std::string title = match.title;
    if (title.empty()) title = match.tooltip;
    ImVec2 contentAvail = ImGui::GetContentRegionAvail();
    long lWidth = contentAvail.x;
    auto rWidth = maxWidth + 2 * style.ItemSpacing.x + 2 * style.ItemInnerSpacing.x;
    if (rWidth > 0) lWidth -= rWidth;

    ImGui::PushID(&match);
    ImGui::SetNextItemWidth(lWidth);
    if (ImGui::Selectable("", false, ImGuiSelectableFlags_DontClosePopups)) match.callback();
    if (!match.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
      ImGui::SetTooltip("%s", match.tooltip.c_str());
    ImGui::SameLine();

    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 min = ImGui::GetCursorScreenPos() + style.FramePadding;
    ImVec2 max = min + ImVec2(contentAvail.x - rWidth - 2 * style.ItemSpacing.x, textSize.y + style.FramePadding.y);
    ImRect textRect(min, max);
    ImGui::ItemSize(textRect);
    if (ImGui::ItemAdd(textRect, window->GetID(title.data(), title.data() + title.size())))
      ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), min, max, max.x, max.x, title.data(),
                                title.data() + title.size(), &textSize);

    if (!match.label.empty()) {
      ImGui::SameLine(contentAvail.x - rWidth);
      ImGui::BeginDisabled();
      ImGui::Button(match.label.c_str());
      ImGui::EndDisabled();
    }

    ImGui::PopID();
  }
  if (filtered) {
    ImGui::SetScrollY(0.0f);
    filtered = false;
  }
  ImGui::EndChild();
}

void CommandPalette::match(const std::string& input) {
  constexpr static auto MatchCommand = [](const std::string& input, const std::string& text) -> int {
    if (input.empty()) return 1;
    auto l_text = tolower(text);
    auto l_input = tolower(input);
    if (l_text.starts_with(l_input)) return 3;
    if (l_text.find(l_input) != std::string::npos) return 2;
    return 0;
  };

  if (input.empty()) {
    matches = items_;
    return;
  }

  std::vector<std::pair<CommandItem, int>> result;

  for (const auto& item : items_) {
    int score = MatchCommand(input, item.title) * 2;
    if (score == 0) score = MatchCommand(input, item.tooltip);
    if (score > 0) result.push_back(std::make_pair(item, score));
  }
  std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    if (a.first.title != b.first.title) return a.first.title < b.first.title;
    return a.first.tooltip < b.first.tooltip;
  });

  matches.clear();
  for (const auto& [item, _] : result) matches.push_back(item);
}
}  // namespace ImPlay::Views