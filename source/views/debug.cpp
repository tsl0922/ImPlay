// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <cstdarg>
#include <cstdio>
#include <map>
#include "helpers/utils.h"
#include "helpers/imgui.h"
#include "views/debug.h"

namespace ImPlay::Views {
Debug::Debug(Config* config, Mpv* mpv) : View(config, mpv) { console = new Console(mpv); }

Debug::~Debug() { delete console; }

void Debug::init() { console->init(config->Data.Debug.LogLevel.c_str(), config->Data.Debug.LogLimit); }

void Debug::show() {
  m_open = true;
  initData();
}

void Debug::draw() {
  if (!m_open) return;
  ImVec2 wPos = ImGui::GetMainViewport()->WorkPos;
  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSizeConstraints(ImVec2(scaled(35), scaled(45)), ImVec2(FLT_MAX, FLT_MAX));
  ImGui::SetNextWindowSize(ImVec2(wSize.x * 0.4f, wSize.y * 0.8f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x * 0.2f, wPos.y + wSize.y * 0.5f), ImGuiCond_FirstUseEver,
                          ImVec2(0.2f, 0.5f));
  if (ImGui::Begin("views.debug.title"_i18n, &m_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
    drawHeader();
    drawProperties("views.debug.options"_i18n, options);
    drawProperties("views.debug.properties"_i18n, properties);
    drawBindings();
    drawCommands();
    drawConsole();
  }
  ImGui::End();
  if (m_demo) ImGui::ShowDemoWindow(&m_demo);
  if (m_metrics) ImGui::ShowMetricsWindow(&m_metrics);
}

void Debug::drawHeader() {
  ImGuiIO& io = ImGui::GetIO();
  auto style = ImGuiStyle();
  ImGui::Text("%s", version.c_str());
  auto vSize = ImGui::CalcTextSize(fmt::format("ImGui {}", ImGui::GetVersion()).c_str());
  auto mSize = ImGui::CalcTextSize(fmt::format("FPS: %.2f", io.Framerate).c_str());
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - (vSize.x + mSize.x + 4 * style.FramePadding.x));
  ImGui::Text("ImGui %s", ImGui::GetVersion());
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) m_demo = !m_demo;
  ImGui::SameLine();
  ImGui::TextColored(style.Colors[ImGuiCol_CheckMark], "FPS: %.2f", io.Framerate);
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) m_metrics = !m_metrics;
  ImGui::BeginDisabled();
  ImGui::TextUnformatted("views.debug.hint"_i18n);
  ImGui::EndDisabled();
  ImGui::Spacing();
}

void Debug::drawConsole() {
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (m_node != "Console") ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader("views.debug.console"_i18n)) return;
  m_node = "Console";
  console->draw();
}

void Debug::drawBindings() {
  auto bindings = mpv->bindings;
  if (m_node != "Bindings") ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader(i18n_a("views.debug.bindings", bindings.size()).c_str())) return;
  m_node = "Bindings";

  static char buf[256] = "";
  ImGui::TextUnformatted("views.debug.commands.filter"_i18n);
  ImGui::SameLine();
  ImGui::PushItemWidth(-1);
  ImGui::InputText("##Filter.bindings", buf, IM_ARRAYSIZE(buf));
  ImGui::PopItemWidth();

  static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                                 ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("input-bindings", 6, flags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Section", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Weak", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Command", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();
    for (auto& binding : bindings) {
      if (buf[0] != '\0' && !findCase(binding.key, buf) && !findCase(binding.cmd, buf)) continue;
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Selectable(binding.section.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
      ImGui::TableNextColumn();
      ImGui::Text("%d", binding.priority);
      ImGui::TableNextColumn();
      ImGui::Text("%s", binding.weak ? "yes" : "no");
      ImGui::TableNextColumn();
      ImGui::Text("%s", binding.key.c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%s", binding.cmd.c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%s", binding.comment.c_str());
    }
    ImGui::EndTable();
  }
}

