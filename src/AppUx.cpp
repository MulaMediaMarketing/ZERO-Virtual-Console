#include "App.h"

using Microsoft::WRL::ComPtr;

namespace zero {

void App::NotifyFocusMoved() {
    shellUx_.NotifyFocusMoved();
    lastInput_ = std::chrono::steady_clock::now();
}

bool App::ShellContentOwnsFocus() const noexcept {
    return launchUxMode_ == LaunchUxMode::Hidden &&
           !shellUx_.OverlayRenderActive() &&
           capturesUx_.mode != CaptureExperienceMode::Viewer &&
           capturesUx_.mode != CaptureExperienceMode::DeleteConfirm;
}

void App::RestoreShellForeground() {
    if (!hwnd_) return;
    EnterBorderlessFullscreen();
    SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

void App::DrawFocusRing(const D2D1_RECT_F& bounds, float radius, bool light) {
    if (!target_) return;
    ComPtr<ID2D1SolidColorBrush> focus;
    const auto color = light
        ? D2D1::ColorF(D2D1::ColorF::White, 0.98f)
        : D2D1::ColorF(0x20CFFF, 0.98f);
    if (FAILED(target_->CreateSolidColorBrush(color, focus.GetAddressOf()))) return;
    const float inset = shellUx_.ReducedMotion() ? 3.5f : 2.5f;
    const auto ring = D2D1::RoundedRect(
        D2D1::RectF(bounds.left - inset, bounds.top - inset, bounds.right + inset, bounds.bottom + inset),
        radius + inset, radius + inset);
    target_->DrawRoundedRectangle(ring, focus.Get(), shellUx_.FocusThickness());
}

} // namespace zero
