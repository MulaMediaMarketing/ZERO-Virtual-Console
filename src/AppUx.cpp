#include "App.h"

using Microsoft::WRL::ComPtr;

namespace zero {

void App::NotifyFocusMoved() {
    shellUx_.NotifyFocusMoved();
    lastInput_ = std::chrono::steady_clock::now();
}

void App::DrawFocusRing(const D2D1_RECT_F& bounds, float radius, bool light) {
    if (!target_) return;

    ComPtr<ID2D1SolidColorBrush> focus;
    const auto color = light
        ? D2D1::ColorF(D2D1::ColorF::White, 0.96f)
        : D2D1::ColorF(0x171717, 0.95f);
    if (FAILED(target_->CreateSolidColorBrush(color, focus.GetAddressOf()))) return;

    const float inset = shellUx_.ReducedMotion() ? 3.0f : 2.0f;
    const auto ring = D2D1::RoundedRect(
        D2D1::RectF(bounds.left - inset,
                    bounds.top - inset,
                    bounds.right + inset,
                    bounds.bottom + inset),
        radius + inset,
        radius + inset);
    target_->DrawRoundedRectangle(ring, focus.Get(), shellUx_.FocusThickness());
}

} // namespace zero
