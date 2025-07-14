// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace NFD {
using Filters = std::vector<std::pair<std::string, std::string>>;

std::optional<std::filesystem::path> openFile(Filters filters);
std::optional<std::vector<std::filesystem::path>> openFiles(Filters filters);
std::optional<std::filesystem::path> openFolder();
}  // namespace NFD