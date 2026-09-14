#pragma once
#include <array>
#include <chrono>
#include <cstdint>

namespace zero {

struct InputSnapshot {
    bool up{};
    bool down{};
    bool left{};
    bool right{};
    bool select{};
    bool action{};
    bool back{};
    bool menu{};
    bool shoulderLeft{};
    bool shoulderRight{};
};

struct ControllerInputSample {
    bool connected{};
    bool dpadUp{};
    bool dpadDown{};
    bool dpadLeft{};
    bool dpadRight{};
    bool select{};
    bool action{};
    bool back{};
    bool menu{};
    bool shoulderLeft{};
    bool shoulderRight{};
    int16_t leftX{};
    int16_t leftY{};
};

struct KeyboardInputSample {
    bool upHeld{};
    bool downHeld{};
    bool leftHeld{};
    bool rightHeld{};
    bool selectPressed{};
    bool actionPressed{};
    bool backPressed{};
    bool menuPressed{};
    bool shoulderLeftPressed{};
    bool shoulderRightPressed{};
};

class InputCore {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr int kAnalogEngage = 18000;
    static constexpr int kAnalogRelease = 12000;

    InputSnapshot Update(const std::array<ControllerInputSample, 4>& controllers,
                         const KeyboardInputSample& keyboard,
                         Clock::time_point now);

    int ActiveController() const noexcept { return activeController_; }
    bool ControllerConnected() const noexcept { return activeController_ >= 0; }

private:
    enum class NavDirection { None, Up, Down, Left, Right };
    struct AnalogLatch { bool up{}; bool down{}; bool left{}; bool right{}; };

    static bool HasActivity(const ControllerInputSample& sample) noexcept;
    static bool DirectionHeld(NavDirection direction, bool up, bool down, bool left, bool right) noexcept;
    void UpdateAnalogLatch(size_t index, const ControllerInputSample& sample) noexcept;
    NavDirection ResolveDirection(bool up, bool down, bool left, bool right,
                                  const ControllerInputSample* controller) const noexcept;
    bool RepeatNavigation(NavDirection direction, Clock::time_point now);

    std::array<ControllerInputSample, 4> previous_{};
    std::array<AnalogLatch, 4> analog_{};
    int activeController_{-1};
    NavDirection repeatingDirection_{NavDirection::None};
    Clock::time_point repeatStartedAt_{};
    Clock::time_point lastRepeatAt_{};
};

} // namespace zero
