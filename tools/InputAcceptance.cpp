#include "InputCore.h"
#include <array>
#include <chrono>
#include <iostream>

namespace {
using zero::ControllerInputSample;
using zero::InputCore;
using zero::KeyboardInputSample;

void check(bool value, const char* name, bool& all) {
    std::cout << (value ? "[PASS] " : "[FAIL] ") << name << "\n";
    all &= value;
}
}

int main() {
    bool all = true;
    InputCore core;
    std::array<ControllerInputSample, 4> pads{};
    KeyboardInputSample keyboard{};
    auto t = InputCore::Clock::time_point{};

    // First connected controller is selected, but reconnect-held buttons do not
    // synthesize an activation edge.
    pads[0].connected = true;
    pads[0].select = true;
    auto out = core.Update(pads, keyboard, t);
    check(core.ActiveController() == 0, "controller_zero_selected", all);
    check(!out.select, "connect_held_button_suppressed", all);

    pads[0].select = false;
    t += std::chrono::milliseconds(16);
    core.Update(pads, keyboard, t);
    pads[0].select = true;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(out.select, "release_then_press_generates_edge", all);

    // Idle controller 0 must not permanently own the shell if controller 1 is used.
    pads[0].select = false;
    pads[1].connected = true;
    pads[1].dpadRight = true;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(core.ActiveController() == 1, "active_controller_transfers_on_activity", all);
    check(out.right, "new_controller_navigation_works", all);

    // Navigation repeat: first pulse immediately, no pulse before delay, then repeat.
    t += std::chrono::milliseconds(100);
    out = core.Update(pads, keyboard, t);
    check(!out.right, "repeat_waits_for_initial_delay", all);
    t += std::chrono::milliseconds(240);
    out = core.Update(pads, keyboard, t);
    check(out.right, "repeat_fires_after_initial_delay", all);
    t += std::chrono::milliseconds(40);
    out = core.Update(pads, keyboard, t);
    check(!out.right, "repeat_rate_limited", all);
    t += std::chrono::milliseconds(60);
    out = core.Update(pads, keyboard, t);
    check(out.right, "repeat_fires_at_cadence", all);

    // Analog hysteresis should engage above 18k, remain held through 12k-18k,
    // and release below 12k without focus chatter.
    pads[1].dpadRight = false;
    pads[1].leftX = 19000;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(out.right, "analog_engages_above_threshold", all);
    pads[1].leftX = 15000;
    t += std::chrono::milliseconds(100);
    out = core.Update(pads, keyboard, t);
    check(!out.right, "analog_hysteresis_holds_without_new_pulse", all);
    pads[1].leftX = 11000;
    t += std::chrono::milliseconds(16);
    core.Update(pads, keyboard, t);
    pads[1].leftX = 19000;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(out.right, "analog_reengages_after_release_threshold", all);

    // Opposing directions cancel rather than producing unstable focus movement.
    pads[1].leftX = 0;
    pads[1].dpadLeft = true;
    pads[1].dpadRight = true;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(!out.left && !out.right, "opposing_horizontal_inputs_cancel", all);

    // Dominant analog axis chooses a deterministic direction on diagonals.
    pads[1].dpadLeft = false;
    pads[1].dpadRight = false;
    pads[1].leftX = 23000;
    pads[1].leftY = 19000;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(out.right && !out.up, "dominant_analog_axis_wins_diagonal", all);

    // Disconnect clears ownership and repeat state. Reconnected held buttons remain suppressed.
    pads[1] = {};
    t += std::chrono::milliseconds(16);
    core.Update(pads, keyboard, t);
    check(core.ActiveController() == 0, "disconnect_falls_back_to_connected_controller", all);
    pads[0] = {};
    t += std::chrono::milliseconds(16);
    core.Update(pads, keyboard, t);
    check(!core.ControllerConnected(), "all_controllers_disconnected", all);
    pads[2].connected = true;
    pads[2].back = true;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(core.ActiveController() == 2, "reconnected_controller_claims_ownership", all);
    check(!out.back, "reconnect_held_back_does_not_escape", all);

    // Keyboard remains a valid fallback with no controller connected.
    pads[2] = {};
    keyboard.selectPressed = true;
    t += std::chrono::milliseconds(16);
    out = core.Update(pads, keyboard, t);
    check(out.select, "keyboard_fallback_select", all);

    std::cout << "Result: " << (all ? "PASS" : "FAIL") << "\n";
    return all ? 0 : 2;
}
