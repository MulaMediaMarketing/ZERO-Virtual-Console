#pragma once
#include <windows.h>
#include <chrono>

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

class Input {
public:
    InputSnapshot Poll();
private:
    enum class NavDirection { None, Up, Down, Left, Right };

    bool RepeatNavigation(NavDirection direction, bool held,
                          std::chrono::steady_clock::time_point now);

    WORD previousButtons_{0};
    SHORT previousLX_{0};
    SHORT previousLY_{0};
    NavDirection repeatingDirection_{NavDirection::None};
    std::chrono::steady_clock::time_point repeatStartedAt_{};
    std::chrono::steady_clock::time_point lastRepeatAt_{};
};
}
