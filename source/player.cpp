// Copyright (c) 2022-2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include <thread>
#include <romfs/romfs.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <fonts/cascadia.h>
#include <fonts/fontawesome.h>
#include <fonts/unifont.h>
#include <strnatcmp.h>
#include "theme.h"
#include "player.h"

namespace ImPlay {
Player::Player(Config *config) : config(config) {
  mpv = new Mpv();

  about = new Views::About();
  debug = new Views::Debug(config, mpv);
  quickview = new Views::Quickview(config, mpv);
  settings = new Views::Settings(config, mpv);
  contextMenu = new Views::ContextMenu(config, mpv);
  commandPalette = new Views::CommandPalette(config, mpv);
}

Player::~Player() {
  delete about;
  delete debug;
  delete quickview;
  delete settings;
  delete contextMenu;
  delete commandPalette;
  delete mpv;
}

bool Player::init(std::map<std::string, std::string> &options) {
  mpv->option("config", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");
  mpv->option("load-osd-console", "no");
  mpv->option("osd-playing-msg", "${media-title}");
  mpv->option("screenshot-directory", "~~desktop/");

  // override-display-fps is renamed to display-fps-override in mpv 0.37.0
  mpv->option<int64_t, MPV_FORMAT_INT64>("override-display-fps", GetMonitorRefreshRate());
  mpv->option<int64_t, MPV_FORMAT_INT64>("display-fps-override", GetMonitorRefreshRate());

  if (!config->Data.Mpv.UseConfig) {
    writeMpvConf();
    mpv->option("osc", "no");
    mpv->option("config-dir", config->dir().c_str());
  }

  if (config->Data.Window.Single) mpv->option("input-ipc-server", config->ipcSocket().c_str());

  for (const auto &[key, value] : options) {
    if (int err = mpv->option(key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return false;
    }
  }

  debug->init();

  {
    ContextGuard guard(this);
    logoTexture = ImGui::LoadTexture("icon.png");
    mpv->init(GetGLAddrFunc(), GetWid());
  }

  SetWindowDecorated(mpv->property<int, MPV_FORMAT_FLAG>("border"));
  mpv->property<int64_t, MPV_FORMAT_INT64>("volume", config->Data.Mpv.Volume);
  if (config->Data.Recent.SpaceToPlayLast) mpv->command("keybind SPACE 'script-message-to implay play-pause'");
  initObservers();

  return true;
}

void Player::draw() {
  drawVideo();

  about->draw();
  debug->draw();
  quickview->draw();
  settings->draw();
  contextMenu->draw();
  commandPalette->draw();

  drawOpenURL();
  drawDialog();
}

void Player::drawVideo() {
  auto vp = ImGui::GetMainViewport();
  auto drawList = ImGui::GetBackgroundDrawList(vp);

  if (!idle) {
    ImTextureID texture = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex));
    drawList->AddImage(texture, vp->WorkPos, vp->WorkPos + vp->WorkSize);
  } else if (logoTexture != nullptr && !mpv->forceWindow) {
    const ImVec2 center = vp->GetWorkCenter();
    const ImVec2 delta(64, 64);
    drawList->AddImage(logoTexture, center - delta, center + delta);
  }
}

void Player::render() {
  auto g = ImGui::GetCurrentContext();
  if (g != nullptr && g->WithinFrameScope) return;

  {
    ContextGuard guard(this);

    if (idle) {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (config->FontReload) {
      loadFonts();
      ImGui_ImplOpenGL3_DestroyFontsTexture();
      ImGui_ImplOpenGL3_CreateFontsTexture();
      config->FontReload = false;
    }
    ImGui_ImplOpenGL3_NewFrame();
  }

  BackendNewFrame();
  ImGui::NewFrame();

#if defined(_WIN32) && defined(IMGUI_HAS_VIEWPORT)
  if (config->Data.Mpv.UseWid) {
    ImGuiViewport *vp = ImGui::GetMainViewport();
    vp->Flags &= ~ImGuiViewportFlags_CanHostOtherWindows;  // HACK: disable main viewport merge
  }
#endif

  draw();
  ImGui::Render();

  {
    ContextGuard guard(this);
    GetFramebufferSize(&width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SetSwapInterval(config->Data.Interface.Fps > 60 ? 0 : 1);
    SwapBuffers();
    mpv->reportSwap();

#ifdef IMGUI_HAS_VIEWPORT
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }
#endif
  }
}

void Player::renderVideo() {
  ContextGuard guard(this);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  mpv->render(width, height, fbo, false);
}

void Player::initGui() {
  ContextGuard guard(this);

#ifdef IMGUI_IMPL_OPENGL_ES3
  if (!gladLoadGLES2((GLADloadfunc)GetGLAddrFunc())) throw std::runtime_error("Failed to load GLES 2!");
#else
  if (!gladLoadGL((GLADloadfunc)GetGLAddrFunc())) throw std::runtime_error("Failed to load GL!");
#endif
  SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
  if (config->Data.Interface.Docking) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
  if (config->Data.Interface.Viewports) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

  loadFonts();

  glGenFramebuffers(1, &fbo);
  glGenTextures(1, &tex);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef IMGUI_IMPL_OPENGL_ES3
  ImGui_ImplOpenGL3_Init("#version 300 es");
#elif defined(__APPLE__)
  ImGui_ImplOpenGL3_Init("#version 150");
#else
  ImGui_ImplOpenGL3_Init("#version 130");
#endif
}

void Player::exitGui() {
  MakeContextCurrent();

  ImGui_ImplOpenGL3_Shutdown();
  glDeleteTextures(1, &tex);
  glDeleteFramebuffers(1, &fbo);

  ImGui::DestroyContext();
}

void Player::saveState() {
  if (config->Data.Window.Save) {
    GetWindowPos(&config->Data.Window.X, &config->Data.Window.Y);
    GetWindowSize(&config->Data.Window.W, &config->Data.Window.H);
  }
  config->Data.Mpv.Volume = mpv->volume;
  config->save();
}

void Player::restoreState() {
  int mw, mh;
  GetMonitorSize(&mw, &mh);
  int w = std::max((int)(mw * 0.4), 600);
  int h = std::max((int)(mh * 0.4), 400);
  int x = (mw - w) / 2;
  int y = (mh - h) / 2;
  if (config->Data.Window.Save) {
    if (config->Data.Window.W > 0) w = config->Data.Window.W;
    if (config->Data.Window.H > 0) h = config->Data.Window.H;
    if (config->Data.Window.X >= 0) x = config->Data.Window.X;
    if (config->Data.Window.Y >= 0) y = config->Data.Window.Y;
  }
  SetWindowSize(w, h);
  SetWindowPos(x, y);
}

void Player::loadFonts() {
  auto interface = config->Data.Interface;
  float fontSize = config->Data.Font.Size;
  float iconSize = fontSize - 2;
  float scale = interface.Scale;
  if (scale == 0) {
    float xscale, yscale;
    GetWindowScale(&xscale, &yscale);
    scale = std::max(xscale, yscale);
  }
  if (scale <= 0) scale = 1.0f;
  fontSize = std::floor(fontSize * scale);
  iconSize = std::floor(iconSize * scale);

  ImGuiStyle style;
  ImGui::SetTheme(interface.Theme.c_str(), &style, interface.Rounding, interface.Shadow);

  ImGuiIO &io = ImGui::GetIO();
#if defined(_WIN32) && defined(IMGUI_HAS_VIEWPORT)
  if (config->Data.Mpv.UseWid) io.ConfigViewportsNoAutoMerge = true;
#endif

#ifdef __APPLE__
  io.FontGlobalScale = 1.0f / scale;
#else
  style.ScaleAllSizes(scale);
#endif
  ImGui::GetStyle() = style;

  io.Fonts->Clear();

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;

  const ImWchar *font_range = config->buildGlyphRanges();
  if (fileExists(config->Data.Font.Path))
    io.Fonts->AddFontFromFileTTF(config->Data.Font.Path.c_str(), 0, &cfg, font_range);
  else
    io.Fonts->AddFontFromMemoryCompressedTTF(unifont_compressed_data, unifont_compressed_size, 0, &cfg, font_range);

  cfg.MergeMode = true;

  static ImWchar fa_range[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(fa_compressed_data, fa_compressed_size, iconSize, &cfg, fa_range);
  io.Fonts->AddFontFromMemoryCompressedTTF(cascadia_compressed_data, cascadia_compressed_size, fontSize);

  io.Fonts->Build();
}

void Player::shutdown() { mpv->command(config->Data.Mpv.WatchLater ? "quit-watch-later" : "quit"); }

void Player::onCursorEvent(double x, double y) {
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  mpv->commandv("mouse", xs.c_str(), ys.c_str(), nullptr);
}

void Player::onScrollEvent(double x, double y) {
  if (abs(x) > 0) onKeyEvent(x > 0 ? "WHEEL_LEFT" : "WHEEL_RIGH");
  if (abs(y) > 0) onKeyEvent(y > 0 ? "WHEEL_UP" : "WHEEL_DOWN");
}

void Player::onKeyEvent(std::string name) { mpv->commandv("keypress", name.c_str(), nullptr); }

void Player::onKeyDownEvent(std::string name) { mpv->commandv("keydown", name.c_str(), nullptr); }

void Player::onKeyUpEvent(std::string name) { mpv->commandv("keyup", name.c_str(), nullptr); }

void Player::onDropEvent(int count, const char **paths) {
  std::sort(paths, paths + count, [](const auto &a, const auto &b) { return strnatcasecmp(a, b) < 0; });
  std::vector<std::filesystem::path> files;
  for (int i = 0; i < count; i++) files.emplace_back(reinterpret_cast<char8_t *>(const_cast<char *>(paths[i])));
  load(files);
}

void Player::updateWindowState() {
  int width = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
  int height = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
  if (width > 0 && height > 0) {
    int x, y, w, h;
    GetWindowPos(&x, &y);
    GetWindowSize(&w, &h);
    if ((w != width || h != height) && mpv->autoResize) {
      SetWindowSize(width, height);
      SetWindowPos(x + (w - width) / 2, y + (h - height) / 2);
    }
    if (mpv->keepaspect && mpv->keepaspectWindow) SetWindowAspectRatio(width, height);
  }
}

void Player::initObservers() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [this](void *data) { SetWindowShouldClose(true); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [this](void *data) {
    if (!mpv->fullscreen) updateWindowState();
  });

  mpv->observeEvent(MPV_EVENT_FILE_LOADED, [this](void *data) {
    auto path = mpv->property("path");
    if (path != "" && path != "bd://" && path != "dvd://") config->addRecentFile(path, mpv->property("media-title"));
    mpv->property("force-media-title", "");
    mpv->property("start", "none");
  });

  mpv->observeEvent(MPV_EVENT_CLIENT_MESSAGE, [this](void *data) {
    auto msg = static_cast<mpv_event_client_message *>(data);
    execute(msg->num_args, msg->args);
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("idle-active", [this](int flag) {
    idle = static_cast<bool>(flag);
    if (idle) {
      SetWindowTitle(PLAYER_NAME);
      SetWindowAspectRatio(-1, -1);
    }
  });

  mpv->observeProperty<char *, MPV_FORMAT_STRING>("media-title", [this](char *data) { SetWindowTitle(data); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("border", [this](int flag) { SetWindowDecorated(flag); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("ontop", [this](int flag) { SetWindowFloating(flag); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("window-maximized", [this](int flag) { SetWindowMaximized(flag); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("window-minimized", [this](int flag) { SetWindowMinimized(flag); });
  mpv->observeProperty<double, MPV_FORMAT_DOUBLE>("window-scale", [this](double scale) {
    int w = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    int h = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (w > 0 && h > 0) SetWindowSize((int)(w * scale), (int)(h * scale));
  });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("fullscreen", [this](int flag) { SetWindowFullscreen(flag); });
}

void Player::writeMpvConf() {
  auto path = dataPath();
  auto mpvConf = path / "mpv.conf";
  auto inputConf = path / "input.conf";

  if (!std::filesystem::exists(mpvConf)) {
    std::ofstream file(mpvConf, std::ios::binary);
    auto content = romfs::get("mpv/mpv.conf");
    file.write(reinterpret_cast<const char *>(content.data()), content.size()) << "\n";
    file << "# use opengl-hq video output for high-quality video rendering.\n";
    file << "profile=gpu-hq\n";
    file << "deband=no\n\n";
    file << "# Enable hardware decoding if available.\n";
    file << "hwdec=auto\n";
  }

  if (!std::filesystem::exists(inputConf)) {
    std::ofstream file(inputConf, std::ios::binary);
    auto content = romfs::get("mpv/input.conf");
    file.write(reinterpret_cast<const char *>(content.data()), content.size()) << "\n";
    file << "MBTN_RIGHT   script-message-to implay context-menu    # show context menu\n";
#ifdef __APPLE__
    file << "Meta+Shift+p script-message-to implay command-palette # show command palette\n";
#else
    file << "Ctrl+Shift+p script-message-to implay command-palette # show command palette\n";
#endif
    file << "`            script-message-to implay metrics         # open console window\n";
  }

  auto scrips = path / "scripts";
  auto scriptOpts = path / "script-opts";

  std::filesystem::create_directories(scrips);
  std::filesystem::create_directories(scriptOpts);

  auto oscLua = scrips / "osc.lua";

  if (!std::filesystem::exists(oscLua)) {
    std::ofstream file(oscLua, std::ios::binary);
    auto content = romfs::get("mpv/osc.lua").span<char>();
    file.write(content.data(), content.size());
  }
}

void Player::execute(int n_args, const char **args_) {
  if (n_args == 0) return;

  static std::map<std::string, std::function<void(int, const char **)>> commands = {
      {"open", [&](int n, const char **args) { openFilesDlg(mediaFilters); }},
      {"open-folder", [&](int n, const char **args) { openFolderDlg(); }},
      {"open-disk", [&](int n, const char **args) { openFolderDlg(false, true); }},
      {"open-iso", [&](int n, const char **args) { openFileDlg(isoFilters); }},
      {"open-clipboard", [&](int n, const char **args) { openClipboard(); }},
      {"open-url", [&](int n, const char **args) { openURL(); }},
      {"open-config-dir", [&](int n, const char **args) { openUrl(config->dir()); }},
      {"load-sub", [&](int n, const char **args) { openFilesDlg(subtitleFilters); }},
      {"load-conf",
       [&](int n, const char **args) {
         if (n > 0) mpv->loadConfig(args[0]);
       }},
      {"quickview", [&](int n, const char **args) { quickview->show(n > 0 ? args[0] : nullptr); }},
      {"playlist-add-files", [&](int n, const char **args) { openFilesDlg(mediaFilters, true); }},
      {"playlist-add-folder", [&](int n, const char **args) { openFolderDlg(true); }},
      {"playlist-sort", [&](int n, const char **args) { playlistSort(n > 0 && strcmp(args[0], "true") == 0); }},
      {"play-pause",
       [&](int n, const char **args) {
         auto count = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-count");
         if (count > 0)
           mpv->command("cycle pause");
         else if (config->getRecentFiles().size() > 0) {
           for (auto &file : config->getRecentFiles()) {
             if (fileExists(file.path) || file.path.find("://") != std::string::npos) {
               mpv->commandv("loadfile", file.path.c_str(), nullptr);
               mpv->commandv("set", "force-media-title", file.title.c_str(), nullptr);
               break;
             }
           }
         }
       }},
      {"about", [&](int n, const char **args) { about->show(); }},
      {"settings", [&](int n, const char **args) { settings->show(); }},
      {"metrics", [&](int n, const char **args) { debug->show(); }},
      {"command-palette", [&](int n, const char **args) { commandPalette->show(n, args); }},
      {"context-menu", [&](int n, const char **args) { contextMenu->show(); }},
      {"show-message",
       [&](int n, const char **args) {
         if (n > 1) messageBox(args[0], args[1]);
       }},
      {"theme",
       [&](int n, const char **args) {
         if (n > 0) ImGui::SetTheme(args[0], nullptr, config->Data.Interface.Rounding, config->Data.Interface.Shadow);
       }},
  };

  const char *cmd = args_[0];
  auto it = commands.find(cmd);
  try {
    if (it != commands.end()) it->second(n_args - 1, args_ + 1);
  } catch (const std::exception &e) {
    messageBox("Error", fmt::format("{}: {}", cmd, e.what()));
  }
}

void Player::openFileDlg(NFD::Filters filters, bool append) {
  mpv->command("set pause yes");
  if (auto res = NFD::openFile(filters)) load({*res}, append);
  mpv->command("set pause no");
}

void Player::openFilesDlg(NFD::Filters filters, bool append) {
  mpv->command("set pause yes");
  if (auto res = NFD::openFiles(filters)) load(*res, append);
  mpv->command("set pause no");
}

void Player::openFolderDlg(bool append, bool disk) {
  mpv->command("set pause yes");
  if (auto res = NFD::openFolder()) load({*res}, append, disk);
  mpv->command("set pause no");
}

void Player::openClipboard() {
  auto content = GetClipboardString();
  if (content != "") {
    auto str = trim(content);
    mpv->commandv("loadfile", str.c_str(), nullptr);
    mpv->commandv("show-text", str.c_str(), nullptr);
  }
}

void Player::openURL() { m_openURL = true; }

void Player::openDvd(std::filesystem::path path) {
  mpv->property("dvd-device", path.string().c_str());
  mpv->commandv("loadfile", "dvd://", nullptr);
}

void Player::openBluray(std::filesystem::path path) {
  mpv->property("bluray-device", path.string().c_str());
  mpv->commandv("loadfile", "bd://", nullptr);
}

void Player::playlistSort(bool reverse) {
  if (mpv->playlist.empty()) return;
  std::vector<Mpv::PlayItem> items(mpv->playlist);
  std::sort(items.begin(), items.end(), [&](const auto &a, const auto &b) {
    std::string str1 = a.title != "" ? a.title : a.filename();
    std::string str2 = b.title != "" ? b.title : b.filename();
    return strnatcasecmp(str1.c_str(), str2.c_str()) < 0;
  });
  if (reverse) std::reverse(items.begin(), items.end());

  int64_t timePos = mpv->timePos;
  int64_t pos = -1;
  for (int i = 0; i < items.size(); i++) {
    if (items[i].id == mpv->playlistPos) {
      pos = i;
      break;
    }
  }
  std::vector<std::string> playlist = {"#EXTM3U"};
  for (auto &item : items) {
    if (item.title != "") playlist.push_back(fmt::format("#EXTINF:-1,{}", item.title));
    playlist.push_back(item.path.string());
  }
  mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-start", pos);
  mpv->property("start", fmt::format("+{}", timePos).c_str());
  if (!mpv->playing()) mpv->command("playlist-clear");
  mpv->commandv("loadlist", fmt::format("memory://{}", join(playlist, "\n")).c_str(),
                mpv->playing() ? "replace" : "append", nullptr);
}

void Player::load(std::vector<std::filesystem::path> files, bool append, bool disk) {
  int i = 0;
  for (auto &file : files) {
    if (std::filesystem::is_directory(file)) {
      if (disk) {
        if (std::filesystem::exists(file / u8"BDMV"))
          openBluray(file);
        else
          openDvd(file);
        break;
      }
      for (const auto &entry : std::filesystem::recursive_directory_iterator(file)) {
        auto path = entry.path().string();
        if (isMediaFile(path)) {
          const char *action = append ? "append" : (i > 0 ? "append-play" : "replace");
          mpv->commandv("loadfile", path.c_str(), action, nullptr);
          i++;
        }
      }
    } else {
      if (file.extension() == ".iso") {
        if ((double)std::filesystem::file_size(file) / 1000 / 1000 / 1000 > 4.7)
          openBluray(file);
        else
          openDvd(file);
        break;
      } else if (isSubtitleFile(file.string())) {
        mpv->commandv("sub-add", file.string().c_str(), append ? "auto" : "select", nullptr);
      } else {
        const char *action = append ? "append" : (i > 0 ? "append-play" : "replace");
        mpv->commandv("loadfile", file.string().c_str(), action, nullptr);
      }
      i++;
    }
  }
}

void Player::drawOpenURL() {
  if (!m_openURL) return;
  ImGui::OpenPopup("views.dialog.open_url.title"_i18n);

  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(wSize.x * 0.8f, scaled(50)), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("views.dialog.open_url.title"_i18n, &m_openURL,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_openURL = false;
    static char url[256] = {0};
    bool loadfile = false;
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Input URL", "views.dialog.open_url.hint"_i18n, url, IM_ARRAYSIZE(url),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
      if (url[0] != '\0') loadfile = true;
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(10));
    if (ImGui::Button("views.dialog.open_url.cancel"_i18n, ImVec2(scaled(5), 0))) m_openURL = false;
    ImGui::SameLine();
    if (url[0] == '\0') ImGui::BeginDisabled();
    if (ImGui::Button("views.dialog.open_url.ok"_i18n, ImVec2(scaled(5), 0))) loadfile = true;
    if (url[0] == '\0') ImGui::EndDisabled();
    if (loadfile) {
      m_openURL = false;
      mpv->commandv("loadfile", url, nullptr);
    }
    if (!m_openURL) url[0] = '\0';
    ImGui::EndPopup();
  }
}

void Player::drawDialog() {
  if (!m_dialog) return;
  ImGui::OpenPopup(m_dialog_title.c_str());

  ImGui::SetNextWindowSize(ImVec2(scaled(30), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal(m_dialog_title.c_str(), &m_dialog, ImGuiWindowFlags_NoMove)) {
    ImGui::TextWrapped("%s", m_dialog_msg.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(5));
    if (ImGui::Button("OK", ImVec2(scaled(5), 0))) m_dialog = false;
    ImGui::EndPopup();
  }
}

void Player::messageBox(std::string title, std::string msg) {
  m_dialog_title = title;
  m_dialog_msg = msg;
  m_dialog = true;
}

bool Player::isMediaFile(std::string file) {
  auto ext = std::filesystem::path(file).extension().string();
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(videoTypes.begin(), videoTypes.end(), ext) != videoTypes.end()) return true;
  if (std::find(audioTypes.begin(), audioTypes.end(), ext) != audioTypes.end()) return true;
  if (std::find(imageTypes.begin(), imageTypes.end(), ext) != imageTypes.end()) return true;
  return false;
}

bool Player::isSubtitleFile(std::string file) {
  auto ext = std::filesystem::path(file).extension().string();
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(subtitleTypes.begin(), subtitleTypes.end(), ext) != subtitleTypes.end()) return true;
  return false;
}
}  // namespace ImPlay