#pragma once
#include <GLFW/glfw3.h>

namespace mpv {
namespace input {
void win_pos_cb(GLFWwindow* window, int x, int y);
void win_size_cb(GLFWwindow* window, int w, int h);
void cursor_pos_cb(GLFWwindow* window, double x, double y);
void mouse_button_cb(GLFWwindow* window, int button, int action, int mods);
void scroll_cb(GLFWwindow* window, double x, double y);
void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods);
void drop_cb(GLFWwindow* window, int count, const char* paths[]);
}  // namespace input
}  // namespace mpv