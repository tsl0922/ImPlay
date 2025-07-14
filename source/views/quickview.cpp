#include <algorithm>
#include <fonts/fontawesome.h>
#include "helpers/utils.h"
#include "helpers/imgui.h"
#include "views/quickview.h"

namespace ImPlay::Views {
Quickview::Quickview(Config *config, Mpv *mpv) : View(config, mpv) {
  // clang-format off
  addTab("playlist", "views.quickview.playlist", [this]() { drawPlaylistTabContent(); });
  addTab("chapters", "views.quickview.chapters", [this]() { drawChaptersTabContent(); });
  addTab("video", "views.quickview.video", [this]() { drawVideoTabContent(); });
  addTab("audio", "views.quickview.audio", [this]() { drawAudioTabContent(); });
  addTab("subtitle", "views.quickview.subtitle", [this]() { drawSubtitleTabContent(); });
  // clang-format on

  mpv->observeEvent(MPV_EVENT_FILE_LOADED, [this](void *data) {
    updateAudioEqChannels();
    applyAudioEq(false);
  });
}

void Quickview::show(const char *tab) {
  if (tab != nullptr) {
    curTab = tab;
    tabSwitched = false;
  }
  View::show();
}

void Quickview::draw() {
  if (winMode)
    drawWindow();
  else
    drawPopup();
}

void Quickview::drawWindow() {
  if (!m_open) return;
  ImVec2 wPos = ImGui::GetMainViewport()->WorkPos;
  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  float width = std::min(wSize.x * 0.3f, scaled(30));
  float height = std::max(wSize.y * 0.8f, scaled(50));
  ImGui::SetNextWindowSizeConstraints(ImVec2(scaled(25), scaled(30)), ImVec2(FLT_MAX, FLT_MAX));
  ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - width, wPos.y + wSize.y * 0.5f), ImGuiCond_FirstUseEver,
                          ImVec2(0.2f, 0.5f));

  if (ImGui::Begin("menu.quickview"_i18n, &m_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
    drawTabBar();
  }
  ImGui::End();
}

void Quickview::drawPopup() {
  if (m_open) {
    ImGui::OpenPopup("##quickview");
    m_open = false;
  }
  ImVec2 wPos = ImGui::GetMainViewport()->WorkPos;
  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  float width = std::min(wSize.x * 0.5f, scaled(30));

  ImGui::SetNextWindowSize(ImVec2(width, wSize.y), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImVec2(wPos.x + wSize.x - width, wPos.y), ImGuiCond_Always);

  if (ImGui::BeginPopup("##quickview")) {
    drawTabBar();
    ImGui::EndPopup();
  }
}

void Quickview::alignRight(const char *label) {
  ImGui::SameLine(ImGui::GetContentRegionAvail().x -
                  (ImGui::CalcTextSize(label).x + 2 * ImGui::GetStyle().FramePadding.x));
}

bool Quickview::iconButton(const char *icon, const char *cmd, const char *tooltip, bool sameline) {
  if (sameline) ImGui::SameLine();
  bool ret = ImGui::Button(fmt::format("{}##{}", icon, cmd).c_str());
  if (ret) mpv->command(cmd);
  if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", tooltip);
  return ret;
}

bool Quickview::toggleButton(const char *label, bool toggle, const char *tooltip, ImGuiCol col) {
  ImGui::PushStyleColor(col, ImGui::GetStyleColorVec4(toggle ? ImGuiCol_CheckMark : col));
  bool ret = ImGui::Button(fmt::format("{}##{}", label, tooltip ? tooltip : "").c_str());
  if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", tooltip);
  ImGui::PopStyleColor();
  return ret;
}

bool Quickview::toggleButton(bool toggle, const char *tooltip, const char *id) {
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  const char *label = toggle ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF;
  bool ret = toggleButton(fmt::format("{}##{}", label, id ? id : "").c_str(), toggle, tooltip, ImGuiCol_Text);
  ImGui::PopStyleColor();
  return ret;
}

void Quickview::emptyLabel() {
  ImGui::BeginDisabled();
  ImGui::TextUnformatted("views.quickview.empty"_i18n);
  ImGui::EndDisabled();
}