static void formatCommands(mpv_node& node, std::vector<std::pair<std::string, std::string>>& commands) {
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    char* name = nullptr;
    std::vector<std::string> args;
    bool vararg = false;
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "name") == 0) name = value.u.string;
      if (strcmp(key, "args") == 0) {
        for (int k = 0; k < value.u.list->num; k++) {
          auto arg = value.u.list->values[k];
          char* name_;
          bool optional_ = false;
          for (int l = 0; l < arg.u.list->num; l++) {
            auto k = arg.u.list->keys[l];
            auto v = arg.u.list->values[l];
            if (strcmp(k, "name") == 0) name_ = v.u.string;
            if (strcmp(k, "optional") == 0) optional_ = v.u.flag;
          }
          args.push_back(optional_ ? fmt::format("<{}>", name_) : name_);
        }
      }
      if (strcmp(key, "vararg") == 0) vararg = value.u.flag;
    }
    if (name == nullptr) continue;
    std::string args_str;
    if (!args.empty()) {
      args_str = fmt::format("{}", join(args, " "));
      if (vararg) args_str += " ...";
    }
    commands.push_back({name, args_str});
  }
}

void Debug::initData() {
  version = mpv->property("mpv-version");

  mpv_node node = mpv->property<mpv_node, MPV_FORMAT_NODE>("options");
  options.clear();
  for (int i = 0; i < node.u.list->num; i++) options.push_back(node.u.list->values[i].u.string);
  mpv_free_node_contents(&node);

  node = mpv->property<mpv_node, MPV_FORMAT_NODE>("property-list");
  properties.clear();
  for (int i = 0; i < node.u.list->num; i++) properties.push_back(node.u.list->values[i].u.string);
  mpv_free_node_contents(&node);

  node = mpv->property<mpv_node, MPV_FORMAT_NODE>("command-list");
  commands.clear();
  formatCommands(node, commands);
  mpv_free_node_contents(&node);

  console->initCommands(commands);
}

void Debug::drawCommands() {
  if (m_node != "Commands") ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader(i18n_a("views.debug.commands", commands.size()).c_str())) return;
  m_node = "Commands";

  static char buf[256] = "";
  ImGui::TextUnformatted("views.debug.commands.filter"_i18n);
  ImGui::SameLine();
  ImGui::PushItemWidth(-1);
  ImGui::InputText("##Filter.commands", buf, IM_ARRAYSIZE(buf));
  ImGui::PopItemWidth();
  if (ImGui::BeginListBox("command-list", ImVec2(-FLT_MIN, -FLT_MIN))) {
    for (auto& [name, args] : commands) {
      if (buf[0] != '\0' && !findCase(name, buf)) continue;
      ImGui::PushID(name.c_str());
      ImGui::Selectable("", false);
      ImGui::SameLine();
      ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "%s", name.c_str());
      if (!args.empty()) {
        ImGui::SameLine();
        ImGui::Text("%s", args.c_str());
      }
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

void Debug::drawProperties(const char* title, std::vector<std::string>& props) {
  if (m_node != title) ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader(fmt::format("{} [{}]", title, props.size()).c_str())) {
    return;
  }
  m_node = title;

  int mask = 1 << MPV_FORMAT_NONE | 1 << MPV_FORMAT_STRING | 1 << MPV_FORMAT_OSD_STRING | 1 << MPV_FORMAT_FLAG |
             1 << MPV_FORMAT_INT64 | 1 << MPV_FORMAT_DOUBLE | 1 << MPV_FORMAT_NODE | 1 << MPV_FORMAT_NODE_ARRAY |
             1 << MPV_FORMAT_NODE_MAP | 1 << MPV_FORMAT_BYTE_ARRAY;
  static int format = mask;
  static char buf[256] = "";
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("views.debug.properties.format"_i18n);
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
  ImGui::CheckboxFlags("DOUBLE", &format, 1 << MPV_FORMAT_DOUBLE);
  ImGui::SameLine();
  ImGui::CheckboxFlags("NODE_ARRAY", &format, 1 << MPV_FORMAT_NODE_ARRAY);
  ImGui::SameLine();
  ImGui::CheckboxFlags("NODE_MAP", &format, 1 << MPV_FORMAT_NODE_MAP);
  ImGui::SameLine();
  ImGui::CheckboxFlags("BYTE_ARRAY", &format, 1 << MPV_FORMAT_BYTE_ARRAY);
  ImGui::Unindent();
  ImGui::TextUnformatted("views.debug.properties.filter"_i18n);
  ImGui::SameLine();
  ImGui::PushItemWidth(-1);
  ImGui::InputText("##Filter.properties", buf, IM_ARRAYSIZE(buf));
  ImGui::PopItemWidth();
  auto posY = ImGui::GetCursorScreenPos().y;
  if (format > 0 && ImGui::BeginListBox(title, ImVec2(-FLT_MIN, -FLT_MIN))) {
    for (auto& name : props) {
      if (buf[0] != '\0' && name.find(buf) == std::string::npos) continue;
      if (ImGui::GetCursorScreenPos().y > posY + ImGui::GetStyle().FramePadding.y && !ImGui::IsItemVisible()) {
        ImGui::BulletText("%s", name.c_str());
        continue;
      }
      auto prop = mpv->property<mpv_node, MPV_FORMAT_NODE>(name.c_str());
      if (format & 1 << prop.format) drawPropNode(name.c_str(), prop);
      mpv_free_node_contents(&prop);
    }
    ImGui::EndListBox();
  }
}

