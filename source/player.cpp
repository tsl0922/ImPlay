// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include <romfs/romfs.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#ifdef IMGUI_IMPL_DX11
#include "helpers/dx11.h"
#include <imgui_impl_dx11.h>
#else
#include <imgui_impl_opengl3.h>
#endif
#include <fonts/fontawesome.h>
#include <fonts/source_code_pro.h>
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
  mpv->option("osc", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");
  mpv->option("load-osd-console", "no");
  mpv->option("osd-playing-msg", "${media-title}");
  mpv->option("screenshot-directory", "~~desktop/");

  if (!config->Data.Mpv.UseConfig) {
    writeMpvConf();
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
    mpv->init(GetGLAddrFunc());
  }

  SetWindowDecorated(mpv->property<int, MPV_FORMAT_FLAG>("border"));
  mpv->property<int64_t, MPV_FORMAT_INT64>("volume", config->Data.Mpv.Volume);
  if (config->Data.Recent.SpaceToPlayLast) mpv->command("keybind SPACE 'script-message-to implay play-pause'");
  initObservers();

  return true;
}

void Player::draw() {
  drawLogo();

  about->draw();
  debug->draw();
  quickview->draw();
  settings->draw();
  contextMenu->draw();
  commandPalette->draw();

  drawOpenURL();
  drawDialog();
}

void Player::drawLogo() {
  if (logoTexture == nullptr || mpv->forceWindow || !idle) return;

  ImGuiViewport *vp = ImGui::GetMainViewport();
  const float width = std::min(vp->WorkSize.x, vp->WorkSize.y) * 0.1f;
  const ImVec2 delta(width, width);
  const ImVec2 center = vp->GetCenter();
  ImRect bb(center - delta, center + delta);

  ImGui::GetBackgroundDrawList(vp)->AddImage(logoTexture, bb.Min, bb.Max);
}

void Player::render() {
  {
    ContextGuard guard(this);

    if (idle) {
#ifdef IMGUI_IMPL_DX11
      int fbw = 0, fbh = 0;
      float color[4] = {0, 0, 0, 1};
      this->GetFramebufferSize(&fbw, &fbh);
      D3D11::Resize(fbw, fbh);
      D3D11::ClearColor(color);
#else
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
    }

    if (config->FontReload) {
      loadFonts();
#ifdef IMGUI_IMPL_DX11
#else
      ImGui_ImplOpenGL3_DestroyFontsTexture();
      ImGui_ImplOpenGL3_CreateFontsTexture();
#endif
      config->FontReload = false;
    }
#ifdef IMGUI_IMPL_DX11
    ImGui_ImplDX11_NewFrame();
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
  }

  BackendNewFrame();
  ImGui::NewFrame();

  if (!idle) {
    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImTextureID texture = reinterpret_cast<ImTextureID>(tex);
    ImGui::GetBackgroundDrawList(vp)->AddImage(texture, vp->Pos, vp->Pos + vp->Size);
  }

  draw();
  ImGui::Render();

  {
    ContextGuard guard(this);
#ifdef IMGUI_IMPL_DX11
    D3D11::RenderTarget();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    D3D11::Present();
#else
    GetFramebufferSize(&width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SwapBuffers();
#endif
    mpv->reportSwap();
  }

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();

    std::lock_guard<std::mutex> lock(contextLock);
    ImGui::RenderPlatformWindowsDefault();
    DeleteContext();
  }
}

void Player::renderVideo() {
  ContextGuard guard(this);
#ifdef IMGUI_IMPL_DX11
  int fbw = 0, fbh = 0;
  this->GetFramebufferSize(&fbw, &fbh);
  D3D11::Resize(fbw, fbh);
#else
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
  mpv->render(width, height, fbo, false);
}

void Player::initGui() {
  ContextGuard guard(this);

#ifdef IMGUI_IMPL_DX11
  if (!D3D11::Init((HWND)GetWid())) throw std::runtime_error("init dx11 failed");
#else
#if defined(IMGUI_IMPL_OPENGL_ES3)
  if (!gladLoadGLES2((GLADloadfunc)GetGLAddrFunc())) throw std::runtime_error("Failed to load GLES 2!");
#else
  if (!gladLoadGL((GLADloadfunc)GetGLAddrFunc())) throw std::runtime_error("Failed to load GL!");
#endif
  SetSwapInterval(1);
#endif

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  if (config->Data.Interface.Docking) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  if (config->Data.Interface.Viewports) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  loadFonts();
#ifdef IMGUI_IMPL_DX11
  ImGui_ImplDX11_Init(D3D11::d3dDevice, D3D11::d3dDeviceContext);
#else
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
#endif
}

