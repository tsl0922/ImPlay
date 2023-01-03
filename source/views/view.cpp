#include "views/view.h"

namespace ImPlay::Views {
void View::openFile(std::vector<nfdu8filteritem_t> filters, std::function<void(nfdu8char_t*)> callback) {
  NFD::Init();
  nfdchar_t* outPath;
  if (NFD::OpenDialog(outPath, filters.data(), filters.size()) == NFD_OKAY) {
    callback(outPath);
    NFD::FreePath(outPath);
  }
  NFD::Quit();
}

void View::openFiles(std::vector<nfdu8filteritem_t> filters, std::function<void(nfdu8char_t*, int)> callback) {
  NFD::Init();
  const nfdpathset_t* outPaths;
  if (NFD::OpenDialogMultiple(outPaths, filters.data(), filters.size()) == NFD_OKAY) {
    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);
    for (auto i = 0; i < numPaths; i++) {
      nfdchar_t* path;
      NFD::PathSet::GetPath(outPaths, i, path);
      callback(path, i);
    }
    NFD::PathSet::Free(outPaths);
  }
  NFD::Quit();
}

void View::openFolder(std::function<void(nfdu8char_t*)> callback) {
  NFD::Init();
  nfdchar_t* outPath;
  if (NFD::PickFolder(outPath) == NFD_OKAY) {
    callback(outPath);
    NFD::FreePath(outPath);
  }
  NFD::Quit();
}
}  // namespace ImPlay::Views