void Quickview::drawTabBar() {
  if (ImGui::BeginTabBar("Quickviewview")) {
    for (auto &[name, title, draw] : tabs) {
      ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
      if (iequals(curTab, name) && !tabSwitched) {
        flags |= ImGuiTabItemFlags_SetSelected;
        tabSwitched = true;
      }
      if (ImGui::BeginTabItem(i18n(title).c_str(), nullptr, flags)) {
        if (ImGui::BeginChild(name.c_str())) draw();
        ImGui::EndChild();
        ImGui::EndTabItem();
      }
    }
    if (ImGui::TabItemButton(winMode ? ICON_FA_WINDOW_RESTORE : ICON_FA_WINDOW_MAXIMIZE)) {
      winMode = !winMode;
      show();
    }
    ImGui::EndTabBar();
  }
}

void Quickview::drawTracks(const char *title, const char *type, const char *prop, std::string pos) {
  ImGui::TextUnformatted(title);
  alignRight(ICON_FA_TOGGLE_ON);
  bool toggle = !iequals(pos, "no");
  if (strcmp(prop, "sid") == 0) toggle = mpv->sidv;
  if (strcmp(prop, "secondary-sid") == 0) toggle = mpv->sidv2;
  if (toggleButton(toggle, "views.quickview.tracks.toggle"_i18n, prop)) {
    if (strstr(prop, "sid") != nullptr) {
      const char *prefix = strstr(prop, "secondary") != nullptr ? "secondary-" : "";
      mpv->commandv("set", fmt::format("{}sub-visibility", prefix).c_str(), toggle ? "no" : "yes", nullptr);
    } else {
      mpv->commandv("cycle-values", prop, "no", "auto", nullptr);
    }
  }
  if (ImGui::BeginChild(fmt::format("##tracks-{}", prop).c_str(),
                        ImVec2(-FLT_MIN, 3 * ImGui::GetFrameHeightWithSpacing()),
                        ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY)) {
    auto items = mpv->tracks;
    if (items.empty())
      emptyLabel();
    else
      items.insert(items.begin(), {0, type, "views.quickview.tracks.no"_i18n, "", false});
    for (auto &item : items) {
      if (item.type != type) continue;
      bool selected = item.id == 0 ? pos == "no" : pos == std::to_string(item.id);
      auto title = item.title.empty() ? i18n_a("views.quickview.tracks.item", item.id) : item.title;
      if (!item.lang.empty()) title += fmt::format(" [{}]", item.lang);
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", selected)) mpv->property(prop, std::to_string(item.id).c_str());
      ImGui::SameLine();
      ImGui::TextColored(ImGui::GetStyleColorVec4(selected ? ImGuiCol_CheckMark : ImGuiCol_Text), "%s", title.c_str());
      ImGui::PopID();
    }
  }
  ImGui::EndChild();
}

void Quickview::drawTracks(const char *type, const char *prop, std::string pos) {
  drawTracks("views.quickview.tracks"_i18n, type, prop, pos);
}

