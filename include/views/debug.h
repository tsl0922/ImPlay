#pragma once
#include <vector>
#include <imgui.h>
#include "view.h"
#include "mpv.h"

namespace ImPlay::Views {
class Debug : public View {
 public:
  explicit Debug(Mpv *mpv);
  ~Debug();

  void init();
  void show() override;
  void draw() override;

 private:
  struct Console {
    explicit Console(Mpv *mpv);
    ~Console();

    void init(const char *level);
    void draw();

    void ClearLog();
    void AddLog(const char *fmt, ...);
    void ExecCommand(const char *command_line);
    int TextEditCallback(ImGuiInputTextCallbackData *data);
    void initCommands();

    const std::vector<std::string> builtinCommands = {"HELP", "CLEAR", "HISTORY"};

    Mpv *mpv;
    char InputBuf[256];
    ImVector<char *> Items;
    ImVector<char *> Commands;
    ImVector<char *> History;
    int HistoryPos = -1;  // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter Filter;
    bool AutoScroll = true;
    bool ScrollToBottom = false;
    bool CommandInited = false;
    int LogLimit = 500;
  };

  void drawHeader();
  void drawConsole();
  void drawBindings();
  void drawCommands();
  void drawProperties(const char *title, const char *key);
  void drawPropNode(const char *name, mpv_node &node, int depth = 0);

  Mpv *mpv = nullptr;
  Console *console = nullptr;
  bool m_demo = false;
  bool m_metrics = false;
};
}  // namespace ImPlay::Views