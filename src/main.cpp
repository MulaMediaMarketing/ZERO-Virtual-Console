#include "App.h"
#include "FirstBootService.h"
#include "FirstBootWizard.h"
#include "PlaceholderContentProvider.h"
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
bool gZeroPreviewEnabled = false;
bool gZeroPreviewHomeVisible = false;
size_t gZeroCurrentNavIndex = 0;

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

void drawSidebarIcon(HDC dc, size_t index, int x, int y, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));

    const int l = x;
    const int t = y;
    const int r = x + 18;
    const int b = y + 18;
    const int cx = x + 9;
    const int cy = y + 9;

    switch (index) {
        case 0:
            MoveToEx(dc, l + 1, t + 8, nullptr); LineTo(dc, cx, t + 1); LineTo(dc, r - 1, t + 8);
            Rectangle(dc, l + 4, t + 8, r - 3, b - 1);
            break;
        case 1:
            Ellipse(dc, l + 1, t + 1, r - 1, b - 1);
            MoveToEx(dc, cx - 3, cy + 4, nullptr); LineTo(dc, cx + 4, cy - 4); LineTo(dc, cx + 1, cy + 2); LineTo(dc, cx - 3, cy + 4);
            break;
        case 2:
            Rectangle(dc, l + 2, t + 6, r - 2, b - 1);
            Arc(dc, l + 5, t, r - 5, t + 11, 0, 0, 0, 0);
            break;
        case 3:
            Rectangle(dc, l + 1, t + 1, l + 7, t + 7); Rectangle(dc, l + 11, t + 1, r - 1, t + 7);
            Rectangle(dc, l + 1, t + 11, l + 7, b - 1); Rectangle(dc, l + 11, t + 11, r - 1, b - 1);
            break;
        case 4:
            Arc(dc, l + 1, t + 6, l + 11, b - 1, 0, 0, 0, 0);
            Arc(dc, l + 6, t + 1, r - 2, b - 2, 0, 0, 0, 0);
            MoveToEx(dc, l + 4, b - 2, nullptr); LineTo(dc, r - 3, b - 2);
            break;
        case 5:
            MoveToEx(dc, cx, t + 1, nullptr); LineTo(dc, cx, t + 12);
            MoveToEx(dc, cx - 5, t + 8, nullptr); LineTo(dc, cx, t + 13); LineTo(dc, cx + 5, t + 8);
            MoveToEx(dc, l + 2, b - 1, nullptr); LineTo(dc, r - 2, b - 1);
            break;
        case 6:
            Ellipse(dc, l + 2, t + 2, l + 9, t + 9); Ellipse(dc, l + 10, t + 4, r - 1, t + 11);
            Arc(dc, l, t + 7, l + 12, b + 4, 0, 0, 0, 0); Arc(dc, l + 7, t + 9, r + 2, b + 4, 0, 0, 0, 0);
            break;
        case 7:
            Rectangle(dc, l + 5, t + 2, r - 5, t + 10);
            Arc(dc, l, t + 2, l + 8, t + 11, 0, 0, 0, 0); Arc(dc, r - 8, t + 2, r, t + 11, 0, 0, 0, 0);
            MoveToEx(dc, cx, t + 10, nullptr); LineTo(dc, cx, b - 3); MoveToEx(dc, l + 5, b - 2, nullptr); LineTo(dc, r - 5, b - 2);
            break;
        case 8:
            MoveToEx(dc, l + 1, t + 6, nullptr); LineTo(dc, l + 1, t + 1); LineTo(dc, l + 6, t + 1);
            MoveToEx(dc, r - 1, t + 6, nullptr); LineTo(dc, r - 1, t + 1); LineTo(dc, r - 6, t + 1);
            MoveToEx(dc, l + 1, b - 6, nullptr); LineTo(dc, l + 1, b - 1); LineTo(dc, l + 6, b - 1);
            MoveToEx(dc, r - 1, b - 6, nullptr); LineTo(dc, r - 1, b - 1); LineTo(dc, r - 6, b - 1);
            Ellipse(dc, cx - 3, cy - 3, cx + 4, cy + 4);
            break;
        case 9:
            Ellipse(dc, cx - 4, t + 1, cx + 4, t + 9);
            Arc(dc, l + 2, t + 8, r - 2, b + 4, 0, 0, 0, 0);
            break;
        case 10:
            Rectangle(dc, l + 1, t + 2, r - 1, t + 13);
            MoveToEx(dc, cx, t + 13, nullptr); LineTo(dc, cx, b - 2);
            MoveToEx(dc, l + 5, b - 2, nullptr); LineTo(dc, r - 5, b - 2);
            break;
        default:
            Ellipse(dc, cx - 5, cy - 5, cx + 6, cy + 6); Ellipse(dc, cx - 2, cy - 2, cx + 3, cy + 3);
            MoveToEx(dc, cx, t, nullptr); LineTo(dc, cx, t + 4); MoveToEx(dc, cx, b - 4, nullptr); LineTo(dc, cx, b);
            MoveToEx(dc, l, cy, nullptr); LineTo(dc, l + 4, cy); MoveToEx(dc, r - 4, cy, nullptr); LineTo(dc, r, cy);
            break;
    }

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void drawSidebarIcons(HWND hwnd) {
    HDC dc = GetDC(hwnd);
    if (!dc) return;
    for (size_t i = 0; i < 12; ++i) {
        const int y = 126 + static_cast<int>(i) * 52 + 12;
        const COLORREF color = (i == gZeroCurrentNavIndex) ? RGB(240, 248, 255) : RGB(140, 154, 175);
        // Navigation labels begin at x=34. Keep the 18px icon in a dedicated
        // x=8..26 lane so there is always an 8px gap and no icon/text collision.
        drawSidebarIcon(dc, i, 8, y, color);
    }
    ReleaseDC(hwnd, dc);
}