void Debug::drawPropNode(const char* name, mpv_node& node, int depth) {
  constexpr static auto drawSimple = [&](const char* title, mpv_node prop) {
    std::string value;
    auto style = ImGuiStyle();
    ImVec4 color = style.Colors[ImGuiCol_CheckMark];
    switch (prop.format) {
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
      case MPV_FORMAT_BYTE_ARRAY:
        value = fmt::format("byte array [{}]", prop.u.ba->size);
        break;
      case MPV_FORMAT_NONE:
      default:
        value = i18n("views.debug.properties.invalid");
        color = style.Colors[ImGuiCol_TextDisabled];
        break;
    }
    ImGui::PushID(&prop);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
    ImGui::Selectable("", false);
    if (ImGui::BeginPopupContextItem(fmt::format("##menu_{}", title).c_str())) {
      if (ImGui::MenuItem("views.debug.properties.menu.copy"_i18n))
        ImGui::SetClipboardText(fmt::format("{}={}", title, value).c_str());
      if (ImGui::MenuItem("views.debug.properties.menu.copy_name"_i18n)) ImGui::SetClipboardText(title);
      if (ImGui::MenuItem("views.debug.properties.menu.copy_value"_i18n)) ImGui::SetClipboardText(value.c_str());
      ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::BulletText("%s", title);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
    ImGui::TextColored(color, "%s", value.c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", value.c_str());
    ImGui::PopStyleVar();
    ImGui::PopID();
  };

  switch (node.format) {
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
        for (int i = 0; i < node.u.list->num; i++) drawPropNode(node.u.list->keys[i], node.u.list->values[i]);
        ImGui::TreePop();
      }
      break;
    default:
      drawSimple(name, node);
      break;
  }
}

Debug::Console::Console(Mpv* mpv) : mpv(mpv) {
  ClearLog();
  memset(InputBuf, 0, sizeof(InputBuf));
}

Debug::Console::~Console() {
  ClearLog();
  for (int i = 0; i < History.Size; i++) free(History[i]);
  for (int i = 0; i < Commands.Size; i++) free(Commands[i]);
}

void Debug::Console::init(const char* level, int limit) {
  LogLevel = level;
  LogLimit = limit;
  mpv->requestLog(level, [this](const char* prefix, const char* level, const char* text) {
    AddLog(level, "[%s] %s", prefix, text);
  });
}

void Debug::Console::initCommands(std::vector<std::pair<std::string, std::string>>& commands) {
  if (CommandInited) return;
  for (auto& cmd : builtinCommands) Commands.push_back(ImStrdup(cmd.c_str()));
  for (auto& [name, args] : commands) Commands.push_back(ImStrdup(name.c_str()));
  CommandInited = true;
}

void Debug::Console::ClearLog() {
  for (int i = 0; i < Items.Size; i++) free(Items[i].Str);
  Items.clear();
}

void Debug::Console::AddLog(const char* level, const char* fmt, ...) {
  std::va_list args;
  va_start(args, fmt);

  int size;
  std::va_list copy;
  va_copy(copy, args);
  size = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);

  char buf[size + 1];
  std::vsnprintf(buf, size, fmt, args);
  buf[size] = '\0';
  va_end(args);

  int fontIdx = 1;  // mono
  const char* p = buf;
  auto mono = ImGui::GetIO().Fonts->Fonts[fontIdx];
  while (*p) {
    if (*p != '\n' && *p != '\r' && !mono->IsGlyphInFont((ImWchar)*p)) {
      fontIdx = 0;  // unicode
      break;
    }
    p++;
  }

  Items.push_back({ImStrdup(buf), level, fontIdx});
  if (Items.Size > LogLimit) {
    int offset = Items.Size - LogLimit;
    for (int i = 0; i < offset; i++) free(Items[i].Str);
    Items.erase(Items.begin(), Items.begin() + offset);
  }
}

