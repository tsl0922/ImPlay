// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <algorithm>
#include <cstring>
#include "helpers/utils.h"
#include "helpers/imgui.h"
#include "views/command_palette.h"

namespace ImPlay::Views {
CommandPalette::CommandPalette(Config* config, Mpv* mpv) : View(config, mpv) {
  providers["bindings"] = [=, this](const char*) {
    for (auto& item : mpv->bindings)
      items.push_back({
          item.comment,
          item.cmd,
          item.key,
          -1,
          [=, this]() { mpv->command(item.cmd); },
      });
    pos = 0;
  };
  providers["chapters"] = [=, this](const char*) {
    for (auto& item : mpv->chapters) {
      auto title = item.title.empty() ? fmt::format("Chapter {}", item.id + 1) : item.title;
      auto time = fmt::format("{:%H:%M:%S}", std::chrono::duration<int>((int)item.time));
      items.push_back({
          title,
          "",
          time,
          item.id,
          [=, this]() { mpv->commandv("seek", std::to_string(item.time).c_str(), "absolute", nullptr); },
      });
    }
    pos = mpv->chapter;
  };
  providers["playlist"] = [=, this](const char*) {
    for (auto& item : mpv->playlist) {
      std::string title = item.title;
      if (title.empty() && !item.filename().empty()) title = item.filename();
      if (title.empty()) title = fmt::format("Item {}", item.id + 1);
      items.push_back({
          title,
          item.path.string(),
          "",
          item.id,
          [=, this]() { mpv->commandv("playlist-play-index", std::to_string(item.id).c_str(), nullptr); },
      });
    }
    pos = mpv->playlistPos;
  };
  providers["tracks"] = [=, this](const char* type) {
    for (auto& item : mpv->tracks) {
      if (type != nullptr && item.type != type) continue;
      auto title = item.title.empty() ? fmt::format("Track {}", item.id) : item.title;
      if (!item.lang.empty()) title += fmt::format(" [{}]", item.lang);
      items.push_back({
          title,
          "",
          toupper(item.type),
          item.id,
          [=, this]() {
            if (item.type == "audio")
              mpv->property<int64_t, MPV_FORMAT_INT64>("aid", item.id);
            else if (item.type == "video")
              mpv->property<int64_t, MPV_FORMAT_INT64>("vid", item.id);
            else if (item.type == "sub")
              mpv->property<int64_t, MPV_FORMAT_INT64>("sid", item.id);
          },
      });
    }
    pos = 0;
  };
  providers["history"] = [=, this](const char*) {
    for (auto& file : config->getRecentFiles()) {
      items.push_back({
          file.title,
          file.path,
          "",
          -1,
          [=, this]() { mpv->commandv("loadfile", file.path.c_str(), nullptr); },
      });
    }
    pos = 0;
  };
}

void CommandPalette::show(int n, const char** args) {
  std::string target = "bindings";
  if (n > 0) target = args[0];
  if (!providers.contains(target)) return;
  auto callback = providers[target];

  items.clear();
  callback(n > 1 ? args[1] : nullptr);

  View::show();
}

void CommandPalette::draw() {
  if (items.empty()) return;
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
    ImGui::SetKeyboardFocusHere(0);
    focusInput = false;
  }

  ImGui::PushItemWidth(-1);
  ImGui::InputTextWithHint(
      "##command_input", "views.command_palette.tip"_i18n, buffer.data(), buffer.size(),
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
  ImGui::BeginChild("##command_matches", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NavFlattened);
  float maxWidth = 0;
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
    float lWidth = contentAvail.x;
    auto rWidth = maxWidth + 2 * style.ItemInnerSpacing.x + style.ItemSpacing.x;
    if (rWidth > 0) lWidth -= rWidth;

    ImGui::PushID(&match);
    ImGui::SetNextItemWidth(lWidth);
    if (ImGui::Selectable("", false, ImGuiSelectableFlags_DontClosePopups)) {
      pos = match.id;
      match.callback();
    }
    if (!match.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
      ImGui::SetTooltip("%s", match.tooltip.c_str());
    ImGui::SameLine();

    bool selected = pos > 0 && match.id == pos;
    auto color = ImGui::GetStyleColorVec4(selected ? ImGuiCol_CheckMark : ImGuiCol_Text);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextEllipsis(title.c_str(), contentAvail.x - rWidth - style.ItemSpacing.x);
    if (ImGui::IsWindowAppearing() && selected) ImGui::SetScrollHereY(0.25f);
    ImGui::PopStyleColor();

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
    matches = items;
    return;
  }

  std::vector<std::pair<CommandItem, int>> result;

  for (const auto& item : items) {
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