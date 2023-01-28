#include <algorithm>
#include <fonts/fontawesome.h>
#include "views/quick.h"
#include "helpers.h"

namespace ImPlay::Views {
Quick::Quick(Config *config, Mpv *mpv) : View() {
  this->config = config;
  this->mpv = mpv;

  addTab("Playlist", [this]() { drawPlaylistTabContent(); });
  addTab("Chapters", [this]() { drawChaptersTabContent(); });
  addTab("Video", [this]() { drawVideoTabContent(); }, true);
  addTab("Audio", [this]() { drawAudioTabContent(); }, true);
  addTab("Subtitle", [this]() { drawSubtitleTabContent(); }, true);
}

void Quick::draw() {
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
  if (ImGui::Begin("Quick Panel", &m_open, flags)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape) || (!pin && !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)))
      m_open = false;
    if (ImGui::BeginTabBar("Quickview")) {
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
      if (ImGui::TabItemButton(ICON_FA_THUMBTACK)) pin = !pin;
      if (ImGui::TabItemButton(ICON_FA_TIMES)) m_open = false;
      ImGui::EndTabBar();
    }
    ImGui::End();
  }
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();
}

void Quick::drawTracks(const char *type, const char *prop) {
  ImGui::Text("Tracks");
  if (ImGui::BeginListBox("##tracks", ImVec2(-FLT_MIN, 3 * ImGui::GetTextLineHeightWithSpacing()))) {
    auto items = mpv->trackList();
    for (auto &item : items) {
      if (item.type != type) continue;
      auto title = item.title.empty() ? format("Track {}", item.id) : item.title;
      if (!item.lang.empty()) title += format(" [{}]", item.lang);
      if (ImGui::Selectable(title.c_str(), item.selected)) mpv->property<int64_t, MPV_FORMAT_INT64>(prop, item.id);
    }
    ImGui::EndListBox();
  }
}

void Quick::drawPlaylistTabContent() {
  auto items = mpv->playlist();
  auto style = ImGui::GetStyle();
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos");
  auto bottom = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y;
  if (ImGui::BeginListBox("##playlist", ImVec2(-FLT_MIN, -bottom))) {
    static int selected = pos;
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
        if (ImGui::MenuItem("Play")) mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-pos", item.id);
        if (ImGui::MenuItem("Play Next"))
          mpv->commandv("playlist-move", std::to_string(item.id).c_str(), std::to_string(pos + 1).c_str(), nullptr);
        ImGui::Separator();
        if (ImGui::MenuItem("Move to first"))
          mpv->commandv("playlist-move", std::to_string(item.id).c_str(), "0", nullptr);
        if (ImGui::MenuItem("Move to last"))
          mpv->commandv("playlist-move", std::to_string(item.id).c_str(), std::to_string(items.size()).c_str(),
                        nullptr);
        ImGui::Separator();
        if (ImGui::MenuItem("Add Files")) mpv->command("script-message-to implay playlist-add-files");
        if (ImGui::MenuItem("Add Folders"))
          mpv->commandv("script-message-to", "implay", "playlist-add-folder", nullptr);
        ImGui::Separator();
        if (ImGui::MenuItem("Remove")) mpv->commandv("playlist-remove", std::to_string(item.id).c_str(), nullptr);
        if (ImGui::MenuItem("Clear")) mpv->command("playlist-clear");
        ImGui::EndPopup();
      }
      ImGui::SameLine();
      ImGui::TextColored(item.id == pos ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_Text], "%s",
                         title.c_str());
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
  ImGui::Spacing();

  if (ImGui::Button(ICON_FA_SEARCH))
    mpv->command("script-message-to implay command-palette playlist");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_SYNC)) mpv->command("cycle-values loop-playlist inf no");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Loop");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_RANDOM)) mpv->command("playlist-shuffle");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Shuffle");
  auto iconSize = ImGui::CalcTextSize(ICON_FA_PLUS);
  ImGui::SameLine(ImGui::GetWindowWidth() - 3 * (iconSize.x + 2 * style.FramePadding.x + style.ItemSpacing.x));
  if (ImGui::Button(ICON_FA_PLUS)) mpv->command("script-message-to implay playlist-add-files");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Add Files");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_FOLDER_PLUS)) mpv->command("script-message-to implay playlist-add-folder");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Add Folder");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_TRASH_ALT)) mpv->command("playlist-clear");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Clear");
}

