// BitWork.cpp : アプリケーションのエントリ ポイントを定義します。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Resource.h"
#include "Expression.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/backends/imgui_impl_win32.h"
#include "vendor/imgui/backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

using Microsoft::WRL::ComPtr;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
struct Graphics
{
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swap;
    ComPtr<ID3D11RenderTargetView> target;
    bool makeTarget()
    {
        ComPtr<ID3D11Texture2D> buffer;
        return SUCCEEDED(swap->GetBuffer(0, IID_PPV_ARGS(buffer.GetAddressOf()))) &&
            SUCCEEDED(device->CreateRenderTargetView(buffer.Get(), nullptr, target.ReleaseAndGetAddressOf()));
    }
};
struct App
{
    int width = 32;
    bool isSigned = false;
    bool topMost = false;
    std::uint32_t value = 0;
    std::array<char, 256> decimal{}, hex{};
    std::string error;
    void sync()
    {
        value &= bitwork::mask(width);
        const auto dec = isSigned ? bitwork::signedValue(value, width) : static_cast<std::int64_t>(value);
        const auto d = std::to_string(dec);
        std::copy(d.begin(), d.end(), decimal.begin()); decimal[d.size()] = 0;
        std::snprintf(hex.data(), hex.size(), "0x%0*X", width / 4, value);
        error.clear();
    }
    void input(const char* label, std::array<char, 256>& buffer, int base)
    {
        ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label); ImGui::SameLine(58 * ImGui::GetStyle().FontScaleDpi);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText(label[0] == 'D' ? "##decimal" : "##hex", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            try { value = bitwork::Expression(buffer.data(), base, width, isSigned).evaluate(); sync(); }
            catch (const std::exception& e) { error = e.what(); }
        }
    }
    void draw(HWND window)
    {
        const auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("BitWork", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        const float scale = ImGui::GetStyle().FontScaleDpi;
        ImGui::TextColored(ImVec4(.30f,.85f,.75f,1), "BITWORK");
        ImGui::SameLine();
        if (ImGui::Checkbox("TopMost", &topMost))
        {
            if (!SetWindowPos(window, topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
            {
                topMost = !topMost;
                error = "Could not change TopMost state.";
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("?")) ImGui::OpenPopup("Help");
        if (ImGui::BeginPopup("Help"))
        {
            ImGui::TextUnformatted("Click bits to toggle. Enter to apply an expression.");
            ImGui::TextUnformatted("Dec: 16*2   Hex: 0x80 & 0x01");
            ImGui::TextUnformatted("Operators: + - * / % & | ^ ~ << >> ( )");
            ImGui::TextUnformatted("Hex defaults to base 16. Prefixes: 0x / 0b.");
            ImGui::TextUnformatted("Each operation wraps to the selected width.");
            ImGui::TextUnformatted("Shift count: 0..width-1. Integers only.");
            ImGui::TextUnformatted("Signed: arithmetic right shift. Unsigned: logical.");
            ImGui::EndPopup();
        }
        ImGui::Separator();
        for (const int size : {8,16,32})
        {
            if (size != 8) ImGui::SameLine();
            const auto label = std::to_string(size) + " bit";
            if (ImGui::RadioButton(label.c_str(), width == size)) { width = size; sync(); }
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Signed", &isSigned)) sync();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Unchecked: unsigned / Checked: signed");
        ImGui::TextDisabled("MSB                                    LSB");
        const int columns = width == 8 ? 8 : 16;
        const float gap = 2 * scale;
        const float byteGap = 10 * scale;
        const int byteBoundaries = columns / 8 - 1;
        const float totalGap = (columns - 1) * gap + byteBoundaries * (byteGap - gap);
        const float cell = (ImGui::GetContentRegionAvail().x - totalGap) / columns;
        for (int row = 0; row < width / columns; ++row)
        {
            const int high = width - row * columns - 1;
            for (int col = 0; col < columns; ++col)
            {
                if (col) ImGui::SameLine(0, col % 8 == 0 ? byteGap : gap);
                const int bit = high - col;
                const bool on = (value & (1u << bit)) != 0;
                ImGui::PushID(bit);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1 * scale, 3 * scale));
                ImGui::PushStyleColor(ImGuiCol_Button, on ? ImVec4(.12f,.52f,.46f,1) : ImVec4(.14f,.18f,.24f,1));
                if (ImGui::Button(on ? "1" : "0", ImVec2(cell, 23 * scale))) { value ^= 1u << bit; sync(); }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bit %d / 0x%08X", bit, 1u << bit);
                ImGui::PopID();
            }
        }
        ImGui::Separator();
        input("Dec", decimal, 10); input("Hex", hex, 16);
        if (!error.empty()) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,.48f,.42f,1)); ImGui::TextWrapped("%s", error.c_str()); ImGui::PopStyleColor(); }
        ImGui::Spacing();
        const float buttonWidth = (ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemSpacing.x) / 3;
        if (ImGui::Button("~ Invert", ImVec2(buttonWidth, 28 * scale))) { value = ~value; sync(); }
        ImGui::SameLine();
        if (ImGui::Button("<< Left", ImVec2(buttonWidth, 28 * scale))) { value <<= 1; sync(); }
        ImGui::SameLine();
        if (ImGui::Button(">> Right", ImVec2(buttonWidth, 28 * scale)))
        {
            value = isSigned ? static_cast<std::uint32_t>(bitwork::signedValue(value, width) >> 1) : value >> 1; sync();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("%d-bit wraparound / %s right shift", width, isSigned ? "Arithmetic" : "Logical");
        ImGui::End();
    }
};
LRESULT CALLBACK wndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam)) return 1;
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    if (message == WM_GETMINMAXINFO)
    {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        const UINT dpi = GetDpiForWindow(window);
        RECT minimum{0, 0, MulDiv(320, dpi, 96), MulDiv(260, dpi, 96)};
        AdjustWindowRectExForDpi(&minimum, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
        info->ptMinTrackSize = {minimum.right - minimum.left, minimum.bottom - minimum.top};
        return 0;
    }
    if (message == WM_SYSCOMMAND && (wParam & 0xfff0) == SC_KEYMENU) return 0;
    return DefWindowProcW(window, message, wParam, lParam);
}
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = wndProc; wc.hInstance = instance; wc.lpszClassName = L"BitWorkWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    const UINT iconDpi = GetDpiForSystem();
    wc.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_BITWORK), IMAGE_ICON,
        GetSystemMetricsForDpi(SM_CXICON, iconDpi), GetSystemMetricsForDpi(SM_CYICON, iconDpi), LR_SHARED));
    wc.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_BITWORK), IMAGE_ICON,
        GetSystemMetricsForDpi(SM_CXSMICON, iconDpi), GetSystemMetricsForDpi(SM_CYSMICON, iconDpi), LR_SHARED));
    if (!RegisterClassExW(&wc)) return 1;
    const UINT initialDpi = GetDpiForSystem();
    RECT initial{0, 0, MulDiv(360, initialDpi, 96), MulDiv(280, initialDpi, 96)};
    AdjustWindowRectExForDpi(&initial, WS_OVERLAPPEDWINDOW, FALSE, 0, initialDpi);
    const HWND window = CreateWindowExW(0, wc.lpszClassName, L"BitWork", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, initial.right - initial.left, initial.bottom - initial.top, nullptr, nullptr, instance, nullptr);
    if (!window) { UnregisterClassW(wc.lpszClassName, instance); return 1; }
    Graphics gfx;
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2; desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.OutputWindow = window;
    desc.SampleDesc.Count = 1; desc.Windowed = TRUE; desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    auto create = [&](D3D_DRIVER_TYPE type)
    {
        return D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &desc, gfx.swap.ReleaseAndGetAddressOf(), gfx.device.ReleaseAndGetAddressOf(), nullptr, gfx.context.ReleaseAndGetAddressOf());
    };
    if ((FAILED(create(D3D_DRIVER_TYPE_HARDWARE)) && FAILED(create(D3D_DRIVER_TYPE_WARP))) || !gfx.makeTarget())
    {
        MessageBoxW(window, L"Direct3D 11 の初期化に失敗しました。", L"BitWork", MB_OK | MB_ICONERROR);
        DestroyWindow(window); UnregisterClassW(wc.lpszClassName, instance); return 1;
    }
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    auto& io = ImGui::GetIO(); io.IniFilename = nullptr; io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10, 8); style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 5); style.FrameRounding = 6; style.GrabRounding = 6;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(.07f,.09f,.13f,1);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(.12f,.15f,.21f,1);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(.18f,.62f,.54f,1);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(.30f,.85f,.75f,1);
    const float scale = ImGui_ImplWin32_GetDpiScaleForHwnd(window);
    style.ScaleAllSizes(scale); style.FontScaleDpi = scale;
    const bool platformReady = ImGui_ImplWin32_Init(window);
    const bool rendererReady = platformReady && ImGui_ImplDX11_Init(gfx.device.Get(), gfx.context.Get());
    if (!rendererReady)
    {
        MessageBoxW(window, L"UI の初期化に失敗しました。", L"BitWork", MB_OK | MB_ICONERROR);
        if (platformReady) ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(); DestroyWindow(window); UnregisterClassW(wc.lpszClassName, instance); return 1;
    }
    ShowWindow(window, show); UpdateWindow(window);
    App app; app.sync(); bool running = true; int exitCode = 0;
    while (running)
    {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (!running) break;
        if (IsIconic(window)) { WaitMessage(); continue; }
        RECT client{}; GetClientRect(window, &client);
        DXGI_SWAP_CHAIN_DESC current{}; gfx.swap->GetDesc(&current);
        if (current.BufferDesc.Width != static_cast<UINT>(client.right) || current.BufferDesc.Height != static_cast<UINT>(client.bottom))
        {
            gfx.context->OMSetRenderTargets(0, nullptr, nullptr); gfx.target.Reset();
            if (FAILED(gfx.swap->ResizeBuffers(0, client.right, client.bottom, DXGI_FORMAT_UNKNOWN, 0)) || !gfx.makeTarget()) { exitCode = 1; break; }
        }
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame(); app.draw(window); ImGui::Render();
        const float clear[] = {.07f,.09f,.13f,1}; auto* target = gfx.target.Get();
        gfx.context->OMSetRenderTargets(1, &target, nullptr); gfx.context->ClearRenderTargetView(target, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (FAILED(gfx.swap->Present(1, 0))) { exitCode = 1; break; }
    }
    if (exitCode) MessageBoxW(window, L"描画デバイスの処理に失敗しました。再起動してください。", L"BitWork", MB_OK | MB_ICONERROR);
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    if (IsWindow(window)) DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, instance);
    return exitCode;
}

