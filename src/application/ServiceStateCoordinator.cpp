#include "application/ServiceStateCoordinator.h"

namespace zero::application {

ServiceStateCoordinator::ServiceStateCoordinator(GameRegistry& registry,
                                                 CaptureLibrary& captures,
                                                 IFriendsProvider& friends,
                                                 IStoreProvider& store,
                                                 ProductionRuntime& runtime) noexcept
    : registry_(registry), captures_(captures), friends_(friends), store_(store), runtime_(runtime) {}

void ServiceStateCoordinator::RefreshLocal() {
    registry_.Refresh();
    captures_.Refresh();
    ++revision_;
}

void ServiceStateCoordinator::RefreshNetworkAdapters() {
    friends_.Refresh();
    store_.Refresh();
    ++revision_;
}

void ServiceStateCoordinator::RefreshAll() {
    registry_.Refresh();
    captures_.Refresh();
    friends_.Refresh();
    store_.Refresh();
    ++revision_;
}

void ServiceStateCoordinator::RefreshForPage(ProductionUxPage page) {
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
    ++revision_;
}

void ServiceStateCoordinator::PollRuntime() {
    runtime_.Poll();
    ++revision_;
}

ServiceStateSnapshot ServiceStateCoordinator::Snapshot() const noexcept {
    return ServiceStateSnapshot{
        revision_,
        registry_.Games().size(),
        captures_.Items().size(),
        friends_.State(),
        store_.State(),
        runtime_.State()
    };
}

} // namespace zero::application
