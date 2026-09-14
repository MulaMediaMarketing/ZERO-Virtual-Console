#include "Input.h"
#include <windows.h>
#include <Xinput.h>
#include <array>

namespace zero {

InputSnapshot Input::Poll() {
    std::array<ControllerInputSample, 4> controllers{};
    for (DWORD i = 0; i < controllers.size(); ++i) {
        XINPUT_STATE state{};
        if (XInputGetState(i, &state) != ERROR_SUCCESS) continue;
        auto& out = controllers[i];
        out.connected = true;
        const WORD b = state.Gamepad.wButtons;
        out.dpadUp = (b & XINPUT_GAMEPAD_DPAD_UP) != 0;
        out.dpadDown = (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        out.dpadLeft = (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        out.dpadRight = (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
        out.select = (b & XINPUT_GAMEPAD_A) != 0;
        out.action = (b & XINPUT_GAMEPAD_X) != 0;
        out.back = (b & XINPUT_GAMEPAD_B) != 0;
        out.menu = (b & XINPUT_GAMEPAD_START) != 0;
        out.shoulderLeft = (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        out.shoulderRight = (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        out.leftX = state.Gamepad.sThumbLX;
        out.leftY = state.Gamepad.sThumbLY;
    }

    KeyboardInputSample keyboard{};
    keyboard.upHeld = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
    keyboard.downHeld = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
    keyboard.leftHeld = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
    keyboard.rightHeld = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
    keyboard.selectPressed = (GetAsyncKeyState(VK_RETURN) & 1) != 0;
    keyboard.actionPressed = (GetAsyncKeyState('X') & 1) != 0;
    keyboard.backPressed = (GetAsyncKeyState(VK_ESCAPE) & 1) != 0;
    keyboard.menuPressed = (GetAsyncKeyState(VK_F1) & 1) != 0;
    keyboard.shoulderLeftPressed = (GetAsyncKeyState(VK_PRIOR) & 1) != 0;
    keyboard.shoulderRightPressed = (GetAsyncKeyState(VK_NEXT) & 1) != 0;

    return core_.Update(controllers, keyboard, InputCore::Clock::now());
}

} // namespace zero
