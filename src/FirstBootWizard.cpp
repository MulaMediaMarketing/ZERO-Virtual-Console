#include "FirstBootWizard.h"
#include <Xinput.h>
#include <algorithm>

namespace zero {

FirstBootWizard::FirstBootWizard(HINSTANCE instance, FirstBootService& service)
    : instance_(instance), service_(service), state_(service.Load()) {}

bool FirstBootWizard::Run() {
    if (!InitWindow()) return false;
    ShowWindow(hwnd_, SW_SHOW);
    EnterFullscreen();
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, 1, 16, nullptr);

    MSG msg{};
    while (!finished_ && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (hwnd_) DestroyWindow(hwnd_);
    return accepted_;
}

bool FirstBootWizard::InitWindow() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance_;
    wc.lpszClassName = L"ZeroFirstBootWizard";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"ZERO Setup",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 1280, 720, nullptr, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

void FirstBootWizard::EnterFullscreen() {
    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{}; mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return;
    SetWindowPos(hwnd_, HWND_TOP,
        mi.rcMonitor.left, mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void FirstBootWizard::DrawCentered(HDC dc, const std::wstring& text, int y, int height, HFONT font, COLORREF color) {
    RECT rc{}; GetClientRect(hwnd_, &rc);
    RECT line{60, y, rc.right - 60, y + height};
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
    RECT rc{}; GetClientRect(hwnd_, &rc);
    HBRUSH bg = CreateSolidBrush(RGB(249, 248, 244));
    FillRect(dc, &rc, bg);
    DeleteObject(bg);

    HFONT logo = CreateFontW(34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Display");
    HFONT title = CreateFontW(56, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Display");
    HFONT body = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Text");
    HFONT small = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_SWISS, L"Segoe UI Variable Text");

    SetBkMode(dc, TRANSPARENT);
    RECT brand{60, 44, 340, 100};
    SelectObject(dc, logo);
    SetTextColor(dc, RGB(20,20,20));
    DrawTextW(dc, L"ZERO", -1, &brand, DT_LEFT | DT_TOP | DT_SINGLELINE);

    const int centerY = std::max(180, rc.bottom / 2 - 170);
    std::wstring heading;
    std::wstring description;
    std::wstring action;

    switch (step_) {
        case Step::Welcome:
            heading = L"Welcome to ZERO";
            description = L"Turn this PC into a controller-first game console experience.";
            action = L"A / Enter   Start setup";
            break;
        case Step::Profile:
            heading = L"Your ZERO profile";
            description = L"Milestone 1 uses a local profile. Online ZERO ID comes later.";
            action = L"Profile: " + std::wstring(state_.profileName.begin(), state_.profileName.end()) + L"     A / Enter   Continue";
            break;
        case Step::Controller:
            heading = L"Controller";
            description = L"Use your controller to navigate ZERO. Press A to confirm it works.";
            action = L"A   Confirm controller";
            break;
        case Step::Display:
            heading = L"Display";
            description = L"ZERO is running borderless fullscreen on this display.";
            action = L"A / Enter   Looks good";
            break;
        case Step::Audio:
            heading = L"Audio";
            description = L"Choose your starting system volume. You can change it later in Settings.";
            action = L"Volume  " + std::to_wstring(volume_) + L"%     Left / Right   Adjust     A / Enter   Confirm";
            break;
        case Step::Complete:
            heading = L"ZERO is ready";
            description = L"Your console shell is configured. Next, import a compatible game.";
            action = L"A / Enter   Go to Home";
            break;
    }

    DrawCentered(dc, heading, centerY, 90, title, RGB(20,20,20));
    DrawCentered(dc, description, centerY + 102, 55, body, RGB(100,100,100));

    RECT card{rc.right/2 - 330, centerY + 200, rc.right/2 + 330, centerY + 278};
    HBRUSH cardBrush = CreateSolidBrush(RGB(28,28,28));
    FillRect(dc, &card, cardBrush);
    DeleteObject(cardBrush);
    DrawCentered(dc, action, centerY + 200, 78, small, RGB(255,255,255));

    DrawCentered(dc, L"B / Esc   Back", rc.bottom - 80, 32, small, RGB(120,120,120));

    DeleteObject(logo); DeleteObject(title); DeleteObject(body); DeleteObject(small);
    EndPaint(hwnd_, &ps);
}

void FirstBootWizard::PollController() {
    XINPUT_STATE xs{};
    if (XInputGetState(0, &xs) != ERROR_SUCCESS) {
        previousButtons_ = 0;
        return;
    }
    const WORD now = xs.Gamepad.wButtons;
    const WORD pressed = static_cast<WORD>(now & ~previousButtons_);
    previousButtons_ = now;

    if (pressed & XINPUT_GAMEPAD_B) { Back(); return; }
    if (step_ == Step::Audio) {
        if (pressed & XINPUT_GAMEPAD_DPAD_LEFT) volume_ = volume_ >= 10 ? volume_ - 10 : 0;
        if (pressed & XINPUT_GAMEPAD_DPAD_RIGHT) volume_ = std::min(100u, volume_ + 10);
    }
    if (pressed & XINPUT_GAMEPAD_A) Advance();
}

void FirstBootWizard::Advance() {
    switch (step_) {
        case Step::Welcome: step_ = Step::Profile; break;
        case Step::Profile: step_ = Step::Controller; break;
        case Step::Controller: state_.controllerConfirmed = true; step_ = Step::Display; break;
        case Step::Display: state_.displayConfirmed = true; step_ = Step::Audio; break;
        case Step::Audio: state_.audioConfirmed = true; step_ = Step::Complete; break;
        case Step::Complete: Complete(); return;
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void FirstBootWizard::Back() {
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

LRESULT FirstBootWizard::HandleMessage(UINT msg, WPARAM wp, LPARAM) {
    switch (msg) {
        case WM_PAINT: Paint(); return 0;
        case WM_TIMER: PollController(); InvalidateRect(hwnd_, nullptr, FALSE); return 0;
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) { Back(); return 0; }
            if (step_ == Step::Audio && wp == VK_LEFT) { volume_ = volume_ >= 10 ? volume_ - 10 : 0; InvalidateRect(hwnd_, nullptr, FALSE); return 0; }
            if (step_ == Step::Audio && wp == VK_RIGHT) { volume_ = std::min(100u, volume_ + 10); InvalidateRect(hwnd_, nullptr, FALSE); return 0; }
            if (wp == VK_RETURN || wp == VK_SPACE) { Advance(); return 0; }
            return 0;
        case WM_CLOSE: finished_ = true; accepted_ = false; return 0;
    }
    return DefWindowProcW(hwnd_, msg, wp, 0);
}

} // namespace zero
