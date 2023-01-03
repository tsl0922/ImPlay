#pragma once
#include <map>
#include <utility>
#include <functional>
#include <GLFW/glfw3.h>
#include "../mpv.h"
#include "command_palette.h"

namespace ImPlay::Views {
class ContextMenu : public View {
 public:
  enum class Action {
    ABOUT,
    PALETTE,
  };

  ContextMenu(GLFWwindow *window, Mpv *mpv);

  void draw() override;
  void setAction(Action action, std::function<void()> callback) { actionHandlers[action] = std::move(callback); }

 private:
  enum class Theme { DARK, LIGHT, CLASSIC };

  void drawAudioDeviceList();
  void drawTracklist(const char *type, const char *prop);
  void drawChapterlist();
  void drawPlaylist();
  void drawProfilelist();

  void open();
  void openDisk();
  void openIso();
  void openSubtitles();
  void openClipboard();
  
  void playlistAddFiles();
  void playlistAddFolder();

  void openDvd(const char *path);
  void openBluray(const char *path);

  bool isMediaType(std::string ext);

  void action(Action action);
  void setTheme(Theme theme);

  GLFWwindow *window = nullptr;
  Mpv *mpv = nullptr;
  Theme theme;
  bool metrics = false;

  std::map<Action, std::function<void()>> actionHandlers;

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
  const std::vector<std::string> subtitleTypes = {"srt",  "ass", "idx", "sub", "sup",
                                                  "ttxt", "txt", "ssa", "smi", "mks"};
};
}  // namespace ImPlay::Views