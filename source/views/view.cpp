// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "helpers.h"
#include "views/view.h"

namespace ImPlay::Views {
void View::openFile(std::vector<nfdu8filteritem_t> filters, std::function<void(nfdu8char_t*)> callback) {
  if (NFD::Init() != NFD_OKAY) {
    print("NFD Init Error: {}\n", NFD::GetError());
    return;
  }
  nfdchar_t* outPath;
  auto result = NFD::OpenDialog(outPath, filters.data(), filters.size());
  switch (result) {
    case NFD_OKAY:
      callback(outPath);
      NFD::FreePath(outPath);
      break;
    case NFD_ERROR:
      print("NFD Error: {}\n", NFD::GetError());
      break;
    case NFD_CANCEL:
    default:
      break;
  }
  NFD::Quit();
}

void View::openFiles(std::vector<nfdu8filteritem_t> filters, std::function<void(nfdu8char_t*, int)> callback) {
  if (NFD::Init() != NFD_OKAY) {
    print("NFD Init Error: {}\n", NFD::GetError());
    return;
  }
  const nfdpathset_t* outPaths;
  nfdpathsetsize_t numPaths;
  auto result = NFD::OpenDialogMultiple(outPaths, filters.data(), filters.size());
  switch (result) {
    case NFD_OKAY:
      NFD::PathSet::Count(outPaths, numPaths);
      for (auto i = 0; i < numPaths; i++) {
        nfdchar_t* path;
        NFD::PathSet::GetPath(outPaths, i, path);
        callback(path, i);
      }
      NFD::PathSet::Free(outPaths);
      break;
    case NFD_ERROR:
      print("NFD Error: {}\n", NFD::GetError());
      break;
    case NFD_CANCEL:
    default:
      break;
  }
  NFD::Quit();
}

void View::openFolder(std::function<void(nfdu8char_t*)> callback) {
  if (NFD::Init() != NFD_OKAY) {
    print("NFD Init Error: {}\n", NFD::GetError());
    return;
  }
  nfdchar_t* outPath;
  auto result = NFD::PickFolder(outPath);
  switch (result) {
    case NFD_OKAY:
      callback(outPath);
      NFD::FreePath(outPath);
      break;
    case NFD_ERROR:
      print("NFD Error: {}\n", NFD::GetError());
      break;
    case NFD_CANCEL:
    default:
      return;
  }
  NFD::Quit();
}
}  // namespace ImPlay::Views