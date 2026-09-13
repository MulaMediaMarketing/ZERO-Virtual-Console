#pragma once
#include "CaptureLibrary.h"
#include "GameRegistry.h"
#include "GameImportService.h"
#include "ResumeStore.h"
#include "RuntimeV4.h"
#include "Settings.h"
#include "Input.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <chrono>
#include <filesystem>

namespace zero {
class App {
public:
    App(HINSTANCE instance);
    int Run();
private:
    enum class Page { Home, Library, Store, Friends, Captures, Settings, GameDetail, Import };
    HINSTANCE instance_{};
    HWND hwnd_{};
    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> heading_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> body_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushText_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushMuted_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushAccent_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brushCard_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> cachedHero_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> cachedCapture_;
    std::filesystem::path cachedHeroPath_;
    std::filesystem::path cachedCapturePath_;

    GameRegistry registry_;
    GameImportService importer_;
    CaptureLibrary captures_;
    ResumeStore resumeStore_;
    RuntimeV4 runtime_;
    SettingsStore settingsStore_;
    UserSettings settings_;
    Input input_;
    Page page_{Page::Home};
    size_t selectedGame_{0};
    size_t selectedCapture_{0};
    size_t captureScroll_{0};
    size_t navIndex_{0};
    size_t overlayIndex_{0};
    bool overlayVisible_{false};
    bool preferResume_{true};
    bool captureViewerVisible_{false};
    bool captureDeleteConfirm_{false};
    std::chrono::steady_clock::time_point lastInput_{};
    std::wstring status_;

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT, WPARAM, LPARAM);
    bool InitWindow();
    bool InitGraphics();
    void CreateDeviceResources();
    void Paint();
    void DrawOverlay(float width, float height);
    void DrawRecentGames(float width, float height);
    void DrawEmptyState(const std::wstring& title, const std::wstring& body, float width);
    void DrawCaptures(float width, float height);
    void DrawCaptureViewer(float width, float height);
    void DrawCaptureDeleteConfirm(float width, float height);
    void Tick();
    void HandleInput(const InputSnapshot&);
    void HandleCaptureInput(const InputSnapshot&);
    void NavigateTo(Page page);
    void LaunchSelected(bool useResume);
    void ImportGameFolder();
    void EnterBorderlessFullscreen();
    void SetOverlayVisible(bool visible);
    void ClampCaptureSelection();
    Microsoft::WRL::ComPtr<ID2D1Bitmap> LoadBitmap(const std::filesystem::path& path);
    void DrawHeroArtwork(const GameManifest& game, const D2D1_RECT_F& bounds);
    void DrawTextLine(const std::wstring&, float x, float y, float w, float h, bool heading=false, ID2D1Brush* brush=nullptr);
    void DrawRoundedCard(const D2D1_RECT_F&, float radius, ID2D1Brush* fill);
    std::wstring Widen(const std::string&) const;
};
}
