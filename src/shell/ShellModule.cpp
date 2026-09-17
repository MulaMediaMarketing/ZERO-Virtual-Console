#include "shell/ShellModule.h"

namespace zero::shell {

ShellModule::ShellModule(v5::ProductionShellIntegration& navigation,
                         GameRegistry& registry,
                         CaptureLibrary& captures,
                         IFriendsProvider& friends,
                         IStoreProvider& store) noexcept
    : navigation_(navigation),
      registry_(registry),
      captures_(captures),
      friends_(friends),
      store_(store) {}

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

void ShellModule::RefreshAll() {
    registry_.Refresh();
    captures_.Refresh();
    friends_.Refresh();
    store_.Refresh();
}

void ShellModule::RefreshForPage(ProductionUxPage page) {
    switch (page) {
        case ProductionUxPage::Captures:
            captures_.Refresh();
            break;
        case ProductionUxPage::Friends:
            friends_.Refresh();
            break;
        case ProductionUxPage::Store:
        case ProductionUxPage::Wishlist:
        case ProductionUxPage::Checkout:
            store_.Refresh();
            break;
        case ProductionUxPage::Home:
        case ProductionUxPage::Discover:
        case ProductionUxPage::Library:
        case ProductionUxPage::GameDetail:
        case ProductionUxPage::Import:
            registry_.Refresh();
            break;
        default:
            break;
    }
}

} // namespace zero::shell
