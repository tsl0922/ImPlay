#pragma once
#include <GLFW/glfw3.h>
#include "mpv.h"
#include <map>
#include <vector>
#include <sstream>
#include <iterator>
#include <functional>

namespace ImPlay {
class Player {
 public:
  Player(const char *title);
  ~Player();

  bool init(int argc, char *argv[]);
  void draw();
  void render(int w, int h);
  void pollEvent();

  void setCursor(double x, double y);
  void setMouse(int button, int action, int mods);
  void setScroll(double x, double y);
  void setKey(int key, int scancode, int action, int mods);
  void setDrop(int count, const char **paths);

 private:
  enum class Theme { DARK, LIGHT, CLASSIC };
  using CommandHandler = std::function<void(std::string)>;

  struct CommandMatch {
    std::string key;
    std::string command;
    std::string comment;
    int score;
  };

  struct CommandPalette {
    bool open = false;
    bool focusInput = false;
    bool justOpened = false;
    std::vector<char> buffer = std::vector<char>(1024, 0x00);
    std::vector<Mpv::BindingItem> bindinglist;
    std::vector<CommandMatch> matches;
    CommandHandler callback;

    void draw();
    void show(std::vector<Mpv::BindingItem> &bindinglist, CommandHandler callback);
    void match(const std::string &input);
  };

  void showAbout();
  void showCommandPalette();

  void drawTracklistMenu(const char *type, const char *prop);
  void drawChapterlistMenu();
  void drawPlaylistMenu();
  void drawContextMenu();

  void openFile();
  void loadSub();
  void initMpv();
  void setTheme(Theme theme);

  GLFWwindow *window = nullptr;
  Mpv *mpv = nullptr;
  Mpv::OptionParser optionParser;
  CommandPalette commandPalette;
  const char *title;
  Theme theme;
  bool about = false;
  bool demo = false;

  std::vector<Mpv::TrackItem> tracklist;
  std::vector<Mpv::PlayItem> playlist;
  std::vector<Mpv::ChapterItem> chapterlist;
  std::vector<Mpv::BindingItem> bindinglist;
  std::vector<std::string> profilelist;
  bool paused = false;
  bool loaded = false;

  void translateMod(std::vector<std::string> &keys, int mods) {
    if (mods & GLFW_MOD_CONTROL) keys.push_back("Ctrl");
    if (mods & GLFW_MOD_ALT) keys.push_back("Alt");
    if (mods & GLFW_MOD_SHIFT) keys.push_back("Shift");
    if (mods & GLFW_MOD_SUPER) keys.push_back("Meta");
  }

