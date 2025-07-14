// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#pragma once
#include <imgui.h>

namespace ImGui {
bool IsAnyKeyPressed();
void HalignCenter(const char* text);
void TextCentered(const char* text, bool disabled = false);
void TextEllipsis(const char* text, float maxWidth = 0);
void Hyperlink(const char* label, const char* url);
void HelpMarker(const char* desc);
ImTextureID LoadTexture(const char* path, ImVec2* size = nullptr);
}  // namespace ImGui