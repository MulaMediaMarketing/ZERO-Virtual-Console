#include "FirstBootWizard.h"
#include <Xinput.h>
#include <algorithm>

namespace zero {
namespace {
constexpr COLORREF kBg = RGB(249, 248, 244);
constexpr COLORREF kText = RGB(20, 20, 20);
constexpr COLORREF kMuted = RGB(105, 105, 105);
constexpr COLORREF kCard = RGB(28, 28, 28);
constexpr COLORREF kSoft = RGB(235, 233, 228);

int stepIndex(FirstBootWizard::Step step) {
    switch (step) {
        case FirstBootWizard::Step::Welcome: return 0;
        case FirstBootWizard::Step::Profile: return 1;
        case FirstBootWizard::Step::Controller: return 2;
        case FirstBootWizard::Step::Display: return 3;
        case FirstBootWizard::Step::Audio: return 4;
        case FirstBootWizard::Step::Complete: return 5;
    }
    return 0;
}

std::wstring widen(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return std::wstring(text.begin(), text.end());
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), count);
    return out;
}

std::string narrow(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string out(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), count, nullptr, nullptr);
    return out;
}
}

FirstBootWizard::FirstBootWizard(HINSTANCE instance, FirstBootService& service)
    : instance_(instance), service_(service), state_(service.Load()), volume_(state_.volume) {
    if (volume_ > 100) volume_ = 80;
}