void drawPreviewCard(HDC dc, const zero::PlaceholderGameInfo& game, const RECT& rect, size_t index, bool compact) {
    const COLORREF tones[] = {
        RGB(15, 57, 86), RGB(33, 29, 72), RGB(15, 47, 78), RGB(72, 34, 27),
        RGB(18, 43, 68), RGB(30, 37, 56), RGB(35, 45, 57), RGB(20, 39, 51),
        RGB(22, 48, 63), RGB(63, 27, 38), RGB(29, 43, 63), RGB(26, 34, 57)
    };
    HBRUSH fill = CreateSolidBrush(tones[index % 12]);
    FillRect(dc, &rect, fill);
    DeleteObject(fill);

    HPEN border = CreatePen(PS_SOLID, index == 0 ? 2 : 1, index == 0 ? RGB(32, 207, 255) : RGB(45, 71, 96));
    HGDIOBJ oldPen = SelectObject(dc, border);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(244, 248, 255));
    RECT title{rect.left + 12, rect.top + 12, rect.right - 10, rect.top + (compact ? 42 : 54)};
    DrawTextW(dc, game.title, -1, &title, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_LEFT);

    SetTextColor(dc, RGB(156, 178, 202));
    RECT developer{rect.left + 12, rect.top + (compact ? 40 : 54), rect.right - 10, rect.top + (compact ? 64 : 78)};
    DrawTextW(dc, game.developer, -1, &developer, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_LEFT);

    if (!compact) {
        SetTextColor(dc, RGB(104, 213, 255));
        RECT state{rect.left + 12, rect.bottom - 30, rect.right - 10, rect.bottom - 8};
        DrawTextW(dc, game.playState, -1, &state, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_LEFT);
    }
}

