// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <vector>
#include <imgui.h>

namespace ImGui {
void SetTheme(const char* theme);
std::vector<const char*> Themes();
void StyleColorsSpectrum(ImGuiStyle* dst);
void StyleColorsDracula(ImGuiStyle* dst);
void StyleColorsDeepDark(ImGuiStyle* dst);
}  // namespace ImGui