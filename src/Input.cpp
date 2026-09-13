#include "Input.h"
#include <Xinput.h>

namespace zero {

bool Input::RepeatNavigation(NavDirection direction, bool held,
                             std::chrono::steady_clock::time_point now) {
    using namespace std::chrono_literals;
    if (!held) {
        if (repeatingDirection_ == direction) repeatingDirection_ = NavDirection::None;
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

InputSnapshot Input::Poll() {
    InputSnapshot out{};
    const auto now = std::chrono::steady_clock::now();

    bool heldUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
    bool heldDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
    bool heldLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    bool heldRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;

    XINPUT_STATE state{};
    if (XInputGetState(0, &state) == ERROR_SUCCESS) {
        const WORD b = state.Gamepad.wButtons;
        auto pressed = [&](WORD mask){ return (b & mask) && !(previousButtons_ & mask); };

        const int threshold = 18000;
        heldUp |= (b & XINPUT_GAMEPAD_DPAD_UP) != 0 || state.Gamepad.sThumbLY > threshold;
        heldDown |= (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0 || state.Gamepad.sThumbLY < -threshold;
        heldLeft |= (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0 || state.Gamepad.sThumbLX < -threshold;
        heldRight |= (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 || state.Gamepad.sThumbLX > threshold;

        out.select = pressed(XINPUT_GAMEPAD_A);
        out.action = pressed(XINPUT_GAMEPAD_X);
        out.back = pressed(XINPUT_GAMEPAD_B);
        out.menu = pressed(XINPUT_GAMEPAD_START);
        out.shoulderLeft = pressed(XINPUT_GAMEPAD_LEFT_SHOULDER);
        out.shoulderRight = pressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);

        previousButtons_ = b;
        previousLX_ = state.Gamepad.sThumbLX;
        previousLY_ = state.Gamepad.sThumbLY;
    }

    if (heldUp) out.up = RepeatNavigation(NavDirection::Up, true, now);
    else RepeatNavigation(NavDirection::Up, false, now);
    if (heldDown) out.down = RepeatNavigation(NavDirection::Down, true, now);
    else RepeatNavigation(NavDirection::Down, false, now);
    if (heldLeft) out.left = RepeatNavigation(NavDirection::Left, true, now);
    else RepeatNavigation(NavDirection::Left, false, now);
    if (heldRight) out.right = RepeatNavigation(NavDirection::Right, true, now);
    else RepeatNavigation(NavDirection::Right, false, now);

    out.select |= (GetAsyncKeyState(VK_RETURN) & 1) != 0;
    out.action |= (GetAsyncKeyState('X') & 1) != 0;
    out.back |= (GetAsyncKeyState(VK_ESCAPE) & 1) != 0;
    out.shoulderLeft |= (GetAsyncKeyState(VK_PRIOR) & 1) != 0;
    out.shoulderRight |= (GetAsyncKeyState(VK_NEXT) & 1) != 0;
    return out;
}
}
