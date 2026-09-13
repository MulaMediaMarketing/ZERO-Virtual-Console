#pragma once
#include <windows.h>

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
    WORD previousButtons_{0};
    SHORT previousLX_{0};
    SHORT previousLY_{0};
};
}
