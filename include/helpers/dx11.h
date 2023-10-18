#pragma once

#include <d3d11.h>

namespace D3D11 {
bool Init(HWND hWnd);
void Cleanup();
bool Resize(int width, int height);
void RenderTarget();
void Present();
void ClearColor(const float color[4]);

extern ID3D11Device* d3dDevice;
extern ID3D11DeviceContext* d3dDeviceContext;
extern IDXGISwapChain* d3dSwapChain;
}