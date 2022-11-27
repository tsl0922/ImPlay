#include <imgui.h>
#include <imgui_internal.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <iostream>

#include "input.h"
#include "window.h"

// https://www.glfw.org/docs/latest/input_guide.html
// https://mpv.io/manual/stable/#input-conf-syntax
namespace mpv {
namespace input {
static const std::map<int, std::string> key_names = {
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

static const std::map<int, std::string> shift_names = {
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

static const std::map<int, std::string> action_names = {
    {GLFW_PRESS, "keydown"},
    {GLFW_RELEASE, "keyup"},
};

static const std::map<int, std::string> mbtn_names = {
    {GLFW_MOUSE_BUTTON_LEFT, "MBTN_LEFT"},
    {GLFW_MOUSE_BUTTON_MIDDLE, "MBTN_MID"},
    {GLFW_MOUSE_BUTTON_RIGHT, "MBTN_RIGHT"},
    {GLFW_MOUSE_BUTTON_4, "MP_MBTN_BACK"},
    {GLFW_MOUSE_BUTTON_5, "MP_MBTN_FORWARD"},
};

template <typename Range, typename Value = typename Range::value_type>
std::string join(Range const& elements, const char* const delimiter) {
  std::ostringstream os;
  auto b = begin(elements), e = end(elements);
  if (b != e) {
    std::copy(b, prev(e), std::ostream_iterator<Value>(os, delimiter));
    b = prev(e);
  }
  if (b != e) os << *b;
  return os.str();
}

static void translate_mod(std::vector<std::string>& keys, int mods) {
  if (mods & GLFW_MOD_CONTROL) keys.push_back("Ctrl");
  if (mods & GLFW_MOD_ALT) keys.push_back("Alt");
  if (mods & GLFW_MOD_SHIFT) keys.push_back("Shift");
  if (mods & GLFW_MOD_SUPER) keys.push_back("Meta");
}

void win_pos_cb(GLFWwindow* window, int x, int y) {
  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  win->redraw();
}

void win_size_cb(GLFWwindow* window, int w, int h) {
  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  win->redraw();
}

void cursor_pos_cb(GLFWwindow* window, double x, double y) {
  if (ImGui::GetIO().WantCaptureMouse) return;

  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  const char* args[] = {"mouse", xs.c_str(), ys.c_str(), NULL};
  win->player->command(args);
}

void mouse_button_cb(GLFWwindow* window, int button, int action, int mods) {
  if (ImGui::GetIO().WantCaptureMouse) return;

  auto s = action_names.find(action);
  if (s == action_names.end()) return;
  const std::string cmd = s->second;

  std::vector<std::string> keys;
  translate_mod(keys, mods);
  s = mbtn_names.find(button);
  if (s == action_names.end()) return;
  keys.push_back(s->second);
  const std::string arg = join(keys, "+");

  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  const char* args[] = {cmd.c_str(), arg.c_str(), NULL};
  win->player->command(args);
}

void scroll_cb(GLFWwindow* window, double x, double y) {
  if (ImGui::GetIO().WantCaptureMouse) return;

  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (abs(x) > 0) {
    win->player->command(x > 0 ? "keypress WHEEL_LEFT" : "keypress WHEEL_RIGH");
    win->player->command(x > 0 ? "keyup WHEEL_LEFT" : "keyup WHEEL_RIGH");
  }
  if (abs(y) > 0) {
    win->player->command(y > 0 ? "keypress WHEEL_UP" : "keypress WHEEL_DOWN");
    win->player->command(y > 0 ? "keyup WHEEL_UP" : "keyup WHEEL_DOWN");
  }
}

void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (ImGui::GetIO().WantCaptureKeyboard) return;

  auto s = action_names.find(action);
  if (s == action_names.end()) return;
  const std::string cmd = s->second;

  std::string name;
  if (mods & GLFW_MOD_SHIFT) {
    s = shift_names.find(key);
    if (s != shift_names.end()) {
      name = s->second;
      mods &= ~GLFW_MOD_SHIFT;
    }
  }
  if (name.empty()) {
    s = key_names.find(key);
    if (s == key_names.end()) return;
    name = s->second;
  }

  std::vector<std::string> keys;
  translate_mod(keys, mods);
  keys.push_back(name);
  const std::string arg = join(keys, "+");

  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  const char* args[] = {cmd.c_str(), arg.c_str(), NULL};
  win->player->command(args);
}

void drop_cb(GLFWwindow* window, int count, const char* paths[]) {
  if (ImGui::GetIO().WantCaptureMouse) return;

  auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
  for (int i = 0; i < count; i++) {
    const char* cmd[] = {"loadfile", paths[i],
                         i > 0 ? "append-play" : "replace", NULL};
    win->player->command(cmd);
  }
}
}  // namespace input
}  // namespace mpv