void Quick::drawChaptersTabContent() {
  auto items = mpv->chapterList();
  auto style = ImGui::GetStyle();
  auto pos = mpv->property<int64_t, MPV_FORMAT_INT64>("chapter");
  if (ImGui::BeginListBox("##chapters", ImVec2(-FLT_MIN, -FLT_MIN))) {
    for (auto &item : items) {
      auto title = item.title.empty() ? format("Chapter {}", item.id + 1) : item.title;
      auto time = format("{:%H:%M:%S}", std::chrono::duration<int>((int)item.time));
      auto color = item.id == pos ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_Text];
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", item.id == pos))
        mpv->commandv("seek", std::to_string(item.time).c_str(), "absolute", nullptr);
      ImGui::SameLine();
      ImGui::TextColored(color, "%s", title.c_str());
      ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(time.c_str()).x - 2 * style.ItemSpacing.x);
      ImGui::TextColored(color, "%s", time.c_str());
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

void Quick::drawVideoTabContent() {
  drawTracks("video", "vid");
  ImGui::Spacing();

  ImGui::Text("Rotate");
  const char *rotates[] = {"0", "90", "180", "270"};
  for (auto rotate : rotates) {
    if (ImGui::Button(format("{}°", rotate).c_str())) mpv->commandv("set", "video-rotate", rotate, nullptr);
    ImGui::SameLine();
  }
  if (ImGui::Button(ICON_FA_MINUS "##Rotate")) mpv->command("add video-rotate -1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_PLUS "##Rotate")) mpv->command("add video-rotate 1");
  ImGui::Spacing();

  ImGui::Text("Scale");
  const float scales[] = {0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
  for (auto scale : scales) {
    if (ImGui::Button(format("{}%", (int)(scale * 100)).c_str()))
      mpv->commandv("set", "window-scale", std::to_string(scale).c_str(), nullptr);
    ImGui::SameLine();
  }
  if (ImGui::Button(ICON_FA_MINUS "##Scale")) mpv->command("add window-scale -0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_PLUS "##Scale")) mpv->command("add window-scale 0.1");
  ImGui::Spacing();

  ImGui::Text("PanScan");
  if (ImGui::Button(ICON_FA_SEARCH_MINUS)) mpv->command("add video-zoom -0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_SEARCH_PLUS)) mpv->command("add video-zoom 0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_MINUS "##PanScan")) mpv->command("add panscan -0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_PLUS "##PanScan")) mpv->command("add panscan 0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ARROW_LEFT)) mpv->command("add video-pan-x -0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ARROW_RIGHT)) mpv->command("add video-pan-x 0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ARROW_UP)) mpv->command("add video-pan-y -0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ARROW_DOWN)) mpv->command("add video-pan-y 0.1");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_REDO "##PanScan"))
    mpv->command("set video-zoom 0; set panscan 0; set video-pan-x 0 ; set video-pan-y 0");
  ImGui::Spacing();

  ImGui::Text("Aspect Ratio");
  const char *ratios[] = {"16:9", "16:10", "4:3", "2.35:1", "1.85:1", "1:1"};
  for (auto ratio : ratios) {
    if (ImGui::Button(ratio)) mpv->commandv("set", "video-aspect", ratio, nullptr);
    ImGui::SameLine();
  }
  if (ImGui::Button(ICON_FA_REDO "##Aspect Ratio")) mpv->command("set video-aspect -1");
  ImGui::SameLine();
  static char ratio[10] = {0};
  ImGui::SetNextItemWidth(scaled(4));
  if (ImGui::InputTextWithHint("##Aspect Ratio", "Custom", ratio, IM_ARRAYSIZE(ratio),
                               ImGuiInputTextFlags_EnterReturnsTrue)) {
    mpv->commandv("set", "video-aspect", ratio, nullptr);
    ratio[0] = '\0';
  }
  ImGui::Spacing();

  ImGui::Text("Misc");
  if (ImGui::Button("HW Decoding")) mpv->command("cycle-values hwdec auto no");
  ImGui::SameLine();
  if (ImGui::Button("Deinterlace")) mpv->command("cycle deinterlace");
  ImGui::Spacing();

  ImGui::Text("Equalizer");
  static int equalizer[5] = {0};
  const char *eq[] = {"brightness", "contrast", "saturation", "gamma", "hue"};
  for (int i = 0; i < IM_ARRAYSIZE(equalizer); i++) {
    if (ImGui::Button(format("{}##{}", ICON_FA_REDO, eq[i]).c_str())) {
      equalizer[i] = 0;
      mpv->commandv("set", eq[i], "0", nullptr);
    }
    ImGui::SameLine();
    std::string label = eq[i];
    label[0] = std::toupper(label[0]);
    if (ImGui::SliderInt(label.c_str(), &equalizer[i], -100, 100))
      mpv->commandv("set", eq[i], std::to_string(equalizer[i]).c_str(), nullptr);
  }
}

void Quick::drawAudioTabContent() {
  drawTracks("audio", "aid");
  ImGui::Spacing();

  ImGui::Text("Volume");
  static int volume = 100;
  if (ImGui::SliderInt("##Volume", &volume, 0, 200, "%d%%"))
    mpv->commandv("set", "volume", std::to_string(volume).c_str(), nullptr);
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_VOLUME_MUTE)) mpv->command("cycle mute");
  ImGui::Spacing();

  ImGui::Text("Delay");
  static float delay = 0;
  if (ImGui::SliderFloat("##Delay", &delay, -10, 10, "%.1fs"))
    mpv->commandv("set", "audio-delay", format("{:.1f}", delay).c_str(), nullptr);
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_REDO "##Delay")) mpv->command("set audio-delay 0");
}