void Quickview::drawPlaylistTabContent() {
  auto style = ImGui::GetStyle();
  auto pos = mpv->playlistPos;
  if (ImGui::BeginListBox("##playlist", ImVec2(-FLT_MIN, -ImGui::GetFrameHeightWithSpacing()))) {
    auto items = mpv->playlist;
    static int selected = pos;
    auto drawContextmenu = [&](Mpv::PlayItem *item) {
      if (ImGui::MenuItem("views.quickview.playlist.menu.play"_i18n))
        mpv->commandv("playlist-play-index", std::to_string(item->id).c_str(), nullptr);
      if (ImGui::MenuItem("views.quickview.playlist.menu.play_next"_i18n))
        mpv->commandv("playlist-move", std::to_string(item->id).c_str(), std::to_string(pos + 1).c_str(), nullptr);
      ImGui::Separator();
      if (ImGui::MenuItem("views.quickview.playlist.menu.move_up"_i18n, nullptr, false, item->id > 0))
        mpv->commandv("playlist-move", std::to_string(item->id).c_str(), std::to_string(item->id - 1).c_str(), nullptr);
      if (ImGui::MenuItem("views.quickview.playlist.menu.move_down"_i18n, nullptr, false,
                          item->id < (int)items.size() - 1))
        mpv->commandv("playlist-move", std::to_string(item->id + 1).c_str(), std::to_string(item->id).c_str(), nullptr);
      if (ImGui::MenuItem("views.quickview.playlist.menu.move_first"_i18n))
        mpv->commandv("playlist-move", std::to_string(item->id).c_str(), "0", nullptr);
      if (ImGui::MenuItem("views.quickview.playlist.menu.move_last"_i18n))
        mpv->commandv("playlist-move", std::to_string(item->id).c_str(), std::to_string(items.size()).c_str(), nullptr);
      if (ImGui::MenuItem("views.quickview.playlist.menu.remove"_i18n))
        mpv->commandv("playlist-remove", std::to_string(item->id).c_str(), nullptr);
      ImGui::Separator();
      if (ImGui::MenuItem("views.quickview.playlist.menu.copy_path"_i18n))
        ImGui::SetClipboardText(item->path.string().c_str());
      if (ImGui::MenuItem("views.quickview.playlist.menu.reveal"_i18n)) revealInFolder(item->path.string());
    };

    if (items.empty()) emptyLabel();
    for (auto &item : items) {
      std::string title = item.title;
      if (title.empty() && !item.filename().empty()) title = item.filename();
      if (title.empty()) title = i18n_a("views.quickview.playlist.item", item.id + 1);
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", selected == item.id)) selected = item.id;
      if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        mpv->commandv("playlist-play-index", std::to_string(item.id).c_str(), nullptr);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) ImGui::SetTooltip("%s", title.c_str());
      if (ImGui::BeginPopupContextItem()) {
        drawContextmenu(&item);
        ImGui::EndPopup();
      }
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text,
                            ImGui::GetStyleColorVec4(item.id == pos ? ImGuiCol_CheckMark : ImGuiCol_Text));
      ImGui::TextEllipsis(title.c_str());
      if (ImGui::IsWindowAppearing() && item.id == pos) ImGui::SetScrollHereY(0.25f);
      ImGui::PopStyleColor();
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }

  static bool sort = true;
  iconButton(ICON_FA_SEARCH, "script-message-to implay command-palette playlist",
             "views.quickview.playlist.search"_i18n, false);
  iconButton(ICON_FA_SYNC, "cycle-values loop-playlist inf no", "views.quickview.playlist.loop"_i18n);
  iconButton(ICON_FA_RANDOM, "playlist-shuffle", "views.quickview.playlist.shuffle"_i18n);
  if (iconButton(sort ? ICON_FA_SORT_ALPHA_DOWN : ICON_FA_SORT_ALPHA_UP,
                 fmt::format("script-message-to implay playlist-sort {}", sort).c_str(),
                 "views.quickview.playlist.sort"_i18n))
    sort = !sort;
  ImGui::SameLine(ImGui::GetContentRegionAvail().x -
                  3 * (ImGui::CalcTextSize(ICON_FA_PLUS).x + style.FramePadding.x + style.ItemSpacing.x));
  iconButton(ICON_FA_PLUS, "script-message-to implay playlist-add-files", "views.quickview.playlist.add_files"_i18n,
             false);
  iconButton(ICON_FA_FOLDER_PLUS, "script-message-to implay playlist-add-folder",
             "views.quickview.playlist.add_folders"_i18n);
  iconButton(ICON_FA_TRASH_ALT, "playlist-clear", "views.quickview.playlist.clear"_i18n);
}

