#pragma once
#include <string>
#include <map>
#include <vector>
#include <imgui.h>
#include <imgui_internal.h>

namespace ImPlay::Helpers {
struct OptionParser {
  std::map<std::string, std::string> options;
  std::vector<std::string> paths;

  void parse(int argc, char** argv);
};

void marker(const char* desc);
bool loadTexture(const char* path, ImTextureID* out_texture, int* out_width, int* out_height);

std::string tolower(std::string s);
std::string toupper(std::string s);

int openUri(const char* uri);
const char* getDataDir(const char* subdir = "implay");
}  // namespace ImPlay::Helpers