ImVec4 Debug::Console::LogColor(const char* level) {
  std::map<std::string, ImVec4> logColors = {
      {"fatal", ImVec4{0.804f, 0, 0, 1.0f}},        {"error", ImVec4{0.804f, 0, 0, 1.0f}},
      {"warn", ImVec4{0.804f, 0.804f, 0, 1.0f}},    {"info", ImVec4{1.0f, 1.0f, 1.0f, 1.0f}},
      {"status", ImVec4{1.0f, 1.0f, 1.0f, 1.0f}},   {"v", ImVec4{0.075f, 0.631f, 0.055f, 1.0f}},
      {"debug", ImVec4{0.50f, 0.50f, 0.50f, 1.0f}}, {"trace", ImVec4{0.30f, 0.30f, 0.30f, 1.0f}},
  };
  if (level == nullptr || !logColors.contains(level)) level = "status";
  return logColors[level];
}

void Debug::Console::draw() {
  ImGui::TextUnformatted("views.debug.console.log.limit"_i18n);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(scaled(3));
  ImGui::InputInt("##console.log.limit", &LogLimit, 0);
  ImGui::SameLine();
  ImGui::TextDisabled("(%d/%d)", Items.Size, LogLimit);
  ImGui::SameLine();
  ImGui::TextUnformatted("views.debug.console.log.level"_i18n);
  ImGui::SameLine();
  static const char* levels[] = {"fatal", "error", "warn", "info", "status", "v", "debug", "trace", "no"};
  ImGui::SetNextItemWidth(scaled(6));
  if (ImGui::BeginCombo("##Level", LogLevel.c_str())) {
    for (auto& level : levels) {
      bool selected = LogLevel == level;
      ImGui::PushStyleColor(ImGuiCol_Text, LogColor(level));
      if (ImGui::Selectable(level, selected)) init(level, LogLimit);
      ImGui::PopStyleColor();
      if (selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  ImGui::HelpMarker("views.debug.console.log.hint"_i18n);
  ImGui::SameLine();
  ImGui::TextUnformatted("views.debug.console.log.filter"_i18n);
  ImGui::SameLine();
  Filter.Draw("##console.log.filter", 0);
  ImGui::Separator();

  const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  bool copy_to_clipboard = false;
  if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    if (ImGui::BeginPopupContextWindow()) {
      ImGui::MenuItem("views.debug.console.log.menu.auto_scroll"_i18n, nullptr, &AutoScroll);
      if (ImGui::MenuItem("views.debug.console.log.menu.clear"_i18n)) ClearLog();
      ImGui::MenuItem("views.debug.console.log.menu.copy"_i18n, nullptr, &copy_to_clipboard);
      ImGui::EndPopup();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    if (copy_to_clipboard) ImGui::LogToClipboard();
    for (int i = 0; i < Items.Size; i++) {
      auto item = Items[i];
      if (!Filter.PassFilter(item.Str)) continue;

      auto font = ImGui::GetIO().Fonts->Fonts[item.FontIdx];
      ImGui::PushFont(font, font->LegacySize);
      ImGui::PushStyleColor(ImGuiCol_Text, LogColor(item.Lev));
      ImGui::TextUnformatted(item.Str);
      ImGui::PopFont();
      ImGui::PopStyleColor();
    }
    if (copy_to_clipboard) ImGui::LogFinish();

    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;
    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
  ImGui::Separator();

  bool reclaim_focus = false;
  ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
                                         ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
  if (ImGui::InputTextWithHint(
          "views.debug.console.input"_i18n, "views.debug.console.input.tip"_i18n, InputBuf, IM_ARRAYSIZE(InputBuf),
          input_text_flags,
          [](ImGuiInputTextCallbackData* data) {
            Console* console = (Console*)data->UserData;
            return console->TextEditCallback(data);
          },
          (void*)this)) {
    char* s = InputBuf;
    ImStrTrimBlanks(s);
    if (s[0]) ExecCommand(s);
    strcpy(s, "");
    reclaim_focus = true;
  }

  ImGui::SetItemDefaultFocus();
  if (reclaim_focus) ImGui::SetKeyboardFocusHere(-1);  // Auto focus previous widget

  ImGui::SameLine();
  ImGui::HelpMarker("views.debug.console.tip"_i18n);
}

void Debug::Console::ExecCommand(const char* command_line) {
  AddLog("info", "# %s\n", command_line);

  HistoryPos = -1;
  for (int i = History.Size - 1; i >= 0; i--)
    if (ImStricmp(History[i], command_line) == 0) {
      free(History[i]);
      History.erase(History.begin() + i);
      break;
    }
  History.push_back(ImStrdup(command_line));

  if (ImStricmp(command_line, "CLEAR") == 0) {
    ClearLog();
  } else if (ImStricmp(command_line, "HELP") == 0) {
    AddLog("info", "Builtin Commands:");
    for (auto& cmd : builtinCommands) AddLog("info", "- %s", cmd.c_str());
    AddLog("info", "MPV Commands:");
    auto node = mpv->property<mpv_node, MPV_FORMAT_NODE>("command-list");
    std::vector<std::pair<std::string, std::string>> commands;
    formatCommands(node, commands);
    for (auto& [name, args] : commands) AddLog("info", "- %s %s", name.c_str(), args.c_str());
    mpv_free_node_contents(&node);
  } else if (ImStricmp(command_line, "HISTORY") == 0) {
    int first = History.Size - 10;
    for (int i = first > 0 ? first : 0; i < History.Size; i++) AddLog("info", "%3d: %s\n", i, History[i]);
  } else {
    int err = mpv->command(command_line);
    if (err < 0) {
      AddLog("error", "%s", mpv_error_string(err));
    } else {
      AddLog("info", "[mpv] Success");
    }
  }

  ScrollToBottom = true;
}

int Debug::Console::TextEditCallback(ImGuiInputTextCallbackData* data) {
  switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackCompletion: {
      const char* word_end = data->Buf + data->CursorPos;
      const char* word_start = word_end;
      while (word_start > data->Buf) {
        const char c = word_start[-1];
        if (c == ' ' || c == '\t' || c == ',' || c == ';') break;
        word_start--;
      }

      ImVector<const char*> candidates;
      for (int i = 0; i < Commands.Size; i++)
        if (ImStrnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0) candidates.push_back(Commands[i]);

      if (candidates.Size == 0) {
        AddLog("info", "No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
      } else if (candidates.Size == 1) {
        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
        data->InsertChars(data->CursorPos, candidates[0]);
        data->InsertChars(data->CursorPos, " ");
      } else {
        int match_len = (int)(word_end - word_start);
        for (;;) {
          int c = 0;
          bool all_candidates_matches = true;
          for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
            if (i == 0)
              c = std::toupper(candidates[i][match_len]);
            else if (c == 0 || c != std::toupper(candidates[i][match_len]))
              all_candidates_matches = false;
          if (!all_candidates_matches) break;
          match_len++;
        }

        if (match_len > 0) {
          data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
          data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
        }

        AddLog("info", "Possible matches:\n");
        std::string s;
        for (int i = 0; i < candidates.Size; i++) {
          s += fmt::format("{:<32}", candidates[i]);
          if (i != 0 && (i + 1) % 3 == 0) {
            AddLog("info", "%s\n", s.c_str());
            s.clear();
          }
        }
        if (!s.empty()) AddLog("info", "%s\n", s.c_str());
      }

      break;
    }
    case ImGuiInputTextFlags_CallbackHistory: {
      const int prev_history_pos = HistoryPos;
      if (data->EventKey == ImGuiKey_UpArrow) {
        if (HistoryPos == -1)
          HistoryPos = History.Size - 1;
        else if (HistoryPos > 0)
          HistoryPos--;
      } else if (data->EventKey == ImGuiKey_DownArrow) {
        if (HistoryPos != -1)
          if (++HistoryPos >= History.Size) HistoryPos = -1;
      }

      if (prev_history_pos != HistoryPos) {
        const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, history_str);
      }
    }
  }
  return 0;
}
}  // namespace ImPlay::Views