void Quickview::drawChaptersTabContent() {
  auto items = mpv->chapters;
  auto pos = mpv->chapter;
  if (ImGui::BeginListBox("##chapters", ImVec2(-FLT_MIN, -FLT_MIN))) {
    if (items.empty()) emptyLabel();
    for (auto &item : items) {
      auto title = item.title.empty() ? fmt::format("Chapter {}", item.id + 1) : item.title;
      auto time = fmt::format("{:%H:%M:%S}", std::chrono::duration<int>((int)item.time));
      auto color = ImGui::GetStyleColorVec4(item.id == pos ? ImGuiCol_CheckMark : ImGuiCol_Text);
      ImGui::PushID(item.id);
      if (ImGui::Selectable("", item.id == pos))
        mpv->commandv("seek", std::to_string(item.time).c_str(), "absolute", nullptr);
      ImGui::SameLine();
      ImGui::TextColored(color, "%s", title.c_str());
      alignRight(time.c_str());
      ImGui::TextColored(color, "%s", time.c_str());
      if (ImGui::IsWindowAppearing() && item.id == pos) ImGui::SetScrollHereY(0.25f);
      ImGui::PopID();
    }
    ImGui::EndListBox();
  }
}

void Quickview::drawVideoTabContent() {
  drawTracks("video", "vid", mpv->vid);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.quality"_i18n);
  ImGui::SameLine();
  ImGui::HelpMarker("views.quickview.video.quality.help"_i18n);
  const char *qualities[] = {"4320", "2160", "1440", "1080", "720", "480", "360", "240", "144"};
  for (auto quality : qualities) {
    if (ImGui::Button(fmt::format("{}p", quality).c_str())) {
      mpv->property("ytdl-format", fmt::format("bv*[height<={}]+ba/b[height<={}]", quality, quality).c_str());
      if (mpv->playing()) {
        mpv->property("start", fmt::format("+{}", mpv->timePos).c_str());
        mpv->command("playlist-play-index current");
      }
    }
    ImGui::SameLine();
  }
  ImGui::NewLine();
  ImGui::Separator();
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.rotate"_i18n);
  const char *rotates[] = {"0", "90", "180", "270"};
  for (auto rotate : rotates) {
    if (ImGui::Button(fmt::format("{}°", rotate).c_str())) mpv->commandv("set", "video-rotate", rotate, nullptr);
    ImGui::SameLine();
  }
  iconButton(ICON_FA_UNDO, "add video-rotate -1", "views.quickview.video.rotate_left"_i18n, false);
  iconButton(ICON_FA_REDO, "add video-rotate 1", "views.quickview.video.rotate_right"_i18n, true);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.scale"_i18n);
  ImGui::SameLine();
  ImGui::HelpMarker("views.quickview.video.scale.help"_i18n);
  const float scales[] = {0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
  for (auto scale : scales) {
    if (ImGui::Button(fmt::format("{}%", (int)(scale * 100)).c_str()))
      mpv->commandv("set", "window-scale", std::to_string(scale).c_str(), nullptr);
    ImGui::SameLine();
  }
  iconButton(ICON_FA_MINUS, "add window-scale -0.1", "views.quickview.video.scale_down"_i18n, false);
  iconButton(ICON_FA_PLUS, "add window-scale 0.1", "views.quickview.video.scale_up"_i18n);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.panscan"_i18n);
  ImGui::SameLine();
  ImGui::HelpMarker("views.quickview.video.panscan.help"_i18n);
  iconButton(ICON_FA_MINUS, "add panscan -0.1", "views.quickview.video.panscan.dec"_i18n, false);
  iconButton(ICON_FA_PLUS, "add panscan 0.1", "views.quickview.video.panscan.inc"_i18n);
  iconButton(ICON_FA_ARROW_LEFT, "add video-pan-x -0.1", "views.quickview.video.panscan.move_left"_i18n);
  iconButton(ICON_FA_ARROW_RIGHT, "add video-pan-x 0.1", "views.quickview.video.panscan.move_right"_i18n);
  iconButton(ICON_FA_ARROW_UP, "add video-pan-y -0.1", "views.quickview.video.panscan.move_up"_i18n);
  iconButton(ICON_FA_ARROW_DOWN, "add video-pan-y 0.1", "views.quickview.video.panscan.move_down"_i18n);
  iconButton(ICON_FA_SEARCH_MINUS, "add video-zoom -0.1", "views.quickview.video.panscan.zoom_in"_i18n);
  iconButton(ICON_FA_SEARCH_PLUS, "add video-zoom 0.1", "views.quickview.video.panscan.zoom_out"_i18n);
  iconButton(ICON_FA_UNDO, "set video-zoom 0; set panscan 0; set video-pan-x 0 ; set video-pan-y 0",
             "views.quickview.video.panscan.reset"_i18n);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.aspect_ratio"_i18n);
  const char *ratios[] = {"16:9", "16:10", "4:3", "2.35:1", "1.85:1", "1:1"};
  for (auto ratio : ratios) {
    if (ImGui::Button(ratio)) mpv->commandv("set", "video-aspect-override", ratio, nullptr);
    ImGui::SameLine();
  }
  iconButton(ICON_FA_UNDO, "set video-aspect-override -1", "views.quickview.video.aspect_ratio.reset"_i18n, false);
  ImGui::SameLine();
  static char ratio[10] = {0};
  ImGui::SetNextItemWidth(scaled(4));
  if (ImGui::InputTextWithHint("##Aspect Ratio", "views.quickview.video.aspect_ratio.custom"_i18n, ratio,
                               IM_ARRAYSIZE(ratio), ImGuiInputTextFlags_EnterReturnsTrue)) {
    mpv->commandv("set", "video-aspect-override", ratio, nullptr);
    ratio[0] = '\0';
  }
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.misc"_i18n);
  if (ImGui::Button("views.quickview.video.hw_decoding"_i18n)) mpv->command("cycle-values hwdec auto no");
  ImGui::SameLine();
  if (ImGui::Button("views.quickview.video.deinterlace"_i18n)) mpv->command("cycle deinterlace");
  ImGui::NewLine();
  ImGui::Separator();
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.video.equalizer"_i18n);
  ImGui::Spacing();
  static const char *eq[] = {"brightness", "contrast", "saturation", "gamma", "hue"};
  static std::vector<std::string> eq_labels = {
      "views.quickview.video.equalizer.brightness", "views.quickview.video.equalizer.contrast",
      "views.quickview.video.equalizer.saturation", "views.quickview.video.equalizer.gamma",
      "views.quickview.video.equalizer.hue",
  };
  int equalizer[IM_ARRAYSIZE(eq)] = {
      (int)mpv->brightness, (int)mpv->contrast, (int)mpv->saturation, (int)mpv->gamma, (int)mpv->hue,
  };
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + scaled(1));
  ImGui::BeginGroup();
  for (int i = 0; i < IM_ARRAYSIZE(equalizer); i++) {
    if (ImGui::Button(fmt::format("{}##{}", ICON_FA_UNDO, eq[i]).c_str())) mpv->commandv("set", eq[i], "0", nullptr);
    ImGui::SameLine();
    if (ImGui::SliderInt(i18n(eq_labels[i]).c_str(), &equalizer[i], -100, 100))
      mpv->commandv("set", eq[i], std::to_string(equalizer[i]).c_str(), nullptr);
  }
  ImGui::EndGroup();
}

