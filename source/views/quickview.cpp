#include <algorithm>
#include <fonts/fontawesome.h>
#include "helpers.h"
#include "views/quickview.h"

namespace ImPlay::Views {
Quickview::Quickview(Config *config, Dispatch *dispatch, Mpv *mpv) : View(config, dispatch, mpv) {
  addTab("Playlist", [this]() { drawPlaylistTabContent(); });
  addTab("Chapters", [this]() { drawChaptersTabContent(); });
  addTab("Video", [this]() { drawVideoTabContent(); }, true);
  addTab("Audio", [this]() { drawAudioTabContent(); }, true);
  addTab("Subtitle", [this]() { drawSubtitleTabContent(); }, true);
}

void Quickview::draw() {
  if (!m_open) return;
  static bool pin = false;
  auto viewport = ImGui::GetMainViewport();
  float width = std::min(viewport->WorkSize.x * 0.5f, scaled(30));
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

  ImGui::SetNextWindowSize(ImVec2(width, viewport->WorkSize.y));
  ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - width, viewport->WorkPos.y));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  if (ImGui::Begin("Quickview Panel", &m_open, flags)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape) || (!pin && !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)))
      m_open = false;
    if (ImGui::BeginTabBar("Quickviewview")) {
      for (auto &[name, draw, child] : tabs) {
        ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
        if (iequals(curTab, name) && !tabSwitched) {
          flags |= ImGuiTabItemFlags_SetSelected;
          tabSwitched = true;
        }
        if (ImGui::BeginTabItem(name, nullptr, flags)) {
          if (child) ImGui::BeginChild(name);
          draw();
          if (child) ImGui::EndChild();
          ImGui::EndTabItem();
        }
      }
      ImGui::PushStyleColor(ImGuiCol_Tab, ImGui::GetStyleColorVec4(pin ? ImGuiCol_CheckMark : ImGuiCol_Tab));
      if (ImGui::TabItemButton(ICON_FA_THUMBTACK)) pin = !pin;
      ImGui::PopStyleColor();
      if (ImGui::TabItemButton(ICON_FA_TIMES)) m_open = false;
      ImGui::EndTabBar();
    }
    ImGui::End();
  }
  ImGui::PopStyleVar(3);
}

void Quickview::alignRight(const char* label) {
  ImGui::SameLine(ImGui::GetContentRegionAvail().x -
                  (ImGui::CalcTextSize(label).x + 2 * ImGui::GetStyle().FramePadding.x));
}

void Quickview::iconButton(const char *icon, const char *cmd, const char *tooltip, bool sameline) {
  if (sameline) ImGui::SameLine();
  if (ImGui::Button(format("{}##{}", icon, cmd).c_str())) mpv->command(cmd);
  if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", tooltip);
}

bool Quickview::toggleButton(const char *label, bool toggle, const char *tooltip, ImGuiCol col) {
  ImGui::PushStyleColor(col, ImGui::GetStyleColorVec4(toggle ? ImGuiCol_CheckMark : col));
  bool ret = ImGui::Button(format("{}##{}", label, tooltip ? tooltip : "").c_str());
  if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", tooltip);
  ImGui::PopStyleColor();
  return ret;
}

bool Quickview::toggleButton(bool toggle, const char *tooltip) {
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  bool ret = toggleButton(toggle ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, toggle, tooltip, ImGuiCol_Text);
  ImGui::PopStyleColor();
  return ret;
}

