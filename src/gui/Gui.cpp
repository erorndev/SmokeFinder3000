#include "Gui.hpp"

#include <MinHook.h>
#include <dxgi1_2.h>

#include <string>

#include "../game/Schema.hpp"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace Gui {

static void CreateRenderTarget();
static void CleanupRenderTarget();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static HWND g_hWnd = nullptr;
static WNDPROC g_oWndProc = nullptr;
static bool g_Initialized = false;
static bool g_ShowMenu = true;

static bool g_RenderTargetDirty = true;
static UINT g_BufferWidth = 0;
static UINT g_BufferHeight = 0;

static CRITICAL_SECTION g_CS;
static bool g_CSInit = false;

struct ScopedLock {
  ScopedLock() { EnterCriticalSection(&g_CS); }
  ~ScopedLock() { LeaveCriticalSection(&g_CS); }
};

typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* Present1_t)(IDXGISwapChain1*, UINT, UINT,
                                    const DXGI_PRESENT_PARAMETERS*);
typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT,
                                         DXGI_FORMAT, UINT);

static Present_t oPresent = nullptr;
static Present1_t oPresent1 = nullptr;
static ResizeBuffers_t oResizeBuffers = nullptr;

static void AttachToSwapChain(IDXGISwapChain* pSwapChain) {
  if (g_Initialized) {
    CleanupRenderTarget();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_hWnd && g_oWndProc) {
      WNDPROC current =
          reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_hWnd, GWLP_WNDPROC));
      if (current == WndProc)
        SetWindowLongPtr(g_hWnd, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(g_oWndProc));
    }

    if (g_pd3dContext) {
      g_pd3dContext->Release();
      g_pd3dContext = nullptr;
    }
    if (g_pd3dDevice) {
      g_pd3dDevice->Release();
      g_pd3dDevice = nullptr;
    }

    g_Initialized = false;
  }

  g_pSwapChain = pSwapChain;

  if (FAILED(g_pSwapChain->GetDevice(
          __uuidof(ID3D11Device), reinterpret_cast<void**>(&g_pd3dDevice)))) {
    return;
  }
  g_pd3dDevice->GetImmediateContext(&g_pd3dContext);

  DXGI_SWAP_CHAIN_DESC sd{};
  if (FAILED(g_pSwapChain->GetDesc(&sd))) {
    g_pd3dContext->Release();
    g_pd3dContext = nullptr;
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
    return;
  }
  g_hWnd = sd.OutputWindow;

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  if (!ImGui_ImplWin32_Init(g_hWnd) ||
      !ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext)) {
    ImGui::DestroyContext();
    g_pd3dContext->Release();
    g_pd3dContext = nullptr;
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
    return;
  }

  g_RenderTargetDirty = true;
  g_BufferWidth = 0;
  g_BufferHeight = 0;

  g_oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
      g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
  g_Initialized = true;
}

static bool SwapChainNeedsAttach(IDXGISwapChain* pSwapChain) {
  if (!g_Initialized || pSwapChain != g_pSwapChain) return true;

  if (g_pd3dDevice && g_pd3dDevice->GetDeviceRemovedReason() != S_OK)
    return true;

  return false;
}

static void RenderOverlay(IDXGISwapChain* pSwapChain) {
  if (SwapChainNeedsAttach(pSwapChain)) AttachToSwapChain(pSwapChain);

  if (!g_Initialized) return;

  DXGI_SWAP_CHAIN_DESC sd{};
  if (FAILED(pSwapChain->GetDesc(&sd))) return;

  if (sd.BufferDesc.Width == 0 || sd.BufferDesc.Height == 0 || IsIconic(g_hWnd))
    return;

  if (g_RenderTargetDirty || sd.BufferDesc.Width != g_BufferWidth ||
      sd.BufferDesc.Height != g_BufferHeight) {
    CleanupRenderTarget();
    CreateRenderTarget();
    g_BufferWidth = sd.BufferDesc.Width;
    g_BufferHeight = sd.BufferDesc.Height;
    g_RenderTargetDirty = false;
  }

  if (!g_mainRenderTargetView) return;

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();

  RECT clientRect;
  if (GetClientRect(g_hWnd, &clientRect)) {
    float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
    float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(sd.BufferDesc.Width),
                            static_cast<float>(sd.BufferDesc.Height));

    if (clientWidth > 0 && clientHeight > 0 &&
        (clientWidth != sd.BufferDesc.Width ||
         clientHeight != sd.BufferDesc.Height) &&
        GetForegroundWindow() == g_hWnd) {
      POINT cursor;
      if (GetCursorPos(&cursor) && ScreenToClient(g_hWnd, &cursor)) {
        float scaledX = static_cast<float>(cursor.x) *
                        (static_cast<float>(sd.BufferDesc.Width) / clientWidth);
        float scaledY =
            static_cast<float>(cursor.y) *
            (static_cast<float>(sd.BufferDesc.Height) / clientHeight);
        io.AddMousePosEvent(scaledX, scaledY);
      }
    }
  }

  ImGui::NewFrame();

  if (g_ShowMenu) {
    ImGui::Begin("SmokeFinder3000", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);
    ImGui::Button("I am a button");
    ImGui::End();
  }

  ImGui::Render();
  g_pd3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