void Quickview::drawAudioTabContent() {
  drawTracks("audio", "aid", mpv->aid);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.audio.volume"_i18n);
  int volume = (int)mpv->volume;
  if (ImGui::SliderInt("##Volume", &volume, 0, 200, "%d%%"))
    mpv->commandv("set", "volume", std::to_string(volume).c_str(), nullptr);
  ImGui::SameLine();
  if (toggleButton(ICON_FA_VOLUME_MUTE, mpv->mute, "views.quickview.audio.mute"_i18n)) mpv->command("cycle mute");
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.audio.delay"_i18n);
  float delay = (float)mpv->audioDelay;
  if (ImGui::SliderFloat("##Delay", &delay, -10, 10, "%.1fs"))
    mpv->commandv("set", "audio-delay", fmt::format("{:.1f}", delay).c_str(), nullptr);
  iconButton(ICON_FA_UNDO, "set audio-delay 0", "views.quickview.audio.delay.reset"_i18n);
  ImGui::NewLine();
  ImGui::Separator();
  ImGui::NewLine();

  drawAudioEq();
}

void Quickview::drawSubtitleTabContent() {
  drawTracks("sub", "sid", mpv->sid);

  iconButton(ICON_FA_ARROW_UP, "add sub-pos -1", "views.quickview.subtitle.move_up"_i18n, false);
  iconButton(ICON_FA_ARROW_DOWN, "add sub-pos 1", "views.quickview.subtitle.move_down"_i18n);
  iconButton(ICON_FA_REDO, "set sub-pos 100", "views.quickview.subtitle.reset_pos"_i18n);
  alignRight(ICON_FA_PLUS);
  iconButton(ICON_FA_PLUS, "script-message-to implay load-sub", "views.quickview.subtitle.load"_i18n, false);
  ImGui::Separator();
  ImGui::NewLine();

  drawTracks("views.quickview.subtitle.secondary"_i18n, "sub", "secondary-sid", mpv->sid2);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.subtitle.scale"_i18n);
  float scale = (float)mpv->subScale;
  if (ImGui::SliderFloat("##Scale", &scale, 0, 4, "%.1f"))
    mpv->commandv("set", "sub-scale", fmt::format("{:.1f}", scale).c_str(), nullptr);
  iconButton(ICON_FA_UNDO, "set sub-scale 1", "views.quickview.subtitle.scale.reset"_i18n);
  ImGui::NewLine();

  ImGui::TextUnformatted("views.quickview.subtitle.delay"_i18n);
  float delay = (float)mpv->subDelay;
  if (ImGui::SliderFloat("##Delay", &delay, -10, 10, "%.1fs"))
    mpv->commandv("set", "sub-delay", fmt::format("{:.1f}", delay).c_str(), nullptr);
  iconButton(ICON_FA_UNDO, "set sub-delay 0", "views.quickview.subtitle.delay.reset"_i18n);
}

