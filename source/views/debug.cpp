#include <map>
#include <fonts/fontawesome.h>
#include <fmt/format.h>
#include "views/debug.h"
#include "mpv.h"
#include "helpers.h"

namespace ImPlay::Views {
Debug::Debug(Config* config, Mpv* mpv) : View() {
  this->config = config;
  this->mpv = mpv;
  console = new Console(mpv);
}

Debug::~Debug() { delete console; }

void Debug::init() { console->init(config->logLevel.c_str(), config->logLimit); }

void Debug::show() {
  m_open = true;
  console->initCommands();
}

void Debug::draw() {
  if (!m_open) return;
  ImVec2 wPos = ImGui::GetMainViewport()->WorkPos;
  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(wSize.x * 0.4f, wSize.y * 0.8f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x * 0.2f, wPos.y + wSize.y * 0.5f), ImGuiCond_FirstUseEver,
                          ImVec2(0.2f, 0.5f));
  if (ImGui::Begin("Metrics & Debug", &m_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
    drawHeader();
    drawProperties("Options", "options");
    drawProperties("Properties", "property-list");
    drawBindings();
    drawCommands();
    drawConsole();
    ImGui::End();
  }
  if (m_demo) ImGui::ShowDemoWindow(&m_demo);
  if (m_metrics) ImGui::ShowMetricsWindow(&m_metrics);
}

void Debug::drawHeader() {
  ImGuiIO& io = ImGui::GetIO();
  char* version = mpv->property("mpv-version");
  auto style = ImGuiStyle();
  ImGui::Text("%s", version);
  auto vSize = ImGui::CalcTextSize(fmt::format("ImGui {}", ImGui::GetVersion()).c_str());
  auto mSize = ImGui::CalcTextSize(fmt::format("FPS: %.2f", io.Framerate).c_str());
  ImGui::SameLine(ImGui::GetWindowWidth() -
                  (vSize.x + mSize.x + 4 * style.ItemInnerSpacing.x + 3 * style.ItemSpacing.x));
  ImGui::Text("ImGui %s", ImGui::GetVersion());
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) m_demo = !m_demo;
  ImGui::SameLine();
  ImGui::TextColored(style.Colors[ImGuiCol_CheckMark], "FPS: %.2f", io.Framerate);
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) m_metrics = !m_metrics;
  mpv_free(version);

  ImGui::TextWrapped("NOTE: playback may become very slow when Properties are expanded.");
  ImGui::Spacing();
}

void Debug::drawConsole() {
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (m_node != "Console") ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader("Console & Logs")) return;
  m_node = "Console";
  console->draw();
}

void Debug::drawBindings() {
  auto bindings = mpv->bindingList();
  if (m_node != "Bindings") ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader(fmt::format("Bindings [{}]", bindings.size()).c_str())) return;
  m_node = "Bindings";

  if (ImGui::BeginListBox("input-bindings", ImVec2(-FLT_MIN, -FLT_MIN))) {
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

static std::vector<std::pair<std::string, std::string>> formatCommands(mpv_node& node) {
  std::vector<std::pair<std::string, std::string>> commands;
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
      args_str = fmt::format("{}", fmt::join(args, " "));
      if (vararg) args_str += " ...";
    }
    commands.push_back({name, args_str});
  }
  return commands;
}

