// Copyright (c) 2022-2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <map>
#include <string>
#include <vector>
#include <mutex>
#ifdef IMGUI_IMPL_OPENGL_ES3
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#include "mpv.h"
#include "config.h"
#include "views/view.h"
#include "views/about.h"
#include "views/debug.h"
#include "views/quickview.h"
#include "views/settings.h"
#include "views/context_menu.h"
#include "views/command_palette.h"
#include "helpers/imgui.h"
#include "helpers/nfd.h"
#include "helpers/utils.h"

#define PLAYER_NAME "ImPlay"

namespace ImPlay {
class Player {
 public:
  explicit Player(Config *config);
  ~Player();

 protected:
  bool init(std::map<std::string, std::string> &options);
  void shutdown();

  void initGui();
  void exitGui();
  void saveState();
  void restoreState();

  void loadFonts();
  void render();
  void renderVideo();

  void onCursorEvent(double x, double y);
  void onScrollEvent(double x, double y);
  void onKeyEvent(std::string name);
  void onKeyDownEvent(std::string name);
  void onKeyUpEvent(std::string name);
  void onDropEvent(int count, const char **paths);

  Config *config = nullptr;
  Mpv *mpv = nullptr;
  int width = 1280, height = 720;

 private:
  void updateWindowState();
  void initObservers();
  void writeMpvConf();

  void draw();
  void drawVideo();
  void execute(int n_args, const char **args_);

  void openFileDlg(NFD::Filters filters, bool append = false);
  void openFilesDlg(NFD::Filters filters, bool append = false);
  void openFolderDlg(bool append = false, bool disk = false);
  void openClipboard();
  void openURL();
  void openDvd(std::filesystem::path path);
  void openBluray(std::filesystem::path path);

  void playlistSort(bool reverse = false);

  void drawOpenURL();
  void drawDialog();
  void messageBox(std::string title, std::string msg);

  void load(std::vector<std::filesystem::path> files, bool append = false, bool disk = false);
  bool isMediaFile(std::string file);
  bool isSubtitleFile(std::string file);

  virtual int64_t GetWid() { return 0; }
  virtual GLAddrLoadFunc GetGLAddrFunc() = 0;
  virtual std::string GetClipboardString() = 0;
  virtual void GetMonitorSize(int *w, int *h) = 0;
  virtual int GetMonitorRefreshRate() = 0;
  virtual void GetFramebufferSize(int *w, int *h) = 0;
  virtual void MakeContextCurrent() = 0;
  virtual void DeleteContext() = 0;
  virtual void SwapBuffers() = 0;
  virtual void SetSwapInterval(int interval) = 0;
  virtual void BackendNewFrame() = 0;
  virtual void GetWindowScale(float *x, float *y) = 0;
  virtual void GetWindowPos(int *x, int *y) = 0;
  virtual void SetWindowPos(int x, int y) = 0;
  virtual void GetWindowSize(int *w, int *h) = 0;
  virtual void SetWindowSize(int w, int h) = 0;
  virtual void SetWindowTitle(std::string) = 0;
  virtual void SetWindowAspectRatio(int num, int den) = 0;
  virtual void SetWindowMaximized(bool m) = 0;
  virtual void SetWindowMinimized(bool m) = 0;
  virtual void SetWindowDecorated(bool d) = 0;
  virtual void SetWindowFloating(bool f) = 0;
  virtual void SetWindowFullscreen(bool fs) = 0;
  virtual void SetWindowShouldClose(bool c) = 0;

  bool idle = true;
  GLuint fbo = 0, tex = 0;
  ImTextureID logoTexture = 0;
  std::mutex contextLock;

  bool m_openURL = false;
  bool m_dialog = false;
  std::string m_dialog_title = "Dialog";
  std::string m_dialog_msg = "Message";

  Views::About *about;
  Views::Debug *debug;
  Views::Quickview *quickview;
  Views::Settings *settings;
  Views::ContextMenu *contextMenu;
  Views::CommandPalette *commandPalette;

  const std::vector<std::string> videoTypes = {
      "yuv", "y4m",   "m2ts", "m2t",   "mts",  "mtv",  "ts",   "tsv",    "tsa",  "tts",  "trp",  "mpeg", "mpg",
      "mpe", "mpeg2", "m1v",  "m2v",   "mp2v", "mpv",  "mpv2", "mod",    "vob",  "vro",  "evob", "evo",  "mpeg4",
      "m4v", "mp4",   "mp4v", "mpg4",  "h264", "avc",  "x264", "264",    "hevc", "h265", "x265", "265",  "ogv",
      "ogm", "ogx",   "mkv",  "mk3d",  "webm", "avi",  "vfw",  "divx",   "3iv",  "xvid", "nut",  "flic", "fli",
      "flc", "nsv",   "gxf",  "mxf",   "wm",   "wmv",  "asf",  "dvr-ms", "dvr",  "wtv",  "dv",   "hdv",  "flv",
      "f4v", "qt",    "mov",  "hdmov", "rm",   "rmvb", "3gpp", "3gp",    "3gp2", "3g2"};
  const std::vector<std::string> audioTypes = {
      "ac3", "a52",  "eac3", "mlp",  "dts", "dts-hd", "dtshd", "true-hd", "thd",  "truehd", "thd+ac3", "tta", "pcm",
      "wav", "aiff", "aif",  "aifc", "amr", "awb",    "au",    "snd",     "lpcm", "ape",    "wv",      "shn", "adts",
      "adt", "mpa",  "m1a",  "m2a",  "mp1", "mp2",    "mp3",   "m4a",     "aac",  "flac",   "oga",     "ogg", "opus",
      "spx", "mka",  "weba", "wma",  "f4a", "ra",     "ram",   "3ga",     "3ga2", "ay",     "gbs",     "gym", "hes",
      "kss", "nsf",  "nsfe", "sap",  "spc", "vgm",    "vgz",   "m3u",     "m3u8", "pls",    "cue"};
  const std::vector<std::string> imageTypes = {"jpg", "bmp", "png", "gif", "webp"};
  const std::vector<std::string> subtitleTypes = {"srt",  "ass", "idx", "sub", "sup",
                                                  "ttxt", "txt", "ssa", "smi", "mks"};

  const std::vector<std::pair<std::string, std::string>> mediaFilters = {
      {"Videos Files", fmt::format("{}", join(videoTypes, ","))},
      {"Audio Files", fmt::format("{}", join(audioTypes, ","))},
      {"Image Files", fmt::format("{}", join(imageTypes, ","))},
  };
  const std::vector<std::pair<std::string, std::string>> subtitleFilters = {
      {"Subtitle Files", fmt::format("{}", join(subtitleTypes, ","))},
  };
  const std::vector<std::pair<std::string, std::string>> isoFilters = {
      {"ISO Image Files", "iso"},
  };

  struct ContextGuard {
   public:
    inline ContextGuard(Player *p) : p(p) {
      p->contextLock.lock();
      p->MakeContextCurrent();
    }
    inline ~ContextGuard() {
      p->DeleteContext();
      p->contextLock.unlock();
    }

    // Disable copy from lvalue.
    ContextGuard(const ContextGuard &) = delete;
    ContextGuard &operator=(const ContextGuard &) = delete;

   private:
    Player *p;
  };
};
}  // namespace ImPlay