static void HandlePresentResult(HRESULT hr) {
  if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET ||
      hr == DXGI_ERROR_DEVICE_HUNG) {
    CleanupRenderTarget();
    g_RenderTargetDirty = true;
  }
}

HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval,
                         UINT Flags) {
  {
    ScopedLock lock;
    RenderOverlay(pSwapChain);
  }
  HRESULT hr = oPresent(pSwapChain, SyncInterval, Flags);
  {
    ScopedLock lock;
    HandlePresentResult(hr);
  }
  return hr;
}

HRESULT WINAPI hkPresent1(IDXGISwapChain1* pSwapChain, UINT SyncInterval,
                          UINT Flags,
                          const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
  {
    ScopedLock lock;
    RenderOverlay(pSwapChain);
  }
  HRESULT hr = oPresent1(pSwapChain, SyncInterval, Flags, pPresentParameters);
  {
    ScopedLock lock;
    HandlePresentResult(hr);
  }
  return hr;
}

HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
                               UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                               UINT SwapChainFlags) {
  {
    ScopedLock lock;
    if (pSwapChain == g_pSwapChain) {
      if (g_pd3dContext) {
        g_pd3dContext->OMSetRenderTargets(0, nullptr, nullptr);
        g_pd3dContext->Flush();
      }
      CleanupRenderTarget();
      g_RenderTargetDirty = true;
    }
  }
  return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat,
                        SwapChainFlags);
}

struct SwapChainVTableAddrs {
  uintptr_t present = 0;
  uintptr_t present1 = 0;
  uintptr_t resizeBuffers = 0;
};

static bool GetSwapChainVTableAddrs(SwapChainVTableAddrs& out) {
  DXGI_SWAP_CHAIN_DESC sd{};
  sd.BufferCount = 1;
  sd.BufferDesc.Width = 1;
  sd.BufferDesc.Height = 1;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = GetDesktopWindow();
  sd.SampleDesc.Count = 1;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  sd.Windowed = TRUE;

  static const D3D_FEATURE_LEVEL levels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3,
  };
  static const D3D_DRIVER_TYPE driverTypes[] = {D3D_DRIVER_TYPE_HARDWARE,
                                                D3D_DRIVER_TYPE_WARP};

  ID3D11Device* pDevice = nullptr;
  IDXGISwapChain* pSwap = nullptr;
  D3D_FEATURE_LEVEL obtained{};
  HRESULT hr = E_FAIL;

  for (D3D_DRIVER_TYPE driver : driverTypes) {
    hr = D3D11CreateDeviceAndSwapChain(
        nullptr, driver, nullptr, 0, levels, ARRAYSIZE(levels),
        D3D11_SDK_VERSION, &sd, &pSwap, &pDevice, &obtained, nullptr);
    if (SUCCEEDED(hr)) break;
  }

  if (FAILED(hr) || !pSwap) {
    return false;
  }

  void** vtable = *reinterpret_cast<void***>(pSwap);
  out.present = reinterpret_cast<uintptr_t>(vtable[8]);
  out.resizeBuffers = reinterpret_cast<uintptr_t>(vtable[13]);

  IDXGISwapChain1* pSwap1 = nullptr;
  if (SUCCEEDED(pSwap->QueryInterface(__uuidof(IDXGISwapChain1),
                                      reinterpret_cast<void**>(&pSwap1)))) {
    void** vtable1 = *reinterpret_cast<void***>(pSwap1);
    out.present1 = reinterpret_cast<uintptr_t>(vtable1[22]);
    pSwap1->Release();
  }

  pSwap->Release();
  pDevice->Release();
  return true;
}