  const std::map<int, std::string> keyMappings = {
      {GLFW_KEY_SPACE, "SPACE"},
      {GLFW_KEY_APOSTROPHE, "'"},
      {GLFW_KEY_COMMA, ","},
      {GLFW_KEY_MINUS, "-"},
      {GLFW_KEY_PERIOD, "."},
      {GLFW_KEY_SLASH, "/"},
      {GLFW_KEY_0, "0"},
      {GLFW_KEY_1, "1"},
      {GLFW_KEY_2, "2"},
      {GLFW_KEY_3, "3"},
      {GLFW_KEY_4, "4"},
      {GLFW_KEY_5, "5"},
      {GLFW_KEY_6, "6"},
      {GLFW_KEY_7, "7"},
      {GLFW_KEY_8, "8"},
      {GLFW_KEY_9, "9"},
      {GLFW_KEY_SEMICOLON, ";"},
      {GLFW_KEY_EQUAL, "="},
      {GLFW_KEY_A, "a"},
      {GLFW_KEY_B, "b"},
      {GLFW_KEY_C, "c"},
      {GLFW_KEY_D, "d"},
      {GLFW_KEY_E, "e"},
      {GLFW_KEY_F, "f"},
      {GLFW_KEY_G, "g"},
      {GLFW_KEY_H, "h"},
      {GLFW_KEY_I, "i"},
      {GLFW_KEY_J, "j"},
      {GLFW_KEY_K, "k"},
      {GLFW_KEY_L, "l"},
      {GLFW_KEY_M, "m"},
      {GLFW_KEY_N, "n"},
      {GLFW_KEY_O, "o"},
      {GLFW_KEY_P, "p"},
      {GLFW_KEY_Q, "q"},
      {GLFW_KEY_R, "r"},
      {GLFW_KEY_S, "s"},
      {GLFW_KEY_T, "t"},
      {GLFW_KEY_U, "u"},
      {GLFW_KEY_V, "v"},
      {GLFW_KEY_W, "w"},
      {GLFW_KEY_X, "x"},
      {GLFW_KEY_Y, "y"},
      {GLFW_KEY_Z, "z"},
      {GLFW_KEY_LEFT_BRACKET, "["},
      {GLFW_KEY_BACKSLASH, "\\"},
      {GLFW_KEY_RIGHT_BRACKET, "]"},
      {GLFW_KEY_GRAVE_ACCENT, "`"},

      {GLFW_KEY_ESCAPE, "ESC"},
      {GLFW_KEY_ENTER, "ENTER"},
      {GLFW_KEY_TAB, "TAB"},
      {GLFW_KEY_BACKSPACE, "BS"},
      {GLFW_KEY_INSERT, "INS"},
      {GLFW_KEY_DELETE, "DEL"},
      {GLFW_KEY_RIGHT, "RIGHT"},
      {GLFW_KEY_LEFT, "LEFT"},
      {GLFW_KEY_DOWN, "DOWN"},
      {GLFW_KEY_UP, "UP"},
      {GLFW_KEY_PAGE_UP, "PGUP"},
      {GLFW_KEY_PAGE_DOWN, "PGDWN"},
      {GLFW_KEY_HOME, "HOME"},
      {GLFW_KEY_END, "END"},
      {GLFW_KEY_PRINT_SCREEN, "PRINT"},
      {GLFW_KEY_PAUSE, "PAUSE"},
      {GLFW_KEY_F1, "F1"},
      {GLFW_KEY_F2, "F2"},
      {GLFW_KEY_F3, "F3"},
      {GLFW_KEY_F4, "F4"},
      {GLFW_KEY_F5, "F5"},
      {GLFW_KEY_F6, "F6"},
      {GLFW_KEY_F7, "F7"},
      {GLFW_KEY_F8, "F8"},
      {GLFW_KEY_F9, "F9"},
      {GLFW_KEY_F10, "F10"},
      {GLFW_KEY_F11, "F11"},
      {GLFW_KEY_F12, "F12"},
      {GLFW_KEY_F13, "F13"},
      {GLFW_KEY_F14, "F14"},
      {GLFW_KEY_F15, "F15"},
      {GLFW_KEY_F16, "F16"},
      {GLFW_KEY_F17, "F17"},
      {GLFW_KEY_F18, "F18"},
      {GLFW_KEY_F19, "F19"},
      {GLFW_KEY_F20, "F20"},
      {GLFW_KEY_F21, "F21"},
      {GLFW_KEY_F22, "F22"},
      {GLFW_KEY_F23, "F23"},
      {GLFW_KEY_F24, "F24"},
      {GLFW_KEY_KP_0, "KP0"},
      {GLFW_KEY_KP_1, "KP1"},
      {GLFW_KEY_KP_2, "KP2"},
      {GLFW_KEY_KP_3, "KP3"},
      {GLFW_KEY_KP_4, "KP4"},
      {GLFW_KEY_KP_5, "KP5"},
      {GLFW_KEY_KP_6, "KP6"},
      {GLFW_KEY_KP_7, "KP7"},
      {GLFW_KEY_KP_8, "KP8"},
      {GLFW_KEY_KP_9, "KP9"},
      {GLFW_KEY_KP_ENTER, "KP_ENTER"},
  };

  const std::map<int, std::string> shiftMappings = {
      {GLFW_KEY_0, ")"},
      {GLFW_KEY_1, "!"},
      {GLFW_KEY_2, "@"},
      {GLFW_KEY_3, "#"},
      {GLFW_KEY_4, "$"},
      {GLFW_KEY_5, "%"},
      {GLFW_KEY_6, "^"},
      {GLFW_KEY_7, "&"},
      {GLFW_KEY_8, "*"},
      {GLFW_KEY_9, "("},
      {GLFW_KEY_MINUS, "_"},
      {GLFW_KEY_EQUAL, "+"},
      {GLFW_KEY_LEFT_BRACKET, "{"},
      {GLFW_KEY_RIGHT_BRACKET, "}"},
      {GLFW_KEY_BACKSLASH, "|"},
      {GLFW_KEY_SEMICOLON, ":"},
      {GLFW_KEY_APOSTROPHE, "\""},
      {GLFW_KEY_COMMA, "<"},
      {GLFW_KEY_PERIOD, ">"},
      {GLFW_KEY_SLASH, "?"},
  };

  const std::map<int, std::string> actionMappings = {
      {GLFW_PRESS, "keydown"},
      {GLFW_RELEASE, "keyup"},
  };

  const std::map<int, std::string> mbtnMappings = {
      {GLFW_MOUSE_BUTTON_LEFT, "MBTN_LEFT"},    {GLFW_MOUSE_BUTTON_MIDDLE, "MBTN_MID"},
      {GLFW_MOUSE_BUTTON_RIGHT, "MBTN_RIGHT"},  {GLFW_MOUSE_BUTTON_4, "MP_MBTN_BACK"},
      {GLFW_MOUSE_BUTTON_5, "MP_MBTN_FORWARD"},
  };
};
}  // namespace ImPlay