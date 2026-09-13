#include "Input.h"
#include <Xinput.h>
#include <cstdlib>

namespace zero {
InputSnapshot Input::Poll() {
    InputSnapshot out{};
    XINPUT_STATE state{};
    if (XInputGetState(0, &state) == ERROR_SUCCESS) {
        const WORD b = state.Gamepad.wButtons;
        auto pressed = [&](WORD mask){ return (b & mask) && !(previousButtons_ & mask); };
        out.up = pressed(XINPUT_GAMEPAD_DPAD_UP);
        out.down = pressed(XINPUT_GAMEPAD_DPAD_DOWN);
        out.left = pressed(XINPUT_GAMEPAD_DPAD_LEFT);
        out.right = pressed(XINPUT_GAMEPAD_DPAD_RIGHT);
        out.select = pressed(XINPUT_GAMEPAD_A);
        out.action = pressed(XINPUT_GAMEPAD_X);
        out.back = pressed(XINPUT_GAMEPAD_B);
        out.menu = pressed(XINPUT_GAMEPAD_START);
        out.shoulderLeft = pressed(XINPUT_GAMEPAD_LEFT_SHOULDER);
        out.shoulderRight = pressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);
        const int threshold = 18000;
        if (state.Gamepad.sThumbLY > threshold && previousLY_ <= threshold) out.up = true;
        if (state.Gamepad.sThumbLY < -threshold && previousLY_ >= -threshold) out.down = true;
        if (state.Gamepad.sThumbLX > threshold && previousLX_ <= threshold) out.right = true;
        if (state.Gamepad.sThumbLX < -threshold && previousLX_ >= -threshold) out.left = true;
        previousButtons_ = b;
        previousLX_ = state.Gamepad.sThumbLX;
        previousLY_ = state.Gamepad.sThumbLY;
    }
    out.up |= (GetAsyncKeyState(VK_UP) & 1) != 0;
    out.down |= (GetAsyncKeyState(VK_DOWN) & 1) != 0;
    out.left |= (GetAsyncKeyState(VK_LEFT) & 1) != 0;
    out.right |= (GetAsyncKeyState(VK_RIGHT) & 1) != 0;
    out.select |= (GetAsyncKeyState(VK_RETURN) & 1) != 0;
    out.action |= (GetAsyncKeyState('X') & 1) != 0;
    out.back |= (GetAsyncKeyState(VK_ESCAPE) & 1) != 0;
    out.shoulderLeft |= (GetAsyncKeyState(VK_PRIOR) & 1) != 0;
    out.shoulderRight |= (GetAsyncKeyState(VK_NEXT) & 1) != 0;
    return out;
}
}
