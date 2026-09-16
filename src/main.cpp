#include "App.h"
#include "FirstBootService.h"
#include "FirstBootWizard.h"
#include "PlatformPaths.h"
#include "Settings.h"
#include <windows.h>
#include <windowsx.h>
#include <chrono>
#include <string>
#include <thread>

namespace {
constexpr UINT WM_ZERO_INSTALL_HEADER = WM_APP + 42;
constexpr int ZERO_HEADER_ID = 7001;
constexpr int ZERO_MENU_PROFILE = 7101;
constexpr int ZERO_MENU_SETTINGS = 7102;
constexpr int ZERO_MENU_QUIT = 7103;

WNDPROC gZeroOriginalWindowProc = nullptr;
HWND gZeroHeader = nullptr;
std::wstring gZeroProfileName = L"Player";

std::wstring widenUtf8(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return std::wstring(text.begin(), text.end());
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), count);
    return out;
}

void sendSidebarClick(HWND parent, size_t index) {
    constexpr int navStartY = 126;
    constexpr int navStride = 52;
    const int y = navStartY + static_cast<int>(index) * navStride + 20;
    const LPARAM point = MAKELPARAM(80, y);
    SendMessageW(parent, WM_LBUTTONDOWN, MK_LBUTTON, point);
    SendMessageW(parent, WM_LBUTTONUP, 0, point);
}

LRESULT CALLBACK ZeroHeaderProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);

            HBRUSH background = CreateSolidBrush(RGB(10, 16, 26));
            FillRect(dc, &rc, background);
            DeleteObject(background);

            SetBkMode(dc, TRANSPARENT);
            HFONT font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Variable Text");
            HFONT oldFont = static_cast<HFONT>(SelectObject(dc, font));

            const int width = rc.right - rc.left;
            const int accountWidth = 220;
            const int downloadWidth = 130;
            const int notificationWidth = 165;
            const int gap = 20;
            const int accountLeft = width - accountWidth - 18;
            const int downloadLeft = accountLeft - gap - downloadWidth;
            const int notificationLeft = downloadLeft - gap - notificationWidth;

            SetTextColor(dc, RGB(140, 154, 175));
            RECT notificationRect{notificationLeft, 27, notificationLeft + notificationWidth, 60};
            DrawTextW(dc, L"NOTIFICATIONS", -1, &notificationRect,
                DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

            RECT downloadRect{downloadLeft, 27, downloadLeft + downloadWidth, 60};
            DrawTextW(dc, L"DOWNLOADS", -1, &downloadRect,
                DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

            HBRUSH chip = CreateSolidBrush(RGB(18, 26, 39));
            RECT chipRect{accountLeft, 13, width - 18, 78};
            FillRect(dc, &chipRect, chip);
            DeleteObject(chip);

            SetTextColor(dc, RGB(244, 248, 255));
            RECT nameRect{accountLeft + 16, 17, width - 40, 45};
            DrawTextW(dc, gZeroProfileName.c_str(), -1, &nameRect,
                DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

            SetTextColor(dc, RGB(140, 154, 175));
            RECT stateRect{accountLeft + 16, 43, width - 40, 71};
            DrawTextW(dc, L"LOCAL", -1, &stateRect,
                DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

            SelectObject(dc, oldFont);
            DeleteObject(font);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int width = rc.right - rc.left;
            const int accountLeft = width - 220 - 18;
            const int x = GET_X_LPARAM(lp);
            const int y = GET_Y_LPARAM(lp);

            if (x >= accountLeft && y >= 10 && y <= 82) {
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING, ZERO_MENU_PROFILE, L"Profile");
                AppendMenuW(menu, MF_STRING, ZERO_MENU_SETTINGS, L"Settings");
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, ZERO_MENU_QUIT, L"Quit ZERO Player");

                POINT pt{x, y};
                ClientToScreen(hwnd, &pt);
                const int command = TrackPopupMenu(menu,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_TOPALIGN,
                    pt.x, pt.y, 0, hwnd, nullptr);
                DestroyMenu(menu);

                HWND parent = GetParent(hwnd);
                if (command == ZERO_MENU_PROFILE) sendSidebarClick(parent, 9);
                else if (command == ZERO_MENU_SETTINGS) sendSidebarClick(parent, 11);
                else if (command == ZERO_MENU_QUIT) PostMessageW(parent, WM_CLOSE, 0, 0);
                return 0;
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void createHeaderOverlay(HWND parent) {
    if (gZeroHeader || !parent) return;

    WNDCLASSW wc{};
    wc.lpfnWndProc = ZeroHeaderProc;
    wc.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
    wc.lpszClassName = L"ZeroProductionHeaderOverlay";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    RECT rc{};
    GetClientRect(parent, &rc);
    const int clientWidth = rc.right - rc.left;
    const int left = (clientWidth > 930) ? clientWidth - 700 : 500;
    const int overlayWidth = clientWidth - left;

    gZeroHeader = CreateWindowExW(0, wc.lpszClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        left, 0, overlayWidth, 94,
        parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ZERO_HEADER_ID)), wc.hInstance, nullptr);
}

void layoutHeaderOverlay(HWND parent) {
    if (!gZeroHeader || !parent) return;
    RECT rc{};
    GetClientRect(parent, &rc);
    const int clientWidth = rc.right - rc.left;
    const int left = (clientWidth > 930) ? clientWidth - 700 : 500;
    const int overlayWidth = clientWidth - left;
    SetWindowPos(gZeroHeader, HWND_TOP, left, 0, overlayWidth, 94, SWP_SHOWWINDOW);
    InvalidateRect(gZeroHeader, nullptr, TRUE);
}

LRESULT CALLBACK ZeroProductionWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_ZERO_INSTALL_HEADER) {
        createHeaderOverlay(hwnd);
        layoutHeaderOverlay(hwnd);
        return 0;
    }

    if (msg == WM_GETMINMAXINFO) {
        auto* info = reinterpret_cast<MINMAXINFO*>(lp);
        if (info) {
            info->ptMinTrackSize.x = 1280;
            info->ptMinTrackSize.y = 720;
        }
        return 0;
    }

    if (msg == WM_SIZE) {
        const LRESULT result = gZeroOriginalWindowProc
            ? CallWindowProcW(gZeroOriginalWindowProc, hwnd, msg, wp, lp)
            : DefWindowProcW(hwnd, msg, wp, lp);
        layoutHeaderOverlay(hwnd);
        return result;
    }

    // Keep F11 useful without dropping back into an unclosable WS_POPUP shell.
    if (msg == WM_KEYDOWN && wp == VK_F11) {
        ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
        return 0;
    }

    if (msg == WM_DESTROY) gZeroHeader = nullptr;

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

        // App::Run initially enters the cinematic borderless shell. Once startup has
        // completed, restore standard Windows chrome so Minimize / Maximize / Close,
        // Alt+F4, task switching, and ordinary window management always work.
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
        PostMessageW(hwnd, WM_ZERO_INSTALL_HEADER, 0, 0);
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

    zero::SettingsStore settingsStore(root);
    const auto settings = settingsStore.Load();
    const auto profileName = settings.profileName.empty() ? std::string("Player") : settings.profileName;
    gZeroProfileName = widenUtf8(profileName);

    InstallProductionWindowChromeAsync();
    zero::App app(hInstance);
    return app.Run();
}
