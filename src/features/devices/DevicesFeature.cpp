#include "features/devices/DevicesFeature.h"

namespace zero::features::devices {

DevicesViewModel DevicesController::Build(const Input& input) noexcept {
    DevicesViewModel model;
    model.controllerConnected = input.ControllerConnected();
    model.controllerSlot = input.ActiveController();
    return model;
}

std::string DevicesView::ControllerStatus(const DevicesViewModel& model) {
    if (!model.controllerConnected) return "OFFLINE";
    return "ONLINE";
}

} // namespace zero::features::devices