void Quickview::drawTracks(const char *type, const char *prop) {
  ImGui::Text("Tracks");
  alignRight(ICON_FA_TOGGLE_ON);
  const char *state = mpv->property(prop);
  if (toggleButton(state && !iequals(state, "no"), "Toggle Tracks"))
    mpv->commandv("cycle-values", prop, "no", "auto", nullptr);
  if (ImGui::BeginListBox("##tracks", ImVec2(-FLT_MIN, 3 * ImGui::GetTextLineHeightWithSpacing()))) {
    auto items = mpv->trackList();
    if (items.empty()) ImGui::TextDisabled("<Empty>");
    for (auto &item : items) {
      if (item.type != type) continue;
      auto title = item.title.empty() ? format("Track {}", item.id) : item.title;
      if (!item.lang.empty()) title += format(" [{}]", item.lang);
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", item.selected)) mpv->property(prop, std::to_string(item.id).c_str());
      ImGui::SameLine();
      ImGui::TextColored(ImGui::GetStyleColorVec4(item.selected ? ImGuiCol_CheckMark : ImGuiCol_Text), "%s",
                         title.c_str());
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

void Quickview::drawPlaylistTabContent() {
  auto style = ImGui::GetStyle();
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos");

  iconButton(ICON_FA_SEARCH, "script-message-to implay command-palette playlist", nullptr, false);
  iconButton(ICON_FA_SYNC, "cycle-values loop-playlist inf no", "Loop");
  iconButton(ICON_FA_RANDOM, "playlist-shuffle", "Shuffle");
  ImGui::SameLine(ImGui::GetContentRegionAvail().x -
                  3 * (ImGui::CalcTextSize(ICON_FA_PLUS).x + style.FramePadding.x + style.ItemSpacing.x));
  iconButton(ICON_FA_PLUS, "script-message-to implay playlist-add-files", "Add Files", false);
  iconButton(ICON_FA_FOLDER_PLUS, "script-message-to implay playlist-add-folder", "Add Folder");
  iconButton(ICON_FA_TRASH_ALT, "playlist-clear", "Clear");
  ImGui::Separator();

  if (ImGui::BeginListBox("##playlist", ImVec2(-FLT_MIN, -FLT_MIN))) {
    auto items = mpv->playlist();
    static int selected = pos;
    auto drawContextmenu = [&](Mpv::PlayItem *item = nullptr) {
      bool enabled = item != nullptr;
      int id = enabled ? item->id : -1;
      if (ImGui::MenuItem("Play", nullptr, nullptr, enabled))
        mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", id);
      if (ImGui::MenuItem("Play Next", nullptr, nullptr, enabled))
        mpv->commandv("playlist-move", std::to_string(id).c_str(), std::to_string(pos + 1).c_str(), nullptr);
      ImGui::Separator();
      if (ImGui::MenuItem("Move to first", nullptr, nullptr, enabled))
        mpv->commandv("playlist-move", std::to_string(id).c_str(), "0", nullptr);
      if (ImGui::MenuItem("Move to last", nullptr, nullptr, enabled))
        mpv->commandv("playlist-move", std::to_string(id).c_str(), std::to_string(items.size()).c_str(), nullptr);
      if (ImGui::MenuItem("Remove", nullptr, nullptr, enabled))
        mpv->commandv("playlist-remove", std::to_string(id).c_str(), nullptr);
      ImGui::Separator();
      if (ImGui::MenuItem("Copy Path", nullptr, nullptr, enabled)) ImGui::SetClipboardText(item->path.c_str());
      if (ImGui::MenuItem("Reveal in Folder", nullptr, nullptr, enabled)) revealInFolder(item->path);
      ImGui::Separator();
      if (ImGui::MenuItem("Search")) mpv->command("script-message-to implay command-palette playlist");
      if (ImGui::MenuItem("Shuffle")) mpv->command("playlist-shuffle");
      if (ImGui::MenuItem("Loop")) mpv->command("cycle-values loop-playlist inf no");
      ImGui::Separator();
      if (ImGui::MenuItem("Add Files")) mpv->command("script-message-to implay playlist-add-files");
      if (ImGui::MenuItem("Add Folders")) mpv->command("script-message-to implay playlist-add-folder");
      if (ImGui::MenuItem("Clear")) mpv->command("playlist-clear");
    };
    if (items.empty()) ImGui::TextDisabled("<Empty>");
    for (auto &item : items) {
      std::string title = item.title;
      if (title.empty() && !item.filename.empty()) title = item.filename;
      if (title.empty()) title = format("Item {}", item.id + 1);
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", selected == item.id)) selected = item.id;
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", item.id);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", title.c_str());
      if (ImGui::BeginPopupContextItem()) {
        drawContextmenu(&item);
        ImGui::EndPopup();
      }
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImGui::GetStyleColorVec4(item.id == pos ? ImGuiCol_CheckMark : ImGuiCol_Text));
      ImGui::TextEllipsis(title.c_str());
      ImGui::PopStyleColor();
      ImGui::PopID();
    }
    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
      drawContextmenu();
      ImGui::EndPopup();
    }
    ImGui::EndListBox();
  }
}

