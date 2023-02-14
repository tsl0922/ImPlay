// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <vector>
#include <imgui.h>

namespace ImGui {
std::vector<const char*> Themes();
void SetTheme(const char* theme, ImGuiStyle* dst = nullptr);
void StyleColorsSpectrum(ImGuiStyle* dst = nullptr);
void StyleColorsDracula(ImGuiStyle* dst = nullptr);
void StyleColorsDeepDark(ImGuiStyle* dst = nullptr);
}  // namespace ImGui