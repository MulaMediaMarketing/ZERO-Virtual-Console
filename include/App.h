#pragma once
#include "CaptureLibrary.h"
#include "FriendsProvider.h"
#include "GameRegistry.h"
#include "GameImportService.h"
#include "IdentityProvider.h"
#include "ResumeStore.h"
#include "RuntimeV4.h"
#include "Settings.h"
#include "ShellUxState.h"
#include "StoreProvider.h"
#include "Input.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <chrono>
#include <filesystem>
#include <string>

namespace zero {
class App {
public:
    enum class Page { Home, Library, Store, Friends, Captures, Settings, GameDetail, Import, Achievements };

    App(HINSTANCE instance);
    int Run();

private:
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
    LocalIdentityProvider identity_;
    DisconnectedFriendsProvider friends_;
    DisconnectedStoreProvider store_;
    ResumeStore resumeStore_;
    RuntimeV4 runtime_;
    SettingsStore settingsStore_;
    UserSettings settings_;
    Input input_;
    ShellUxState shellUx_;
    Page page_{Page::Home};
    Page achievementsReturnPage_{Page::Home};
    size_t selectedGame_{0};
    size_t selectedCapture_{0};
    size_t captureScroll_{0};
    size_t selectedAchievement_{0};
    size_t achievementScroll_{0};
    size_t navIndex_{0};
    size_t overlayIndex_{0};
    bool overlayVisible_{false};
    bool preferResume_{true};
    bool captureViewerVisible_{false};
    bool captureDeleteConfirm_{false};
    bool achievementsFromOverlay_{false};
    std::string achievementPackageId_;
    std::chrono::steady_clock::time_point lastInput_{};
    std::chrono::steady_clock::time_point lastTick_{};
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
    void DrawStore(float width, float height);
    void DrawFriends(float width, float height);
    void DrawCaptures(float width, float height);
    void DrawAchievements(float width, float height);
    void DrawCaptureViewer(float width, float height);
    void DrawCaptureDeleteConfirm(float width, float height);
    void DrawFocusRing(const D2D1_RECT_F& bounds, float radius, bool light = false);
    void Tick();
    void HandleInput(const InputSnapshot&);
    void HandleCaptureInput(const InputSnapshot&);
    void HandleAchievementInput(const InputSnapshot&);
    void NavigateTo(Page page);
    void OpenAchievements(const std::string& packageId, Page returnPage, bool fromOverlay);
    void ClampAchievementSelection();
    void LaunchSelected(bool useResume);
    void ImportGameFolder();
    void EnterBorderlessFullscreen();
    void SetOverlayVisible(bool visible);
    void ClampCaptureSelection();
    void NotifyFocusMoved();
    Microsoft::WRL::ComPtr<ID2D1Bitmap> LoadBitmap(const std::filesystem::path& path);
    void DrawHeroArtwork(const GameManifest& game, const D2D1_RECT_F& bounds);
    void DrawTextLine(const std::wstring&, float x, float y, float w, float h, bool heading=false, ID2D1Brush* brush=nullptr);
    void DrawRoundedCard(const D2D1_RECT_F&, float radius, ID2D1Brush* fill);
    std::wstring Widen(const std::string&) const;
};
}
