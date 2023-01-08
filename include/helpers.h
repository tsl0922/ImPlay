#pragma once
#include <string>
#include <map>
#include <vector>

namespace ImPlay::Helpers {
struct OptionParser {
  std::map<std::string, std::string> options;
  std::vector<std::string> paths;

  void parse(int argc, char** argv);
};

std::string tolower(std::string s);
std::string toupper(std::string s);

const char* getDataDir();
}  // namespace ImPlay::Helpers