static void CreateRenderTarget() {
  if (!g_pSwapChain || !g_pd3dDevice) return;

  ID3D11Texture2D* pBackBuffer = nullptr;
  HRESULT hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  if (FAILED(hr) || !pBackBuffer) {
    return;
  }

  hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                            &g_mainRenderTargetView);
  pBackBuffer->Release();

  if (FAILED(hr)) {
    g_mainRenderTargetView = nullptr;
  }
}

static void CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

void Initialize() {
  if (!g_CSInit) {
    InitializeCriticalSection(&g_CS);
    g_CSInit = true;
  }

  MH_STATUS mhInit = MH_Initialize();
  if (mhInit != MH_OK && mhInit != MH_ERROR_ALREADY_INITIALIZED) {
    return;
  }

  SwapChainVTableAddrs addrs;
  if (!GetSwapChainVTableAddrs(addrs)) return;

  if (addrs.present) {
    if (MH_CreateHook(reinterpret_cast<LPVOID>(addrs.present), &hkPresent,
                      reinterpret_cast<LPVOID*>(&oPresent)) == MH_OK)
      MH_EnableHook(reinterpret_cast<LPVOID>(addrs.present));
  }
  if (addrs.present1) {
    if (MH_CreateHook(reinterpret_cast<LPVOID>(addrs.present1), &hkPresent1,
                      reinterpret_cast<LPVOID*>(&oPresent1)) == MH_OK)
      MH_EnableHook(reinterpret_cast<LPVOID>(addrs.present1));
  }
  if (addrs.resizeBuffers) {
    if (MH_CreateHook(reinterpret_cast<LPVOID>(addrs.resizeBuffers),
                      &hkResizeBuffers,
                      reinterpret_cast<LPVOID*>(&oResizeBuffers)) == MH_OK)
      MH_EnableHook(reinterpret_cast<LPVOID>(addrs.resizeBuffers));
  }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (g_ShowMenu && g_Initialized &&
      ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  if ((msg == WM_DESTROY || msg == WM_NCDESTROY) && g_oWndProc) {
    SetWindowLongPtr(hWnd, GWLP_WNDPROC,
                     reinterpret_cast<LONG_PTR>(g_oWndProc));
  }

  return g_oWndProc ? CallWindowProc(g_oWndProc, hWnd, msg, wParam, lParam)
                    : DefWindowProc(hWnd, msg, wParam, lParam);
}

void Shutdown() {
  if (g_CSInit) EnterCriticalSection(&g_CS);

  if (g_Initialized) {
    CleanupRenderTarget();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_hWnd && g_oWndProc) {
      WNDPROC currentWndProc =
          reinterpret_cast<WNDPROC>(GetWindowLongPtr(g_hWnd, GWLP_WNDPROC));
      if (currentWndProc == WndProc)
        SetWindowLongPtr(g_hWnd, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(g_oWndProc));
    }

    if (g_pd3dContext) {
      g_pd3dContext->Release();
      g_pd3dContext = nullptr;
    }
    if (g_pd3dDevice) {
      g_pd3dDevice->Release();
      g_pd3dDevice = nullptr;
    }

    g_Initialized = false;
  }

  if (g_CSInit) LeaveCriticalSection(&g_CS);

  MH_DisableHook(MH_ALL_HOOKS);
  MH_Uninitialize();

  if (g_CSInit) {
    DeleteCriticalSection(&g_CS);
    g_CSInit = false;
  }
}

HWND GetWindow() { return g_hWnd; }
WNDPROC GetOriginalWndProc() { return g_oWndProc; }
void SetOriginalWndProc(WNDPROC proc) { g_oWndProc = proc; }
bool IsInitialized() { return g_Initialized; }
bool IsMenuOpen() { return g_ShowMenu; }
void ToggleMenu() { g_ShowMenu = !g_ShowMenu; }
}  // namespace Gui