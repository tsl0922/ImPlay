// Copyright (c) 2022 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "helpers/dx11.h"

namespace D3D11 {

ID3D11Device* d3dDevice = nullptr;
ID3D11DeviceContext* d3dDeviceContext = nullptr;
IDXGISwapChain* d3dSwapChain = nullptr;
ID3D11RenderTargetView* d3dRenderTargetView = nullptr;

bool Init(HWND hWnd) {
  static const D3D_DRIVER_TYPE driverAttempts[] = {
      D3D_DRIVER_TYPE_HARDWARE,
      D3D_DRIVER_TYPE_WARP,
      D3D_DRIVER_TYPE_REFERENCE,
  };

  static const D3D_FEATURE_LEVEL levelAttempts[] = {
      D3D_FEATURE_LEVEL_11_1,  // Direct3D 11.1 SM 6
      D3D_FEATURE_LEVEL_11_0,  // Direct3D 11.0 SM 5
      D3D_FEATURE_LEVEL_10_1,  // Direct3D 10.1 SM 4
  };

  HRESULT hr = S_OK;
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;

  for (size_t i = 0; i < ARRAYSIZE(driverAttempts); i++) {
    hr = D3D11CreateDeviceAndSwapChain(nullptr, driverAttempts[i], nullptr, 0, levelAttempts, ARRAYSIZE(levelAttempts),
                                       D3D11_SDK_VERSION, &sd, &d3dSwapChain, &d3dDevice, nullptr, &d3dDeviceContext);
    if (SUCCEEDED(hr)) break;
  }

  if (FAILED(hr)) {
    Cleanup();
    return false;
  }
  return Resize(0, 0);
}

void Cleanup() {
  if (d3dRenderTargetView) d3dRenderTargetView->Release();
  if (d3dSwapChain) d3dSwapChain->Release();
  if (d3dDeviceContext) d3dDeviceContext->Release();
  if (d3dDevice) d3dDevice->Release();
}

bool Resize(int width, int height) {
  ID3D11Texture2D* backBuffer = nullptr;
  HRESULT hr = S_OK;
  if (d3dRenderTargetView) d3dRenderTargetView->Release();
  // Resize render target buffers
  if (width > 0 && height > 0) {
    hr = d3dSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr)) return false;
  }
  hr = d3dSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
  if (FAILED(hr)) return false;
  hr = d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &d3dRenderTargetView);
  backBuffer->Release();
  return SUCCEEDED(hr);
}

void RenderTarget() { d3dDeviceContext->OMSetRenderTargets(1, &d3dRenderTargetView, nullptr); }

void Present() { d3dSwapChain->Present(1, 0); }

void ClearColor(const float color[4]) {
  d3dDeviceContext->OMSetRenderTargets(1, &d3dRenderTargetView, nullptr);
  d3dDeviceContext->ClearRenderTargetView(d3dRenderTargetView, color);
}

}  // namespace D3D11