void Quickview::drawAudioEq() {
  ImGui::TextUnformatted("views.quickview.audio.equalizer"_i18n);
  alignRight(ICON_FA_TOGGLE_ON);
  if (toggleButton(audioEqEnabled, "views.quickview.audio.equalizer.toggle"_i18n)) toggleAudioEq();
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
    if (toggleButton(i18n(item.name).c_str(), audioEqIndex == i)) {
      selectAudioEq(i);
      for (int j = 0; j < FREQ_COUNT; j++) gain[j] = (double)item.values[j] / 12;
    }
    lineWidth += ImGui::GetItemRectSize().x + ImGui::GetStyle().ItemSpacing.x;
    if (i < pSize - 1) {
      auto textSize = ImGui::CalcTextSize(i18n(audioEqPresets[i + 1].name).c_str());
      if (lineWidth + textSize.x + 2 * ImGui::GetStyle().ItemInnerSpacing.x < availWidth)
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
    std::string label = fmt::format("##{}", audioEqFreqs[i]);
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

void Quickview::applyAudioEq(bool osd) {
  std::string message = "views.quickview.audio.equalizer.msg.disabled"_i18n;
  mpv->command("no-osd af remove @aeq");
  if (audioEqEnabled) {
    if (audioEqIndex < 0) return;
    auto equalizer = audioEqPresets[audioEqIndex];
    mpv->commandv("af", "add", equalizer.toFilter("@aeq", audioEqChannels).c_str(), nullptr);
    message = i18n_a("views.quickview.audio.equalizer.msg", i18n(equalizer.name));
  }
  if (osd) mpv->commandv("show-text", message.c_str(), nullptr);
}

void Quickview::toggleAudioEq() {
  audioEqEnabled = !audioEqEnabled;
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
        break;
      }
    }
  }
  mpv_free_node_contents(&node);
}

std::string Quickview::AudioEqItem::toFilter(const char *name, int channels) {
  std::string s;
  double freq = 31.25;
  for (int ch = 0; ch < channels; ch++) {
    for (int f = 0; f < FREQ_COUNT; f++) {
      double v = (double)values[f] / 12;
      s += fmt::format("c{} f={} w={} g={}|", ch, freq, 1000, v);
      freq *= 2;
    }
  }
  return fmt::format("{}:lavfi=[anequalizer={}]", name, s);
}
}  // namespace ImPlay::Views