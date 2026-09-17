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
