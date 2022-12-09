#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstring>
#include "views/command_palette.h"

namespace ImPlay::Views {
CommandPalette::CommandPalette(Mpv* mpv) { this->mpv = mpv; }

CommandPalette::~CommandPalette() {}

void CommandPalette::show() { open = true; }

void CommandPalette::draw() {
  if (open) {
    ImGui::OpenPopup("##command_palette");
    open = false;
    justOpened = true;
  }

  auto viewport = ImGui::GetMainViewport();
  auto pos = viewport->Pos;
  auto size = viewport->Size;
  auto width = size.x * 0.5f;
  auto height = size.y * 0.45f;

  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(pos.x + size.x * 0.5f, pos.y + 50.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));

  if (ImGui::BeginPopup("##command_palette")) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();

    if (justOpened) {
      focusInput = true;
      match("");
      std::memset(buffer.data(), 0x00, buffer.size());
      justOpened = false;
    }

    drawInput();
    ImGui::Separator();
    drawList(width);

    ImGui::EndPopup();
  }
}

void CommandPalette::drawInput() {
  if (focusInput) {
    auto textState = ImGui::GetInputTextState(ImGui::GetID("##command_input"));
    if (textState != nullptr) textState->Stb.cursor = strlen(buffer.data());
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
  auto lWidth = width * 0.75f;
  auto rWidth = width * 0.25f;
  if (rWidth < 200.0f) rWidth = 200.0f;

  ImGui::BeginChild("##command_matches", ImVec2(0, 0), false, ImGuiWindowFlags_NavFlattened);
  for (const auto& match : matches) {
    std::string title = match.comment;
    if (title.empty()) title = match.command;
    ImGui::SetNextItemWidth(lWidth);
    int charLimit = lWidth / ImGui::GetFontSize();
    title = title.substr(0, charLimit);
    if (title.size() == charLimit) title += "...";

    ImGui::PushID(&match);
    if (ImGui::Selectable("", false, ImGuiSelectableFlags_DontClosePopups)) mpv->command(match.command.c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", match.command.c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 6.0f));

    if (!match.input.empty()) {
      size_t pos = 0;
      size_t len = match.input.length();
      while ((pos = title.find(match.input)) != std::string::npos) {
        ImGui::SameLine();
        ImGui::Text("%s", title.substr(0, pos).c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "%s", match.input.c_str());
        title.erase(0, pos + len);
      }
    }
    ImGui::SameLine();
    ImGui::Text("%s", title.c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() - rWidth);
    ImGui::BeginDisabled();
    ImGui::Button(match.key.c_str());
    ImGui::EndDisabled();

    ImGui::PopStyleVar();
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
    if (text.starts_with(input)) return 3;
    if (text.find(input) != std::string::npos) return 2;
    return 0;
  };

  matches.clear();
  auto bindinglist = mpv->bindinglist();

  for (auto& binding : bindinglist) {
    std::string key = binding.key ? binding.key : "";
    std::string command = binding.cmd ? binding.cmd : "";
    std::string comment = binding.comment ? binding.comment : "";
    if (command.empty() || command == "ignore") continue;
    int score = MatchCommand(input, comment) * 2;
    if (score == 0) score = MatchCommand(input, command);
    if (score > 0) matches.push_back({key, command, comment, input, score});
  }
  if (input.empty()) return;

  std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
    if (a.score != b.score) return a.score > b.score;
    if (a.comment != b.comment) return a.comment < b.comment;
    return a.command < b.command;
  });
}
}  // namespace ImPlay::Views