void drawPreviewHome(HWND hwnd) {
    if (!gZeroPreviewEnabled || !gZeroPreviewHomeVisible) return;
    RECT client{};
    GetClientRect(hwnd, &client);
    if (client.right < 1100 || client.bottom < 680) return;

    HDC dc = GetDC(hwnd);
    if (!dc) return;

    const int left = 230;
    RECT content{left, 94, client.right, client.bottom};
    HBRUSH background = CreateSolidBrush(RGB(4, 10, 17));
    FillRect(dc, &content, background);
    DeleteObject(background);

    HFONT titleFont = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Variable Display");
    HFONT bodyFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Variable Text");
    HFONT oldFont = static_cast<HFONT>(SelectObject(dc, titleFont));
    SetBkMode(dc, TRANSPARENT);

    const auto& games = zero::PlaceholderContentProvider::Games();

    RECT hero{left + 28, 112, client.right - 28, 356};
    HBRUSH heroFill = CreateSolidBrush(RGB(8, 30, 49));
    FillRect(dc, &hero, heroFill);
    DeleteObject(heroFill);
    HPEN accent = CreatePen(PS_SOLID, 2, RGB(32, 207, 255));
    HGDIOBJ oldPen = SelectObject(dc, accent);
    for (int x = hero.left + 40; x < hero.right; x += 82) {
        MoveToEx(dc, x, hero.bottom, nullptr); LineTo(dc, x + 120, hero.top);
    }
    SelectObject(dc, oldPen);
    DeleteObject(accent);

    SetTextColor(dc, RGB(154, 190, 219));
    RECT marker{hero.left + 28, hero.top + 22, hero.right - 24, hero.top + 48};
    DrawTextW(dc, L"UI PREVIEW DATA  ·  REMOVABLE PLACEHOLDERS", -1, &marker, DT_SINGLELINE | DT_NOPREFIX);
    SetTextColor(dc, RGB(244, 248, 255));
    RECT heroTitle{hero.left + 28, hero.top + 62, hero.right - 280, hero.top + 108};
    DrawTextW(dc, games[0].title, -1, &heroTitle, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, bodyFont);
    SetTextColor(dc, RGB(180, 201, 220));
    RECT heroDesc{hero.left + 30, hero.top + 116, hero.right - 300, hero.top + 148};
    DrawTextW(dc, games[0].description, -1, &heroDesc, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    HBRUSH playBrush = CreateSolidBrush(RGB(8, 125, 255));
    RECT play{hero.left + 30, hero.top + 164, hero.left + 174, hero.top + 212};
    FillRect(dc, &play, playBrush);
    DeleteObject(playBrush);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT playText{play.left, play.top + 12, play.right, play.bottom};
    DrawTextW(dc, L"PLAY", -1, &playText, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

    SetTextColor(dc, RGB(244, 248, 255));
    RECT continueTitle{left + 28, 374, client.right - 28, 402};
    DrawTextW(dc, L"CONTINUE PLAYING", -1, &continueTitle, DT_SINGLELINE | DT_NOPREFIX);

    const int gap = 12;
    const int available = client.right - left - 56;
    const int cardWidth = (available - gap * 4) / 5;
    for (size_t i = 0; i < 5; ++i) {
        RECT card{left + 28 + static_cast<int>(i) * (cardWidth + gap), 408,
                  left + 28 + static_cast<int>(i) * (cardWidth + gap) + cardWidth, 532};
        drawPreviewCard(dc, games[i], card, i, false);
    }

    RECT recentTitle{left + 28, 550, client.right - 28, 578};
    SetTextColor(dc, RGB(244, 248, 255));
    DrawTextW(dc, L"RECENTLY PLAYED", -1, &recentTitle, DT_SINGLELINE | DT_NOPREFIX);
    const int smallCount = 6;
    const int smallWidth = (available - gap * (smallCount - 1)) / smallCount;
    for (int i = 0; i < smallCount; ++i) {
        RECT card{left + 28 + i * (smallWidth + gap), 584,
                  left + 28 + i * (smallWidth + gap) + smallWidth, 664};
        drawPreviewCard(dc, games[5 + static_cast<size_t>(i)], card, 5 + static_cast<size_t>(i), true);
    }

    SelectObject(dc, oldFont);
    DeleteObject(titleFont);
    DeleteObject(bodyFont);
    ReleaseDC(hwnd, dc);
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

    if (msg == WM_LBUTTONDOWN) {
        const int x = GET_X_LPARAM(lp);
        const int y = GET_Y_LPARAM(lp);
        if (x >= 16 && x <= 214 && y >= 126) {
            const int relative = y - 126;
            const size_t index = static_cast<size_t>(relative / 52);
            if (index < 12 && (relative % 52) <= 42) {
                gZeroCurrentNavIndex = index;
                gZeroPreviewHomeVisible = gZeroPreviewEnabled && index == 0;
            }
        }
    }

    if (msg == WM_PAINT) {
        const LRESULT result = gZeroOriginalWindowProc
            ? CallWindowProcW(gZeroOriginalWindowProc, hwnd, msg, wp, lp)
            : DefWindowProcW(hwnd, msg, wp, lp);
        drawSidebarIcons(hwnd);
        drawPreviewHome(hwnd);
        return result;
    }

    if (msg == WM_SIZE) {
        const LRESULT result = gZeroOriginalWindowProc
            ? CallWindowProcW(gZeroOriginalWindowProc, hwnd, msg, wp, lp)
            : DefWindowProcW(hwnd, msg, wp, lp);
        layoutHeaderOverlay(hwnd);
        return result;
    }

    if (msg == WM_KEYDOWN && wp == VK_F9) {
        gZeroPreviewEnabled = !gZeroPreviewEnabled;
        gZeroPreviewHomeVisible = gZeroPreviewEnabled && gZeroCurrentNavIndex == 0;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

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
    gZeroPreviewEnabled = zero::PlaceholderContentProvider::Enabled();
    gZeroPreviewHomeVisible = gZeroPreviewEnabled;

    InstallProductionWindowChromeAsync();
    zero::App app(hInstance);
    return app.Run();
}