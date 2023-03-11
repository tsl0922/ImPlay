// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <GLFW/glfw3.h>
#include "views/view.h"
#include "views/about.h"
#include "views/debug.h"
#include "views/quickview.h"
#include "views/settings.h"
#include "views/context_menu.h"
#include "views/command_palette.h"
#include "helpers/nfd.h"

namespace ImPlay {
class Command : public Views::View {
 public:
  Command(Config *config, Mpv *mpv, GLFWwindow *window);
  ~Command() override;

  void init();
  void draw() override;
  void execute(int n_args, const char **args_);

  void load(std::vector<std::filesystem::path> files, bool append = false, bool disk = false);
  bool isMediaFile(std::string file);
  bool isSubtitleFile(std::string file);

 private:
  void openFileDlg(NFD::Filters filters, bool append = false);
  void openFilesDlg(NFD::Filters filters, bool append = false);
  void openFolderDlg(bool append = false, bool disk = false);

  void openClipboard();
  void openURL();

  void openDvd(std::filesystem::path path);
  void openBluray(std::filesystem::path path);

  void drawOpenURL();
  void drawDialog();
  void messageBox(std::string title, std::string msg);

  GLFWwindow *window = nullptr;
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
      {"Videos Files", format("{}", join(videoTypes, ","))},
      {"Audio Files", format("{}", join(audioTypes, ","))},
      {"Image Files", format("{}", join(imageTypes, ","))},
  };
  const std::vector<std::pair<std::string, std::string>> subtitleFilters = {
      {"Subtitle Files", format("{}", join(subtitleTypes, ","))},
  };
  const std::vector<std::pair<std::string, std::string>> isoFilters = {
      {"ISO Image Files", "iso"},
  };
};
}  // namespace ImPlay