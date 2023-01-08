#pragma once
#include <string>

namespace ImPlay::Helpers {
    std::string tolower(std::string s);
    std::string toupper(std::string s);

    const char* getDataDir();
}