void Quickview::drawChaptersTabContent() {
  auto items = mpv->chapterList();
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("chapter");
  if (ImGui::BeginListBox("##chapters", ImVec2(-FLT_MIN, -FLT_MIN))) {
    if (items.empty()) ImGui::TextDisabled("<Empty>");
    for (auto &item : items) {
      auto title = item.title.empty() ? format("Chapter {}", item.id + 1) : item.title;
      auto time = format("{:%H:%M:%S}", std::chrono::duration<int>((int)item.time));
      auto color = ImGui::GetStyleColorVec4(item.id == pos ? ImGuiCol_CheckMark : ImGuiCol_Text);
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", item.id == pos))
        mpv->commandv("seek", std::to_string(item.time).c_str(), "absolute", nullptr);
      ImGui::SameLine();
      ImGui::TextColored(color, "%s", title.c_str());
      alignRight(time.c_str());
      ImGui::TextColored(color, "%s", time.c_str());
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

void Quickview::drawVideoTabContent() {
  drawTracks("video", "vid");
  ImGui::NewLine();

  ImGui::Text("Rotate");
  const char *rotates[] = {"0", "90", "180", "270"};
  for (auto rotate : rotates) {
    if (ImGui::Button(format("{}°", rotate).c_str())) mpv->commandv("set", "video-rotate", rotate, nullptr);
    ImGui::SameLine();
  }
  iconButton(ICON_FA_UNDO, "add video-rotate -1", "Rotate Left", false);
  iconButton(ICON_FA_REDO, "add video-rotate 1", "Rotate Right", true);
  ImGui::NewLine();

  ImGui::Text("Scale");
  const float scales[] = {0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
  for (auto scale : scales) {
    if (ImGui::Button(format("{}%", (int)(scale * 100)).c_str()))
      mpv->commandv("set", "window-scale", std::to_string(scale).c_str(), nullptr);
    ImGui::SameLine();
  }
  iconButton(ICON_FA_MINUS, "add window-scale -0.1", "Scale Down", false);
  iconButton(ICON_FA_PLUS, "add window-scale 0.1", "Scale Up");
  ImGui::NewLine();

  ImGui::Text("PanScan");
  iconButton(ICON_FA_SEARCH_MINUS, "add video-zoom -0.1", "Zoom In", false);
  iconButton(ICON_FA_SEARCH_PLUS, "add video-zoom 0.1", "Zoom Out");
  iconButton(ICON_FA_ARROW_LEFT, "add video-pan-x -0.1", "Move Left");
  iconButton(ICON_FA_ARROW_RIGHT, "add video-pan-x 0.1", "Move Right");
  iconButton(ICON_FA_ARROW_UP, "add video-pan-y -0.1", "Move Up");
  iconButton(ICON_FA_ARROW_DOWN, "add video-pan-y 0.1", "Move Down");
  iconButton(ICON_FA_MINUS, "add panscan -0.1", "PanScan Decrease");
  iconButton(ICON_FA_PLUS, "add panscan 0.1", "PanScan Increase");
  iconButton(ICON_FA_UNDO, "set video-zoom 0; set panscan 0; set video-pan-x 0 ; set video-pan-y 0", "Reset");
  ImGui::NewLine();

  ImGui::Text("Aspect Ratio");
  const char *ratios[] = {"16:9", "16:10", "4:3", "2.35:1", "1.85:1", "1:1"};
  for (auto ratio : ratios) {
    if (ImGui::Button(ratio)) mpv->commandv("set", "video-aspect", ratio, nullptr);
    ImGui::SameLine();
  }
  iconButton(ICON_FA_UNDO, "set video-aspect -1", "Reset", false);
  ImGui::SameLine();
  static char ratio[10] = {0};
  ImGui::SetNextItemWidth(scaled(4));
  if (ImGui::InputTextWithHint("##Aspect Ratio", "Custom", ratio, IM_ARRAYSIZE(ratio),
                               ImGuiInputTextFlags_EnterReturnsTrue)) {
    mpv->commandv("set", "video-aspect", ratio, nullptr);
    ratio[0] = '\0';
  }
  ImGui::NewLine();

  ImGui::Text("Misc");
  if (ImGui::Button("HW Decoding")) mpv->command("cycle-values hwdec auto no");
  ImGui::SameLine();
  if (ImGui::Button("Deinterlace")) mpv->command("cycle deinterlace");
  ImGui::NewLine();
  ImGui::Separator();
  ImGui::NewLine();

  ImGui::Text("Equalizer");
  ImGui::Spacing();
  const char *eq[] = {"brightness", "contrast", "saturation", "gamma", "hue"};
  static int equalizer[IM_ARRAYSIZE(eq)] = {
      (int)mpv->property<int64_t, MPV_FORMAT_INT64>("brightness"),
      (int)mpv->property<int64_t, MPV_FORMAT_INT64>("contrast"),
      (int)mpv->property<int64_t, MPV_FORMAT_INT64>("saturation"),
      (int)mpv->property<int64_t, MPV_FORMAT_INT64>("gamma"),
      (int)mpv->property<int64_t, MPV_FORMAT_INT64>("hue"),
  };
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + scaled(1));
  ImGui::BeginGroup();
  for (int i = 0; i < IM_ARRAYSIZE(equalizer); i++) {
    if (ImGui::Button(format("{}##{}", ICON_FA_UNDO, eq[i]).c_str())) {
      equalizer[i] = 0;
      mpv->commandv("set", eq[i], "0", nullptr);
    }
    ImGui::SameLine();
    std::string label = eq[i];
    label[0] = std::toupper(label[0]);
    if (ImGui::SliderInt(label.c_str(), &equalizer[i], -100, 100))
      mpv->commandv("set", eq[i], std::to_string(equalizer[i]).c_str(), nullptr);
  }
  ImGui::EndGroup();
}

void Quickview::drawAudioTabContent() {
  drawTracks("audio", "aid");
  ImGui::NewLine();

  ImGui::Text("Volume");
  static int volume = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("volume");
  if (ImGui::SliderInt("##Volume", &volume, 0, 200, "%d%%"))
    mpv->commandv("set", "volume", std::to_string(volume).c_str(), nullptr);
  ImGui::SameLine();
  bool mute = mpv->property<int, MPV_FORMAT_FLAG>("mute");
  if (toggleButton(ICON_FA_VOLUME_MUTE, mute, "Mute")) mpv->command("cycle mute");
  ImGui::NewLine();

  ImGui::Text("Delay");
  static float delay = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("audio-delay");
  if (ImGui::SliderFloat("##Delay", &delay, -10, 10, "%.1fs"))
    mpv->commandv("set", "audio-delay", format("{:.1f}", delay).c_str(), nullptr);
  iconButton(ICON_FA_UNDO, "set audio-delay 0", "Reset");
  ImGui::NewLine();
  ImGui::Separator();
  ImGui::NewLine();

  drawAudioEq();
}

void Quickview::drawSubtitleTabContent() {
  drawTracks("sub", "sid");

  iconButton(ICON_FA_ARROW_UP, "add sub-pos -1", "Move Subtitle Up", false);
  iconButton(ICON_FA_ARROW_DOWN, "add sub-pos 1", "Move Subtitle Down");
  iconButton(ICON_FA_REDO, "set sub-pos 100", "Reset Subtitle Position");
  iconButton(ICON_FA_EYE_SLASH, "cycle sub-visibility", "Show/Hide Subtitle");
  alignRight(ICON_FA_PLUS);
  iconButton(ICON_FA_PLUS, "script-message-to implay load-sub", "Load External Subtitles..", false);
  ImGui::Separator();
  ImGui::NewLine();

  ImGui::Text("Scale");
  static float scale = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("sub-scale");
  if (ImGui::SliderFloat("##Scale", &scale, 0, 4, "%.1f"))
    mpv->commandv("set", "sub-scale", format("{:.1f}", scale).c_str(), nullptr);
  iconButton(ICON_FA_UNDO, "set sub-scale 1", "Reset");
  ImGui::NewLine();

  ImGui::Text("Delay");
  static float delay = (float)mpv->property<double, MPV_FORMAT_DOUBLE>("sub-delay");
  if (ImGui::SliderFloat("##Delay", &delay, -10, 10, "%.1fs"))
    mpv->commandv("set", "sub-delay", format("{:.1f}", delay).c_str(), nullptr);
  iconButton(ICON_FA_UNDO, "set sub-delay 0", "Reset");
}

void Quickview::drawAudioEq() {
  ImGui::Text("Equalizer");
  alignRight(ICON_FA_TOGGLE_ON);
  if (toggleButton(audioEqEnabled, "Toggle Equalizer")) toggleAudioEq();
  ImGui::Spacing();

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + scaled(1));
  if (!audioEqEnabled) ImGui::BeginDisabled();
  ImGui::BeginGroup();
  float lineWidth = 0;
  float availWidth = ImGui::GetContentRegionAvail().x;
  int pSize = audioEqPresets.size();
  static float gain[FREQ_COUNT] = {0};
  for (int i = 0; i < pSize; i++) {
    auto item = audioEqPresets[i];
    if (toggleButton(item.name, audioEqIndex == i)) {
      selectAudioEq(i);
      for (int j = 0; j < FREQ_COUNT; j++) gain[j] = (double)item.values[j] / 12;
    }
    lineWidth += ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
    if (i < pSize - 1) {
      if (lineWidth + ImGui::CalcTextSize(audioEqPresets[i + 1].name).x < availWidth)
        ImGui::SameLine();
      else
        lineWidth = 0;
    }
  }
  ImGui::EndGroup();
  ImGui::NewLine();

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + scaled(1));
  ImGui::BeginGroup();
  float spacing = scaled(2);
  ImVec2 size = ImVec2(scaled(0.8f), scaled(10));
  float start = ImGui::GetCursorPosX();
  for (int i = 0; i < FREQ_COUNT; i++) {
    std::string label = format("##{}", audioEqFreqs[i]);
    if (ImGui::VSliderFloat(label.c_str(), size, &gain[i], -12, 12, "")) setAudioEqValue(i, gain[i]);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%.1fdB", gain[i]);
    if (i < FREQ_COUNT - 1) ImGui::SameLine(0, spacing);
  }
  for (int i = 0; i < FREQ_COUNT; i++) {
    auto tsize = ImGui::CalcTextSize(audioEqFreqs[i]);
    ImGui::SetCursorPosX(start + i * (spacing + size.x) + (size.x - tsize.x) / 2);
    ImGui::Text("%s", audioEqFreqs[i]);
    ImGui::SameLine();
  }
  ImGui::EndGroup();
  if (!audioEqEnabled) ImGui::EndDisabled();
}

void Quickview::applyAudioEq() {
  if (audioEqEnabled) {
    if (audioEqIndex < 0) return;
    auto equalizer = audioEqPresets[audioEqIndex];
    mpv->command("af remove @aeq");
    mpv->commandv("af", "add", equalizer.toFilter("@aeq", audioEqChannels).c_str(), nullptr);
    mpv->commandv("show-text", format("Audio Equalizer: {}", equalizer.name).c_str(), nullptr);
  } else {
    mpv->command("af remove @aeq; show-text \"Audio Equalizer: Disabled\"");
  }
}

void Quickview::toggleAudioEq() {
  audioEqEnabled = !audioEqEnabled;
  updateAudioEqChannels();
  applyAudioEq();
}

void Quickview::selectAudioEq(int index) {
  audioEqIndex = index;
  int customIndex = audioEqPresets.size() - 1;
  for (int i = 0; i < FREQ_COUNT; i++) audioEqPresets[customIndex].values[i] = audioEqPresets[index].values[i];
  applyAudioEq();
}

void Quickview::setAudioEqValue(int freqIndex, float gain) {
  audioEqIndex = audioEqPresets.size() - 1;
  audioEqPresets[audioEqIndex].values[freqIndex] = gain * 12;
  applyAudioEq();
}

void Quickview::updateAudioEqChannels() {
  auto node = mpv->property<mpv_node, MPV_FORMAT_NODE>("audio-params");
  if (node.format == MPV_FORMAT_NODE_MAP) {
    for (int i = 0; i < node.u.list->num; i++) {
      if (strcmp(node.u.list->keys[i], "channel-count") == 0) {
        audioEqChannels = node.u.list->values[i].u.int64;
        return;
      }
    }
  }
  audioEqChannels = 2;
}

std::string Quickview::AudioEqItem::toFilter(const char *name, int channels) {
  std::string s;
  double freq = 31.25;
  for (int ch = 0; ch < channels; ch++) {
    for (int f = 0; f < FREQ_COUNT; f++) {
      double v = (double)values[f] / 12;
      s += format("c{} f={} w={} g={}|", ch, freq, 1000, v);
      freq *= 2;
    }
  }
  return format("{}:lavfi=[anequalizer={}]", name, s);
}
}  // namespace ImPlay::Views