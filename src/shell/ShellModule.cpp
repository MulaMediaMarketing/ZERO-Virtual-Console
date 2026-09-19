#include "shell/ShellModule.h"

namespace zero::shell {

ShellModule::ShellModule(v5::ProductionShellIntegration& navigation) noexcept
    : navigation_(navigation) {}

ShellIntent ShellModule::RouteGlobalInput(const InputSnapshot& input,
                                          bool runtimeActive,
                                          bool overlayVisible) const noexcept {
    if (runtimeActive && input.menu) return {ShellIntentType::ToggleOverlay, 0};
    if (overlayVisible && (input.menu || input.back)) return {ShellIntentType::ToggleOverlay, 0};
    if (input.shoulderLeft) return {ShellIntentType::MoveTopLevel, -1};
    if (input.shoulderRight) return {ShellIntentType::MoveTopLevel, 1};
    if (input.back) return {ShellIntentType::Back, 0};
    return {};
}

bool ShellModule::MoveTopLevel(int direction, ProductionUxPage& page, std::string& error) {
    if (!navigation_.MoveTopLevel(direction, error)) return false;
    const auto active = navigation_.ActivePage();
    if (!active) {
        error = "ShellKernel did not expose an active production page after navigation";
        return false;
    }
    page = *active;
    return true;
}

bool ShellModule::Back(ProductionUxPage& page, std::string& error) {
    if (!navigation_.Back(error)) return false;
    const auto active = navigation_.ActivePage();
    if (!active) {
        error = "ShellKernel did not expose an active production page after Back";
        return false;
    }
    page = *active;
    return true;
}

} // namespace zero::shell
