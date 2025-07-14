// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <vector>
#include <map>
#include <string>
#include <imgui.h>
#include "view.h"

namespace ImPlay::Views {
class Debug : public View {
 public:
  Debug(Config *config, Mpv *mpv);
  ~Debug();

  void init();
  void show() override;
  void draw() override;

 private:
  struct Console {
    explicit Console(Mpv *mpv);
    ~Console();

    void init(const char *level, int limit);
    void draw();

    void ClearLog();
    void AddLog(const char *level, const char *fmt, ...);
    void ExecCommand(const char *command_line);
    int TextEditCallback(ImGuiInputTextCallbackData *data);
    void initCommands(std::vector<std::pair<std::string, std::string>> &commands);
    ImFont *GetFont(const char *str);

    ImVec4 LogColor(const char *level);

    const std::vector<std::string> builtinCommands = {"HELP", "CLEAR", "HISTORY"};

    struct LogItem {
      char *Str;
      const char *Lev;
      int FontIdx;
    };

    Mpv *mpv;
    char InputBuf[256];
    ImVector<LogItem> Items;
    ImVector<char *> Commands;
    ImVector<char *> History;
    int HistoryPos = -1;  // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter Filter;
    bool AutoScroll = true;
    bool ScrollToBottom = false;
    bool CommandInited = false;
    std::string LogLevel = "status";
    int LogLimit = 500;
  };

  void drawHeader();
  void drawConsole();
  void drawBindings();
  void drawCommands();
  void drawProperties(const char *title, std::vector<std::string> &props);
  void drawPropNode(const char *name, mpv_node &node, int depth = 0);

  void initData();

  Console *console = nullptr;
  std::string version;
  std::string m_node = "Console";
  bool m_demo = false, m_metrics = false;

  std::vector<std::string> options;
  std::vector<std::string> properties;
  std::vector<std::pair<std::string, std::string>> commands;
};
}  // namespace ImPlay::Views