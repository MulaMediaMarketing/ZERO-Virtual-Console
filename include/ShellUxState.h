#pragma once
#include <algorithm>
#include <chrono>

namespace zero {

class ShellUxState {
public:
    void SetReducedMotion(bool reduced) noexcept {
        reducedMotion_ = reduced;
        if (reducedMotion_) {
            transition_ = 1.0f;
            focusPulse_ = 1.0f;
        }
    }

    bool ReducedMotion() const noexcept { return reducedMotion_; }

    void BeginPageTransition() noexcept {
        transition_ = reducedMotion_ ? 1.0f : 0.0f;
    }

    void NotifyFocusMoved() noexcept {
        focusPulse_ = reducedMotion_ ? 1.0f : 0.0f;
    }

    void Advance(std::chrono::duration<float> delta) noexcept {
        const float seconds = std::max(0.0f, delta.count());
        if (reducedMotion_) {
            transition_ = 1.0f;
            focusPulse_ = 1.0f;
            return;
        }
        transition_ = std::min(1.0f, transition_ + seconds / 0.18f);
        focusPulse_ = std::min(1.0f, focusPulse_ + seconds / 0.11f);
    }

    float PageOffsetY() const noexcept {
        if (reducedMotion_) return 0.0f;
        const float t = EaseOutCubic(transition_);
        return (1.0f - t) * 18.0f;
    }

    float FocusThickness() const noexcept {
        if (reducedMotion_) return 3.0f;
        const float t = EaseOutCubic(focusPulse_);
        return 4.5f - (1.5f * t);
    }

private:
    static float EaseOutCubic(float t) noexcept {
        t = std::clamp(t, 0.0f, 1.0f);
        const float u = 1.0f - t;
        return 1.0f - (u * u * u);
    }

    bool reducedMotion_{false};
    float transition_{1.0f};
    float focusPulse_{1.0f};
};

} // namespace zero