void Debug::drawCommands() {
  auto node = mpv->property<mpv_node, MPV_FORMAT_NODE>("command-list");
  if (m_node != "Commands") ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader(fmt::format("Commands [{}]", node.u.list->num).c_str())) {
    mpv_free_node_contents(&node);
    return;
  }
  m_node = "Commands";

  static char buf[256] = "";
  ImGui::Text("Filter:");
  ImGui::SameLine();
  ImGui::PushItemWidth(-1);
  ImGui::InputText("Filter##commands", buf, IM_ARRAYSIZE(buf));
  ImGui::PopItemWidth();
  if (ImGui::BeginListBox("command-list", ImVec2(-FLT_MIN, -FLT_MIN))) {
    auto commands = formatCommands(node);
    for (auto& [name, args] : commands) {
      if (!name.starts_with(buf)) continue;
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
  mpv_free_node_contents(&node);
}

void Debug::drawProperties(const char* title, const char* key) {
  mpv_node node = mpv->property<mpv_node, MPV_FORMAT_NODE>(key);
  if (m_node != title) ImGui::SetNextItemOpen(false, ImGuiCond_Always);
  if (!ImGui::CollapsingHeader(fmt::format("{} [{}]", title, node.u.list->num).c_str())) {
    mpv_free_node_contents(&node);
    return;
  }
  m_node = title;

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
  auto posY = ImGui::GetCursorScreenPos().y;
  if (format > 0 && ImGui::BeginListBox(key, ImVec2(-FLT_MIN, -FLT_MIN))) {
    for (int i = 0; i < node.u.list->num; i++) {
      auto item = node.u.list->values[i];
      if (buf[0] != '\0' && !strstr(item.u.string, buf)) continue;
      if (ImGui::GetCursorScreenPos().y > posY + ImGui::GetStyle().FramePadding.y && !ImGui::IsItemVisible()) {
        ImGui::BulletText("%s", item.u.string);
        continue;
      }
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
    auto style = ImGuiStyle();
    ImVec4 color = style.Colors[ImGuiCol_CheckMark];
    switch (prop.format) {
      case MPV_FORMAT_NONE:
        value = "Invalid";
        color = style.Colors[ImGuiCol_TextDisabled];
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
        color = style.Colors[ImGuiCol_TextDisabled];
        break;
    }
    ImGui::PushID(&prop);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
    ImGui::Selectable("", false);
    if (ImGui::BeginPopupContextItem(fmt::format("##menu_{}", title).c_str())) {
      if (ImGui::MenuItem("Copy")) ImGui::SetClipboardText(fmt::format("{}={}", title, value).c_str());
      if (ImGui::MenuItem("Copy Name")) ImGui::SetClipboardText(title);
      if (ImGui::MenuItem("Copy value")) ImGui::SetClipboardText(value.c_str());
      ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::BulletText("%s", title);
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.5f);
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
        for (int i = 0; i < node.u.list->num; i++) drawPropNode(node.u.list->keys[i], node.u.list->values[i]);
        ImGui::TreePop();
      }
      break;
    case MPV_FORMAT_BYTE_ARRAY:
      ImGui::BulletText("byte array [%ld]", node.u.ba->size);
      break;
    default:
      ImGui::BulletText("Unknown format: %d", node.format);
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

// Portable helpers
static int Stricmp(const char* s1, const char* s2) {
  int d;
  while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
    s1++;
    s2++;
  }
  return d;
}

static int Strnicmp(const char* s1, const char* s2, int n) {
  int d = 0;
  while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
    s1++;
    s2++;
    n--;
  }
  return d;
}

static char* Strdup(const char* s) {
  IM_ASSERT(s);
  size_t len = strlen(s) + 1;
  void* buf = malloc(len);
  IM_ASSERT(buf);
  return (char*)memcpy(buf, (const void*)s, len);
}

static void Strtrim(char* s) {
  char* str_end = s + strlen(s);
  while (str_end > s && str_end[-1] == ' ') str_end--;
  *str_end = 0;
}

void Debug::Console::init(const char* level, int limit) {
  LogLevel = level;
  LogLimit = limit;
  mpv->requestLog(level, [this](const char* prefix, const char* level, const char* text) {
    AddLog(level, "[%s] %s", prefix, text);
  });
}

void Debug::Console::initCommands() {
  if (CommandInited) return;
  for (auto& cmd : builtinCommands) Commands.push_back(Strdup(cmd.c_str()));

  auto node = mpv->property<mpv_node, MPV_FORMAT_NODE>("command-list");
  for (int i = 0; i < node.u.list->num; i++) {
    auto item = node.u.list->values[i];
    for (int j = 0; j < item.u.list->num; j++) {
      auto key = item.u.list->keys[j];
      auto value = item.u.list->values[j];
      if (strcmp(key, "name") == 0) {
        Commands.push_back(Strdup(value.u.string));
      }
    }
  }
  mpv_free_node_contents(&node);
  CommandInited = true;
}

void Debug::Console::ClearLog() {
  for (int i = 0; i < Items.Size; i++) free(Items[i].Str);
  Items.clear();
}

void Debug::Console::AddLog(const char* level, const char* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
  buf[IM_ARRAYSIZE(buf) - 1] = 0;
  va_end(args);
  Items.push_back({Strdup(buf), level});
  if (Items.Size > LogLimit) {
    int offset = Items.Size - LogLimit;
    for (int i = 0; i < offset; i++) free(Items[i].Str);
    Items.erase(Items.begin(), Items.begin() + offset);
  }
}

ImVec4 Debug::Console::LogColor(const char* level) {
  std::map<std::string, ImVec4> logColors = {
      {"fatal", ImVec4{0.804f, 0, 0, 1.0f}},
      {"error", ImVec4{0.804f, 0, 0, 1.0f}},
      {"warn", ImVec4{0.804f, 0.804f, 0, 1.0f}},
      {"info", ImGui::GetStyle().Colors[ImGuiCol_Text]},
      {"v", ImVec4{0.075f, 0.631f, 0.055f, 1.0f}},
      {"debug", ImGui::GetStyle().Colors[ImGuiCol_Text]},
      {"trace", ImGui::GetStyle().Colors[ImGuiCol_Text]},
  };
  if (level == nullptr || !logColors.contains(level)) level = "info";
  return logColors[level];
}

void Debug::Console::draw() {
  ImGui::TextWrapped("Enter 'HELP' for help, 'TAB' for completion, 'Up/Down' for history.");

  if (ImGui::BeginPopup("Log Level")) {
    const char* items[] = {"fatal", "error", "warn", "info", "v", "debug", "trace", "no"};
    static std::string level = LogLevel;
    for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
      if (ImGui::MenuItem(items[i], nullptr, level == items[i])) {
        level = items[i];
        init(items[i], LogLimit);
      }
    }
    ImGui::EndPopup();
  }

  Filter.Draw("Filter##log", 8 * ImGui::GetFontSize());
  ImGui::SameLine();
  ImGui::SetNextItemWidth(3 * ImGui::GetFontSize());
  ImGui::InputInt("Limit", &LogLimit, 0);
  ImGui::SameLine();
  ImGui::Text("(%d/%d)", Items.Size, LogLimit);
  ImGui::SameLine();
  if (ImGui::Button("Level")) ImGui::OpenPopup("Log Level");
  ImGui::SameLine();
  Helpers::marker(
      "The log level and limit changed here won't be saved.\n"
      "Update log settings in Help-Settings to control startup config.");
  ImGui::Separator();

  // Reserve enough left-over height for 1 separator + 1 input text
  const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  bool copy_to_clipboard = false;
  if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                        ImGuiWindowFlags_HorizontalScrollbar)) {
    if (ImGui::BeginPopupContextWindow()) {
      ImGui::MenuItem("Auto-scroll", nullptr, &AutoScroll);
      if (ImGui::MenuItem("Clear")) ClearLog();
      ImGui::MenuItem("Copy", nullptr, &copy_to_clipboard);
      ImGui::EndPopup();
    }

    // Display every line as a separate entry so we can change their color or add custom widgets.
    // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
    // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
    // to only process visible items. The clipper will automatically measure the height of your first item and then
    // "seek" to display only items in the visible area.
    // To use the clipper we can replace your standard loop:
    //      for (int i = 0; i < Items.Size; i++)
    //   With:
    //      ImGuiListClipper clipper;
    //      clipper.Begin(Items.Size);
    //      while (clipper.Step())
    //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
    // - That your items are evenly spaced (same height)
    // - That you have cheap random access to your elements (you can access them given their index,
    //   without processing all the ones before)
    // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
    // We would need random-access on the post-filtered list.
    // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
    // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
    // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
    // to improve this example code!
    // If your items are of variable height:
    // - Split them into same height items would be simpler and facilitate random-seeking into your list.
    // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));  // Tighten spacing
    if (copy_to_clipboard) ImGui::LogToClipboard();
    for (int i = 0; i < Items.Size; i++) {
      auto item = Items[i];
      if (!Filter.PassFilter(item.Str)) continue;

      ImGui::PushStyleColor(ImGuiCol_Text, LogColor(item.Lev));
      ImGui::TextUnformatted(item.Str);
      ImGui::PopStyleColor();
    }
    if (copy_to_clipboard) ImGui::LogFinish();

    // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
    // Using a scrollbar or mouse-wheel will take away from the bottom edge.
    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;

    ImGui::PopStyleVar();
  }
  ImGui::EndChild();
  ImGui::Separator();

  // Command-line
  bool reclaim_focus = false;
  ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
                                         ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
  if (ImGui::InputTextWithHint(
          "Input", "press ENTER to execute", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags,
          [](ImGuiInputTextCallbackData* data) {
            Console* console = (Console*)data->UserData;
            return console->TextEditCallback(data);
          },
          (void*)this)) {
    char* s = InputBuf;
    Strtrim(s);
    if (s[0]) ExecCommand(s);
    strcpy(s, "");
    reclaim_focus = true;
  }

  // Auto-focus on window apparition
  ImGui::SetItemDefaultFocus();
  if (reclaim_focus) ImGui::SetKeyboardFocusHere(-1);  // Auto focus previous widget
}

