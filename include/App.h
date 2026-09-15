#pragma once
#include "CaptureLibrary.h"
#include "CapturesExperience.h"
#include "FriendsExperience.h"
#include "CoreShellExperience.h"
#include "StoreSettingsFirstBootExperience.h"
#include "GameRegistry.h"
#include "IdentityProvider.h"
#include "v5/ImportCoordinator.h"
#include "v5/ProductionRuntime.h"
#include "v5/ProductionShellIntegration.h"
#include "Settings.h"
#include "ShellUxState.h"
#include "StoreProvider.h"
#include "Input.h"
#include "ProductionUxContract.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace zero {
class App {
public:
    using Page = ProductionUxPage;
    enum class LaunchUxMode { Hidden, Starting, Failed, Ended };

    App(HINSTANCE instance);
    int Run();

private:
    class AuthoritativePageProjection final {
    public:
        explicit AuthoritativePageProjection(v5::ProductionShellIntegration& shell) noexcept : shell_(&shell) {}

        operator Page() const noexcept {
            if (!shell_) return Page::Home;
            const auto active = shell_->ActivePage();
            return active.value_or(Page::Home);
        }

        AuthoritativePageProjection& operator=(Page page) noexcept {
            if (shell_) {
                std::string ignored;
                shell_->Navigate(page, ignored);
            }
            return *this;
        }

    private:
        v5::ProductionShellIntegration* shell_{nullptr};
    };

    class AuthoritativeNavProjection final {
    public:
        AuthoritativeNavProjection(App& owner, v5::ProductionShellIntegration& shell) noexcept
            : owner_(&owner), shell_(&shell) {}

        operator size_t() const noexcept {
            return shell_ ? shell_->ActiveTopLevelIndex() : ProductionUxIndex(ProductionUxDestination::Home);
        }

        AuthoritativeNavProjection& operator=(size_t index) noexcept {
            if (!shell_ || index >= ProductionUxNavCount()) return *this;

            const auto beforeRevision = shell_->Snapshot().navigationRevision;
            std::string ignored;
            if (!shell_->Navigate(ProductionUxPageAt(index), ignored)) {
                const auto current = shell_->ActiveTopLevelIndex();
                if (index > current) shell_->MoveTopLevel(1, ignored);
                else if (index < current) shell_->MoveTopLevel(-1, ignored);
            }

            const auto afterRevision = shell_->Snapshot().navigationRevision;
            if (owner_ && afterRevision != beforeRevision) {
                owner_->shellUx_.BeginPageTransition();
                owner_->NotifyFocusMoved();
            }
            return *this;
        }

    private:
        App* owner_{nullptr};
        v5::ProductionShellIntegration* shell_{nullptr};
    };

    class AuthoritativeResumeProjection final {
    public:
        explicit AuthoritativeResumeProjection(ProductionRuntime& runtime) noexcept : runtime_(&runtime) {}
        std::optional<ResumeMetadata> Load(const std::string& packageId) const {
            return runtime_ ? runtime_->Resume(packageId) : std::nullopt;
        }
    private:
        ProductionRuntime* runtime_{nullptr};
    };

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
    v5::ImportCoordinator importer_;
    CaptureLibrary captures_;
    LocalIdentityProvider identity_;
    DisconnectedFriendsProvider friends_;
    DisconnectedStoreProvider store_;
    ProductionRuntime runtime_;
    AuthoritativeResumeProjection resumeStore_{runtime_};
    SettingsStore settingsStore_;
    UserSettings settings_;
    Input input_;
    ShellUxState shellUx_;
    v5::ProductionShellIntegration productionShell_;

    FriendsExperienceState friendsUx_{};
    CaptureExperienceState capturesUx_{};
    CoreShellExperienceState coreShellUx_{};
    StoreExperienceState storeUx_{};
    SettingsExperienceState settingsUx_{};

    AuthoritativePageProjection page_{productionShell_};
    LaunchUxMode launchUxMode_{LaunchUxMode::Hidden};
    RuntimeOutcome lastRuntimeOutcome_{RuntimeOutcome::None};
    size_t selectedAchievement_{0};
    size_t achievementScroll_{0};
    AuthoritativeNavProjection navIndex_{*this, productionShell_};
    size_t overlayIndex_{0};
    bool overlayVisible_{false};
    bool overlayClosing_{false};
    bool preferResume_{true};
    bool achievementsFromOverlay_{false};
    bool launchUsedResume_{false};
    bool settingsControllerConnected_{false};
    uint64_t settingsStorageBytes_{0};
    uint64_t settingsFreeBytes_{0};
    size_t settingsCrashReportCount_{0};
    unsigned settingsDisplayWidth_{0};
    unsigned settingsDisplayHeight_{0};
    std::string achievementPackageId_;
    std::string launchPackageId_;
    std::wstring launchTitle_;
    std::wstring launchError_;
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
    void DrawSettings(float width, float height);
    void DrawLaunchRecovery(float width, float height);
    void DrawCaptureViewer(float width, float height);
    void DrawCaptureDeleteConfirm(float width, float height);
    void DrawFocusRing(const D2D1_RECT_F& bounds, float radius, bool light = false);
    void Tick();
    void HandleInput(const InputSnapshot&);
    void HandleCaptureInput(const InputSnapshot&);
    void HandleAchievementInput(const InputSnapshot&);
    void HandleSettingsInput(const InputSnapshot&);
    void HandleLaunchRecoveryInput(const InputSnapshot&);
    void NavigateTo(Page page);
    void OpenAchievements(const std::string& packageId, Page returnPage, bool fromOverlay);
    void ClampAchievementSelection();
    void LaunchSelected(bool useResume);
    void RetryLaunch();
    void ReturnFromLaunchUx(bool toGameDetail);
    void UpdateLaunchUx();
    void RefreshSettingsTelemetry();
    void OpenSettingsLocation(bool diagnosticsOnly);
    void ImportGameFolder();
    void EnterBorderlessFullscreen();
    void RestoreShellForeground();
    void SetOverlayVisible(bool visible);
    bool ShellContentOwnsFocus() const noexcept;
    void NotifyFocusMoved();
    Microsoft::WRL::ComPtr<ID2D1Bitmap> LoadBitmap(const std::filesystem::path& path);
    void DrawHeroArtwork(const GameManifest& game, const D2D1_RECT_F& bounds);
    void DrawTextLine(const std::wstring&, float x, float y, float w, float h, bool heading=false, ID2D1Brush* brush=nullptr);
    void DrawRoundedCard(const D2D1_RECT_F&, float radius, ID2D1Brush* fill);
    std::wstring Widen(const std::string&) const;
};
}
