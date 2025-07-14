// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <stdexcept>
#include <fmt/format.h>
#define NFD_THROWS_EXCEPTIONS
#include <nfd.hpp>
#include "helpers/nfd.h"

static bool checkError(nfdresult_t result) {
  if (result == NFD_ERROR) {
    const char* err = NFD::GetError();
    throw std::runtime_error(fmt::format("NFD Error: {}", err ? err : "unknown"));
  }
  return result == NFD_OKAY;
}

std::optional<std::filesystem::path> NFD::openFile(NFD::Filters filters) {
  NFD::Guard nfdGuard;
  NFD::UniquePath outPath;
  std::vector<nfdu8filteritem_t> items;

  for (auto& [n, s] : filters) items.emplace_back(nfdu8filteritem_t{n.c_str(), s.c_str()});
  if (!checkError(NFD::OpenDialog(outPath, items.data(), items.size()))) return std::nullopt;
  return std::make_optional(reinterpret_cast<char8_t*>(outPath.get()));
}

std::optional<std::vector<std::filesystem::path>> NFD::openFiles(NFD::Filters filters) {
  NFD::Guard nfdGuard;
  NFD::UniquePathSet outPaths;
  std::vector<nfdu8filteritem_t> items;

  for (auto& [n, s] : filters) items.emplace_back(nfdu8filteritem_t{n.c_str(), s.c_str()});
  if (!checkError(NFD::OpenDialogMultiple(outPaths, items.data(), items.size()))) return std::nullopt;

  std::vector<std::filesystem::path> paths;
  nfdpathsetsize_t numPaths;
  NFD::PathSet::Count(outPaths, numPaths);
  for (auto i = 0; i < numPaths; i++) {
    NFD::UniquePathSetPath path;
    NFD::PathSet::GetPath(outPaths, i, path);
    paths.emplace_back(reinterpret_cast<char8_t*>(path.get()));
  }
  return std::make_optional(paths);
}

std::optional<std::filesystem::path> NFD::openFolder() {
  NFD::Guard nfdGuard;
  NFD::UniquePath outPath;

  if (!checkError(NFD::PickFolder(outPath))) return std::nullopt;
  return std::make_optional(reinterpret_cast<char8_t*>(outPath.get()));
}
