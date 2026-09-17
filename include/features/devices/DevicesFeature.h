#pragma once

#include "Input.h"
#include <string>

namespace zero::features::devices {

struct DevicesViewModel {
    bool localPcAvailable{true};
    bool controllerConnected{false};
    int controllerSlot{-1};
    bool remoteDeviceServiceConnected{false};
};

class DevicesController final {
public:
    static DevicesViewModel Build(const Input& input) noexcept;
};

class DevicesView final {
public:
    static std::string ControllerStatus(const DevicesViewModel& model);
};

} // namespace zero::features::devices
