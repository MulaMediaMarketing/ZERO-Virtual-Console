#include "InputCore.h"
#include <algorithm>
#include <cmath>

namespace zero {

bool InputCore::HasActivity(const ControllerInputSample& s) noexcept {
    return s.dpadUp || s.dpadDown || s.dpadLeft || s.dpadRight ||
           s.select || s.action || s.back || s.menu ||
           s.shoulderLeft || s.shoulderRight ||
           std::abs(static_cast<int>(s.leftX)) >= kAnalogEngage ||
           std::abs(static_cast<int>(s.leftY)) >= kAnalogEngage;
}

bool InputCore::DirectionHeld(NavDirection direction, bool up, bool down, bool left, bool right) noexcept {
    switch (direction) {
        case NavDirection::Up: return up;
        case NavDirection::Down: return down;
        case NavDirection::Left: return left;
        case NavDirection::Right: return right;
        default: return false;
    }
}

void InputCore::UpdateAnalogLatch(size_t index, const ControllerInputSample& s) noexcept {
    auto& latch = analog_[index];

    if (latch.up) latch.up = s.leftY > kAnalogRelease;
    else latch.up = s.leftY > kAnalogEngage;

    if (latch.down) latch.down = s.leftY < -kAnalogRelease;
    else latch.down = s.leftY < -kAnalogEngage;

    if (latch.left) latch.left = s.leftX < -kAnalogRelease;
    else latch.left = s.leftX < -kAnalogEngage;

    if (latch.right) latch.right = s.leftX > kAnalogRelease;
    else latch.right = s.leftX > kAnalogEngage;

    if (latch.up && latch.down) { latch.up = false; latch.down = false; }
    if (latch.left && latch.right) { latch.left = false; latch.right = false; }
}

InputCore::NavDirection InputCore::ResolveDirection(bool up, bool down, bool left, bool right,
                                                    const ControllerInputSample* controller) const noexcept {
    if (up && down) { up = false; down = false; }
    if (left && right) { left = false; right = false; }

    const bool vertical = up || down;
    const bool horizontal = left || right;
    if (!vertical && !horizontal) return NavDirection::None;
    if (vertical && !horizontal) return up ? NavDirection::Up : NavDirection::Down;
    if (!vertical && horizontal) return left ? NavDirection::Left : NavDirection::Right;

    // For diagonals, use the dominant analog axis when available. D-pad ties prefer
    // the current repeating direction to avoid focus jitter, otherwise vertical.
    if (controller) {
        const int ax = std::abs(static_cast<int>(controller->leftX));
        const int ay = std::abs(static_cast<int>(controller->leftY));
        if (ax > ay + 1500) return left ? NavDirection::Left : NavDirection::Right;
        if (ay > ax + 1500) return up ? NavDirection::Up : NavDirection::Down;
    }
    if (DirectionHeld(repeatingDirection_, up, down, left, right)) return repeatingDirection_;
    return up ? NavDirection::Up : NavDirection::Down;
}

bool InputCore::RepeatNavigation(NavDirection direction, Clock::time_point now) {
    using namespace std::chrono_literals;
    if (direction == NavDirection::None) {
        repeatingDirection_ = NavDirection::None;
        return false;
    }
    if (repeatingDirection_ != direction) {
        repeatingDirection_ = direction;
        repeatStartedAt_ = now;
        lastRepeatAt_ = now;
        return true;
    }
    if (now - repeatStartedAt_ < 330ms) return false;
    if (now - lastRepeatAt_ < 95ms) return false;
    lastRepeatAt_ = now;
    return true;
}

InputSnapshot InputCore::Update(const std::array<ControllerInputSample, 4>& controllers,
                                const KeyboardInputSample& keyboard,
                                Clock::time_point now) {
    InputSnapshot out{};

    for (size_t i = 0; i < controllers.size(); ++i) {
        if (controllers[i].connected) UpdateAnalogLatch(i, controllers[i]);
        else analog_[i] = {};
    }

    if (activeController_ >= 0 && !controllers[static_cast<size_t>(activeController_)].connected) {
        activeController_ = -1;
        repeatingDirection_ = NavDirection::None;
    }

    // Keep the current controller while it remains connected. If none is owned,
    // first controller with meaningful activity claims navigation; otherwise the
    // first connected controller is used for stable couch behavior.
    if (activeController_ < 0) {
        for (size_t i = 0; i < controllers.size(); ++i) {
            if (controllers[i].connected && HasActivity(controllers[i])) {
                activeController_ = static_cast<int>(i);
                break;
            }
        }
        if (activeController_ < 0) {
            for (size_t i = 0; i < controllers.size(); ++i) {
                if (controllers[i].connected) {
                    activeController_ = static_cast<int>(i);
                    break;
                }
            }
        }
    }

    const ControllerInputSample* c = nullptr;
    const ControllerInputSample* p = nullptr;
    AnalogLatch latch{};
    if (activeController_ >= 0) {
        const size_t index = static_cast<size_t>(activeController_);
        c = &controllers[index];
        p = &previous_[index];
        latch = analog_[index];

        auto edge = [&](bool nowValue, bool previousValue) { return nowValue && !previousValue; };
        out.select = edge(c->select, p->select);
        out.action = edge(c->action, p->action);
        out.back = edge(c->back, p->back);
        out.menu = edge(c->menu, p->menu);
        out.shoulderLeft = edge(c->shoulderLeft, p->shoulderLeft);
        out.shoulderRight = edge(c->shoulderRight, p->shoulderRight);
    }

    bool up = keyboard.upHeld;
    bool down = keyboard.downHeld;
    bool left = keyboard.leftHeld;
    bool right = keyboard.rightHeld;
    if (c) {
        up |= c->dpadUp || latch.up;
        down |= c->dpadDown || latch.down;
        left |= c->dpadLeft || latch.left;
        right |= c->dpadRight || latch.right;
    }

    const auto nav = ResolveDirection(up, down, left, right, c);
    if (RepeatNavigation(nav, now)) {
        out.up = nav == NavDirection::Up;
        out.down = nav == NavDirection::Down;
        out.left = nav == NavDirection::Left;
        out.right = nav == NavDirection::Right;
    }

    out.select |= keyboard.selectPressed;
    out.action |= keyboard.actionPressed;
    out.back |= keyboard.backPressed;
    out.menu |= keyboard.menuPressed;
    out.shoulderLeft |= keyboard.shoulderLeftPressed;
    out.shoulderRight |= keyboard.shoulderRightPressed;

    previous_ = controllers;
    return out;
}

} // namespace zero
