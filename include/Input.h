#pragma once
#include "InputCore.h"

namespace zero {

class Input {
public:
    InputSnapshot Poll();
    int ActiveController() const noexcept { return core_.ActiveController(); }
    bool ControllerConnected() const noexcept { return core_.ControllerConnected(); }
private:
    InputCore core_;
};

} // namespace zero