void Quick::drawSubtitleTabContent() {
  drawTracks("sub", "sid");
  ImGui::Spacing();

  if (ImGui::Button(ICON_FA_ARROW_UP)) mpv->command("add sub-pos -1");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Move Subtitle Up");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ARROW_DOWN)) mpv->command("add sub-pos 1");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Move Subtitle Down");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_REDO "##Position")) mpv->command("set sub-pos 100");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Reset Subtitle Position");
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_EYE_SLASH)) mpv->command("cycle sub-visibility");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Show/Hide Subtitle");
  auto style = ImGui::GetStyle();
  auto iconSize = ImGui::CalcTextSize(ICON_FA_PLUS);
  ImGui::SameLine(ImGui::GetWindowWidth() - (iconSize.x + 2 * style.FramePadding.x));
  if (ImGui::Button(ICON_FA_PLUS)) mpv->command("script-message-to implay load-sub");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("Load External Subtitles..");
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("Scale");
  static float scale = 1;
  if (ImGui::SliderFloat("##Scale", &scale, 0, 4, "%.1f"))
    mpv->commandv("set", "sub-scale", format("{:.1f}", scale).c_str(), nullptr);
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_REDO "##Scale")) mpv->command("set sub-scale 1");
  ImGui::Spacing();

  ImGui::Text("Delay");
  static float delay = 0;
  if (ImGui::SliderFloat("##Delay", &delay, -10, 10, "%.1fs"))
    mpv->commandv("set", "sub-delay", format("{:.1f}", delay).c_str(), nullptr);
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_REDO "##Delay")) mpv->command("set sub-delay 0");
}
}  // namespace ImPlay::Views