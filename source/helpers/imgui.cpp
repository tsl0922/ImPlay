// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#ifdef IMGUI_IMPL_DX11
#include "helpers/dx11.h"
#elif defined(IMGUI_IMPL_OPENGL_ES3)
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif
#include <romfs/romfs.hpp>
#include "helpers/utils.h"
#include "helpers/imgui.h"

bool ImGui::IsAnyKeyPressed() {
  for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
    if (ImGui::IsKeyPressed((ImGuiKey)key)) return true;
  return false;
}

void ImGui::HalignCenter(const char* text) {
  ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * 0.5f);
}

void ImGui::TextCentered(const char* text, bool disabled) {
  ImGui::HalignCenter(text);
  if (disabled)
    ImGui::TextDisabled("%s", text);
  else
    ImGui::Text("%s", text);
}

void ImGui::TextEllipsis(const char* text, float maxWidth) {
  if (maxWidth == 0) maxWidth = ImGui::GetContentRegionAvail().x;
  ImGuiStyle style = ImGui::GetStyle();
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImVec2 textSize = ImGui::CalcTextSize(text);
  ImVec2 min = ImGui::GetCursorScreenPos();
  ImVec2 max = min + ImVec2(maxWidth - style.FramePadding.x, textSize.y);
  ImRect textRect(min, max);
  ImGui::ItemSize(textRect);
  if (ImGui::ItemAdd(textRect, window->GetID(text)))
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), min, max, max.x, max.x, text, nullptr, &textSize);
}

void ImGui::Hyperlink(const char* label, const char* url) {
  auto style = ImGui::GetStyle();
  ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_CheckMark]);
  ImGui::Text("%s", label ? label : url);
  if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  if (ImGui::IsItemClicked()) ImPlay::openUrl(url);
  ImGui::PopStyleColor();
}

void ImGui::HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip()) {
    ImGui::PushTextWrapPos(35 * ImGui::GetFontSize());
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

ImTextureID ImGui::LoadTexture(const char* path, ImVec2* size) {
  int w, h;
  auto icon = romfs::get(path);
  auto data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(icon.data()), icon.size(), &w, &h, NULL, 4);
  if (data == nullptr) return nullptr;
#ifdef IMGUI_IMPL_DX11
  ID3D11ShaderResourceView* texture = nullptr;
  ID3D11Texture2D* texbuffer = nullptr;
  HRESULT hr = S_OK;

  D3D11_TEXTURE2D_DESC td;
  ZeroMemory(&td, sizeof(td));
  td.Width = w;
  td.Height = h;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_DEFAULT;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = data;
  initData.SysMemPitch = td.Width * 4;
  hr = D3D11::d3dDevice->CreateTexture2D(&td, &initData, &texbuffer);

  if (SUCCEEDED(hr)) {
    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    ZeroMemory(&srvd, sizeof(srvd));
    srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = td.MipLevels;
    hr = D3D11::d3dDevice->CreateShaderResourceView(texbuffer, &srvd, &texture);
    texbuffer->Release();
  }
#else
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  glBindTexture(GL_TEXTURE_2D, 0);
#endif
  stbi_image_free(data);

  if (size != nullptr) {
    size->x = w;
    size->y = h;
  }

  return reinterpret_cast<ImTextureID>(texture);
}