void Player::exitGui() {
#ifdef IMGUI_IMPL_DX11
  D3D11::Cleanup();
#else
  MakeContextCurrent();
  ImGui_ImplOpenGL3_Shutdown();
  glDeleteTextures(1, &tex);
  glDeleteFramebuffers(1, &fbo);
#endif
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
  float fontSize = config->Data.Font.Size;
  float iconSize = fontSize - 2;
  float scale = config->Data.Interface.Scale;
  if (scale == 0) {
    float xscale, yscale;
    GetWindowScale(&xscale, &yscale);
    scale = std::max(xscale, yscale);
  }
  if (scale <= 0) scale = 1.0f;
  fontSize = std::floor(fontSize * scale);
  iconSize = std::floor(iconSize * scale);

  ImGuiStyle style;
  std::string theme = config->Data.Interface.Theme;

  {
    ImGui::SetTheme(theme.c_str(), &style);
    style.TabRounding = 4;
    style.ScrollbarRounding = 9;
    style.WindowRounding = 7;
    style.GrabRounding = 3;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ChildRounding = 4;
    style.WindowShadowSize = 50.0f;
    style.ScrollbarSize = 10.0f;
    style.Colors[ImGuiCol_WindowShadow] = ImVec4(0, 0, 0, 1.0f);
  }

  ImGuiIO &io = ImGui::GetIO();

#ifdef __APPLE__
  io.FontGlobalScale = 1.0f / scale;
#else
  style.ScaleAllSizes(scale);
#endif
  ImGui::GetStyle() = style;

  io.Fonts->Clear();

  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  ImWchar fa_range[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  const ImWchar *font_range = config->buildGlyphRanges();
  io.Fonts->AddFontFromMemoryCompressedTTF(fa_compressed_data, fa_compressed_size, iconSize, &cfg, fa_range);
  cfg.MergeMode = true;
  if (fileExists(config->Data.Font.Path))
    io.Fonts->AddFontFromFileTTF(config->Data.Font.Path.c_str(), 0, &cfg, font_range);
  else
    io.Fonts->AddFontFromMemoryCompressedTTF(unifont_compressed_data, unifont_compressed_size, 0, &cfg, font_range);
  io.Fonts->AddFontFromMemoryCompressedTTF(source_code_pro_compressed_data, source_code_pro_compressed_size, fontSize);

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

void Player::initObservers() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [this](void *data) { SetWindowShouldClose(true); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [this](void *data) {
    int width = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    int height = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (width > 0 && height > 0) {
      int x, y, w, h;
      GetWindowPos(&x, &y);
      GetWindowSize(&w, &h);

      SetWindowSize(width, height);
      SetWindowPos(x + (w - width) / 2, y + (h - height) / 2);
      if (mpv->keepaspect) SetWindowAspectRatio(width, height);
    }
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
      SetWindowTitle(GetWindowTitle());
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
    std::ofstream file(mpvConf);
    auto content = romfs::get("mpv/mpv.conf");
    file.write(reinterpret_cast<const char *>(content.data()), content.size()) << "\n";
    file << "# use opengl-hq video output for high-quality video rendering.\n";
    file << "profile=gpu-hq\n";
    file << "deband=no\n\n";
    file << "# Enable hardware decoding if available.\n";
    file << "hwdec=auto\n";
  }

  if (!std::filesystem::exists(inputConf)) {
    std::ofstream file(inputConf);
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

  auto scriptOpts = path / "script-opts";
  auto oscConf = scriptOpts / "osc.conf";

  std::filesystem::create_directories(scriptOpts);
  if (!std::filesystem::exists(oscConf)) {
    std::ofstream file(oscConf);
    file << "hidetimeout=2000\n";
    file << "idlescreen=no\n";
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
         if (n > 0) ImGui::SetTheme(args[0]);
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
  if (m_dialog) ImGui::OpenPopup(m_dialog_title.c_str());
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