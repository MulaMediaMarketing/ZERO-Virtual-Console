#pragma once
#include "GameRegistry.h"
#include "RuntimeSession.h"
#include "Settings.h"
#include "Input.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <chrono>

namespace zero {
class App {
public:
    App(HINSTANCE instance);
    int Run();
private:
    enum class Page { Home, Library, GameDetail, Settings };
    HINSTANCE instance_{};
    HWND hwnd_{};
    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> heading_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> body_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushText_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushMuted_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushAccent_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushCard_;

    GameRegistry registry_;
    RuntimeSession runtime_;
    SettingsStore settingsStore_;
    UserSettings settings_;
    Input input_;
    Page page_{Page::Home};
    size_t selectedGame_{0};
    size_t navIndex_{0};
    std::chrono::steady_clock::time_point lastInput_{};
    std::wstring status_;

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT, WPARAM, LPARAM);
    bool InitWindow();
    bool InitGraphics();
    void CreateDeviceResources();
    void Paint();
    void Tick();
    void HandleInput(const InputSnapshot&);
    void NavigateTo(Page page);
    void LaunchSelected();
    void DrawTextLine(const std::wstring&, float x, float y, float w, float h, bool heading=false, ID2D1Brush* brush=nullptr);
    void DrawRoundedCard(const D2D1_RECT_F&, float radius, ID2D1Brush* fill);
    std::wstring Widen(const std::string&) const;
};
}
