#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include "player.h"

#define TITLE "ImPlay"
#define WIDTH 1280
#define HEIGHT 720

namespace mpv {
class Window {
 public:
  Window();
  ~Window();

  void loop();
  void render();
  void redraw();

  Player *player;

 private:
  enum class Theme { DARK, LIGHT, CLASSIC };

  void openFile();
  void loadSub();
  void show();

  void draw();
  void drawContextMenu();

  void initPlayer();
  void initGLFW();
  void initImGui();

  void exitGLFW();
  void exitImGui();

  void setTheme(Theme theme);

  GLFWwindow *window = nullptr;
  Theme theme = Theme::DARK;
  bool paused = true;
  bool loaded = false;
  bool demo = false;
};
}  // namespace mpv