#include "App.h"
#include "FirstBootService.h"
#include "FirstBootWizard.h"
#include "PlatformPaths.h"
#include "Settings.h"
#include <windows.h>
#include <chrono>
#include <thread>

namespace {
WNDPROC gZeroOriginalWindowProc = nullptr;

LRESULT CALLBACK ZeroProductionWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_GETMINMAXINFO) {
        auto* info = reinterpret_cast<MINMAXINFO*>(lp);
        if (info) {
            info->ptMinTrackSize.x = 1280;
            info->ptMinTrackSize.y = 720;
        }
        return 0;
    }

    // Keep F11 useful without dropping back into an unclosable WS_POPUP shell.
    if (msg == WM_KEYDOWN && wp == VK_F11) {
        ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
        return 0;
    }

    return gZeroOriginalWindowProc
        ? CallWindowProcW(gZeroOriginalWindowProc, hwnd, msg, wp, lp)
        : DefWindowProcW(hwnd, msg, wp, lp);
}

void InstallProductionWindowChromeAsync() {
    std::thread([] {
        HWND hwnd = nullptr;
        for (int attempt = 0; attempt < 400 && !hwnd; ++attempt) {
            hwnd = FindWindowW(L"ZeroVirtualConsoleWindow", nullptr);
            if (!hwnd) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!hwnd) return;

        // App::Run initially enters the cinematic borderless shell. Once that startup
        // transition has completed, restore normal Windows chrome so users always have
        // visible Minimize / Maximize / Close controls and standard Alt+F4 behavior.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        style &= ~static_cast<LONG_PTR>(WS_POPUP);
        style |= WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);

        gZeroOriginalWindowProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ZeroProductionWindowProc)));

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        ShowWindow(hwnd, SW_MAXIMIZE);
    }).detach();
}

bool applyFirstBootSettings(const std::filesystem::path& root, const zero::FirstBootState& state) {
    zero::SettingsStore settingsStore(root);
    auto settings = settingsStore.Load();
    settings.profileName = state.profileName.empty() ? "Player" : state.profileName;
    settings.volume = static_cast<int>(state.volume > 100 ? 100 : state.volume);
    return settingsStore.Save(settings);
}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // ZERO owns its physical-pixel layout and must not be virtualized by Windows when
    // moving between displays or running at 125/150/200% scale.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const auto root = zero::PlatformPaths::DataRoot();
    zero::FirstBootService firstBoot(root);
    if (firstBoot.IsRequired()) {
        zero::FirstBootWizard wizard(hInstance, firstBoot);
        if (!wizard.Run()) return 0;
        if (!applyFirstBootSettings(root, firstBoot.Load())) {
            MessageBoxW(nullptr,
                L"ZERO completed setup but could not persist the profile and volume settings. Setup will remain saved; restart ZERO after checking local storage access.",
                L"ZERO Setup", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    InstallProductionWindowChromeAsync();
    zero::App app(hInstance);
    return app.Run();
}