void Debug::Console::ExecCommand(const char* command_line) {
  AddLog("info", "# %s\n", command_line);

  // Insert into history. First find match and delete it so it can be pushed to the back.
  // This isn't trying to be smart or optimal.
  HistoryPos = -1;
  for (int i = History.Size - 1; i >= 0; i--)
    if (Stricmp(History[i], command_line) == 0) {
      free(History[i]);
      History.erase(History.begin() + i);
      break;
    }
  History.push_back(Strdup(command_line));

  // Process command
  if (Stricmp(command_line, "CLEAR") == 0) {
    ClearLog();
  } else if (Stricmp(command_line, "HELP") == 0) {
    AddLog("info", "Builtin Commands:");
    for (auto& cmd : builtinCommands) AddLog("- %s", cmd.c_str());
    AddLog("info", "MPV Commands:");
    auto node = mpv->property<mpv_node, MPV_FORMAT_NODE>("command-list");
    auto commands = formatCommands(node);
    for (auto& [name, args] : commands) AddLog("info", "- %s %s", name.c_str(), args.c_str());
    mpv_free_node_contents(&node);
  } else if (Stricmp(command_line, "HISTORY") == 0) {
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

  // On command input, we scroll to bottom even if AutoScroll==false
  ScrollToBottom = true;
}

int Debug::Console::TextEditCallback(ImGuiInputTextCallbackData* data) {
  switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackCompletion: {
      // Locate beginning of current word
      const char* word_end = data->Buf + data->CursorPos;
      const char* word_start = word_end;
      while (word_start > data->Buf) {
        const char c = word_start[-1];
        if (c == ' ' || c == '\t' || c == ',' || c == ';') break;
        word_start--;
      }

      // Build a list of candidates
      ImVector<const char*> candidates;
      for (int i = 0; i < Commands.Size; i++)
        if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0) candidates.push_back(Commands[i]);

      if (candidates.Size == 0) {
        // No match
        AddLog("info", "No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
      } else if (candidates.Size == 1) {
        // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
        data->InsertChars(data->CursorPos, candidates[0]);
        data->InsertChars(data->CursorPos, " ");
      } else {
        // Multiple matches. Complete as much as we can..
        // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
        int match_len = (int)(word_end - word_start);
        for (;;) {
          int c = 0;
          bool all_candidates_matches = true;
          for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
            if (i == 0)
              c = toupper(candidates[i][match_len]);
            else if (c == 0 || c != toupper(candidates[i][match_len]))
              all_candidates_matches = false;
          if (!all_candidates_matches) break;
          match_len++;
        }

        if (match_len > 0) {
          data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
          data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
        }

        // List matches
        AddLog("info", "Possible matches:\n");
        std::string s;
        for (int i = 0; i < candidates.Size; i++) {
          s += fmt::format("{:<32}", candidates[i]);
          if (i != 0 && (i + 1) % 4 == 0) {
            AddLog("info", "%s\n", s.c_str());
            s.clear();
          }
        }
        if (!s.empty()) AddLog("info", "%s\n", s.c_str());
      }

      break;
    }
    case ImGuiInputTextFlags_CallbackHistory: {
      // Example of HISTORY
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

      // A better implementation would preserve the data on the current input line along with cursor position.
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