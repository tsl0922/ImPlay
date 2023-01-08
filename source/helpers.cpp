#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include "helpers.h"
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace ImPlay::Helpers {
std::string tolower(std::string s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

std::string toupper(std::string s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

const char* getDataDir() {
  static char dataDir[256] = {0};
#ifdef _WIN32
  wchar_t* dir = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &dir))) {
    wcstombs(dataDir, dir, 256);
    CoTaskMemFree(dir);
  }
#else
  char *home = getenv("HOME");
  char *xdg_dir = getenv("XDG_CONFIG_HOME");
  if (xdg_dir != nullptr) {
    strncpy(dataDir, xdg_dir, 256);
  } else if (home != nullptr) {
    strncpy(dataDir, home, 256);
    strncat(dataDir, "/.config", 9);
  }
#endif
  if (dataDir[0] != '\0') strncat(dataDir, "/implay", 8);
  return &dataDir[0];
}
}  // namespace ImPlay::Helpers