bool FirstBootWizard::Run() {
    if (!InitWindow()) return false;
    ShowWindow(hwnd_, SW_SHOW);
    EnterFullscreen();
    UpdateDisplayMetadata();
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, 1, 16, nullptr);

    MSG msg{};
    while (!finished_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (hwnd_) {
        KillTimer(hwnd_, 1);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    return accepted_;
}

bool FirstBootWizard::InitWindow() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance_;
    wc.lpszClassName = L"ZeroFirstBootWizard";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"ZERO Setup",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 1280, 720, nullptr, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

void FirstBootWizard::EnterFullscreen() {
    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return;
    SetWindowPos(hwnd_, HWND_TOP,
        mi.rcMonitor.left, mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void FirstBootWizard::UpdateDisplayMetadata() {
    if (!hwnd_) return;
    RECT rc{};
    if (!GetClientRect(hwnd_, &rc)) return;
    state_.displayWidth = static_cast<unsigned>(std::max<LONG>(0, rc.right - rc.left));
    state_.displayHeight = static_cast<unsigned>(std::max<LONG>(0, rc.bottom - rc.top));
}

void FirstBootWizard::DrawCentered(HDC dc, const std::wstring& text, int y, int height, HFONT font, COLORREF color) {
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    RECT line{70, y, rc.right - 70, y + height};
    auto oldFont = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &line,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(dc, oldFont);
}

void FirstBootWizard::Paint() {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd_, &ps);
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    HBRUSH bg = CreateSolidBrush(kBg);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    HFONT logo = CreateFontW(34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Display");
    HFONT title = CreateFontW(54, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Display");
    HFONT body = CreateFontW(23, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Text");
    HFONT small = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Text");

    SetBkMode(dc, TRANSPARENT);
    RECT brand{60, 40, 380, 96};
    SelectObject(dc, logo);
    SetTextColor(dc, kText);
    DrawTextW(dc, L"ZERO", -1, &brand, DT_LEFT | DT_TOP | DT_SINGLELINE);

    const int progressWidth = 320;
    const int progressLeft = rc.right - progressWidth - 64;
    const int active = stepIndex(step_);
    for (int i = 0; i < 6; ++i) {
        RECT segment{progressLeft + i * 48, 58, progressLeft + i * 48 + 34, 64};
        HBRUSH segmentBrush = CreateSolidBrush(i <= active ? kText : RGB(205, 203, 198));
        FillRect(dc, &segment, segmentBrush);
        DeleteObject(segmentBrush);
    }

    const int centerY = std::max(170, static_cast<int>(rc.bottom / 2 - 180));
    std::wstring heading;
    std::wstring description;
    std::wstring action;
    std::wstring detail;

    switch (step_) {
        case Step::Welcome:
            heading = L"Welcome to ZERO";
            description = L"A controller-first console layer for this PC.";
            detail = L"Setup takes about a minute and stays local on this device.";
            action = L"A / Enter   Start setup";
            break;
        case Step::Profile:
            heading = L"Choose your local profile";
            description = L"This name is used by ZERO on this PC. Online identity is a separate future service.";
            detail = L"Type to edit:  " + widen(state_.profileName);
            action = L"A / Enter   Continue";
            break;
        case Step::Controller:
            heading = L"Connect your controller";
            description = controllerConnected_
                ? L"Controller detected. Press A on it to confirm the console controls."
                : L"Waiting for an XInput-compatible controller. Keyboard setup remains available.";
            detail = controllerConnected_ ? L"Status   Connected" : L"Status   Not detected";
            action = controllerConnected_ ? L"A   Confirm controller" : L"Enter   Continue with keyboard fallback";
            break;
        case Step::Display:
            heading = L"Display check";
            description = L"ZERO should fill this display cleanly with no Windows chrome.";
            detail = std::to_wstring(state_.displayWidth) + L" × " + std::to_wstring(state_.displayHeight) + L"   Borderless fullscreen";
            action = L"A / Enter   Looks good";
            break;
        case Step::Audio:
            heading = L"Starting volume";
            description = L"Set ZERO's saved starting volume. This can be changed later in Settings.";
            detail = L"Volume   " + std::to_wstring(volume_) + L"%";
            action = L"Left / Right   Adjust     A / Enter   Confirm";
            break;
        case Step::Complete:
            heading = L"ZERO is ready";
            description = L"Your profile, controls, display, and starting volume are saved.";
            detail = controllerConnected_ ? L"Controller ready   ·   Home next" : L"Keyboard fallback active   ·   Home next";
            action = L"A / Enter   Go to Home";
            break;
    }

    DrawCentered(dc, heading, centerY, 82, title, kText);
    DrawCentered(dc, description, centerY + 96, 46, body, kMuted);
    DrawCentered(dc, detail, centerY + 148, 40, small, kMuted);

    RECT card{rc.right / 2 - 355, centerY + 216, rc.right / 2 + 355, centerY + 294};
    HBRUSH cardBrush = CreateSolidBrush(kCard);
    HPEN cardPen = CreatePen(PS_SOLID, 1, kCard);
    auto oldBrush = SelectObject(dc, cardBrush);
    auto oldPen = SelectObject(dc, cardPen);
    RoundRect(dc, card.left, card.top, card.right, card.bottom, 24, 24);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(cardPen);
    DeleteObject(cardBrush);
    DrawCentered(dc, action, centerY + 216, 78, small, RGB(255, 255, 255));

    if (!notice_.empty()) DrawCentered(dc, notice_, centerY + 314, 36, small, RGB(110, 80, 40));
    DrawCentered(dc, step_ == Step::Welcome ? L"Esc   Exit setup" : L"B / Esc   Back", rc.bottom - 74, 30, small, kMuted);

    DeleteObject(logo);
    DeleteObject(title);
    DeleteObject(body);
    DeleteObject(small);
    EndPaint(hwnd_, &ps);
}

void FirstBootWizard::PollController() {
    XINPUT_STATE xs{};
    const bool connected = XInputGetState(0, &xs) == ERROR_SUCCESS;
    if (controllerConnected_ != connected) {
        controllerConnected_ = connected;
        state_.controllerDetected = connected;
        previousButtons_ = 0;
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
    if (!connected) {
        previousButtons_ = 0;
        return;
    }

    const WORD now = xs.Gamepad.wButtons;
    const WORD pressed = static_cast<WORD>(now & ~previousButtons_);
    previousButtons_ = now;

    if (pressed & XINPUT_GAMEPAD_B) { Back(); return; }
    if (step_ == Step::Audio) {
        if (pressed & XINPUT_GAMEPAD_DPAD_LEFT) volume_ = volume_ >= 5 ? volume_ - 5 : 0;
        if (pressed & XINPUT_GAMEPAD_DPAD_RIGHT) volume_ = std::min(100u, volume_ + 5);
    }
    if (pressed & XINPUT_GAMEPAD_A) Advance(true);
}

void FirstBootWizard::Advance(bool fromController) {
    notice_.clear();
    switch (step_) {
        case Step::Welcome:
            step_ = Step::Profile;
            break;
        case Step::Profile:
            if (state_.profileName.empty()) state_.profileName = "Player";
            step_ = Step::Controller;
            break;
        case Step::Controller:
            if (fromController && controllerConnected_) {
                state_.controllerConfirmed = true;
                state_.controllerDetected = true;
            } else if (!controllerConnected_) {
                state_.controllerConfirmed = true;
                state_.controllerDetected = false;
                notice_ = L"Keyboard fallback saved. Connect a controller anytime from the shell.";
            } else {
                state_.controllerConfirmed = true;
                state_.controllerDetected = true;
            }
            step_ = Step::Display;
            break;
        case Step::Display:
            UpdateDisplayMetadata();
            state_.displayConfirmed = true;
            step_ = Step::Audio;
            break;
        case Step::Audio:
            state_.volume = volume_;
            state_.audioConfirmed = true;
            step_ = Step::Complete;
            break;
        case Step::Complete:
            Complete();
            return;
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void FirstBootWizard::Back() {
    notice_.clear();
    switch (step_) {
        case Step::Welcome: finished_ = true; accepted_ = false; return;
        case Step::Profile: step_ = Step::Welcome; break;
        case Step::Controller: step_ = Step::Profile; break;
        case Step::Display: step_ = Step::Controller; break;
        case Step::Audio: step_ = Step::Display; break;
        case Step::Complete: step_ = Step::Audio; break;
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void FirstBootWizard::Complete() {
    state_.completed = true;
    state_.volume = volume_;
    UpdateDisplayMetadata();
    std::wstring error;
    if (!service_.Save(state_, error)) {
        MessageBoxW(hwnd_, error.c_str(), L"ZERO Setup", MB_OK | MB_ICONERROR);
        return;
    }
    accepted_ = true;
    finished_ = true;
}

LRESULT CALLBACK FirstBootWizard::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    FirstBootWizard* self = reinterpret_cast<FirstBootWizard*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<FirstBootWizard*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    return self ? self->HandleMessage(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT FirstBootWizard::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT:
            Paint();
            return 0;
        case WM_SIZE:
            UpdateDisplayMetadata();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;
        case WM_TIMER:
            PollController();
            return 0;
        case WM_CHAR:
            if (step_ == Step::Profile) {
                std::wstring profile = widen(state_.profileName);
                const wchar_t ch = static_cast<wchar_t>(wp);
                if (ch == L'\b') {
                    if (!profile.empty()) profile.pop_back();
                } else if (ch >= 32 && ch != 127 && profile.size() < 24) {
                    profile.push_back(ch);
                }
                state_.profileName = narrow(profile);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) { Back(); return 0; }
            if (step_ == Step::Audio && wp == VK_LEFT) {
                volume_ = volume_ >= 5 ? volume_ - 5 : 0;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
            if (step_ == Step::Audio && wp == VK_RIGHT) {
                volume_ = std::min(100u, volume_ + 5);
                InvalidateRect(hwnd_, nullptr, FALSE);
                return 0;
            }
            if (wp == VK_RETURN || wp == VK_SPACE) { Advance(false); return 0; }
            return 0;
        case WM_CLOSE:
            finished_ = true;
            accepted_ = false;
            return 0;
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

} // namespace zero
