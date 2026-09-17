#pragma once

#include "CaptureLibrary.h"
#include "FriendsProvider.h"
#include "GameRegistry.h"
#include "StoreProvider.h"
#include "v5/ProductionRuntime.h"
#include <cstdint>
#include <string>

namespace zero::application {

struct ServiceStateSnapshot {
    std::uint64_t revision{0};
    std::size_t installedGameCount{0};
    std::size_t captureCount{0};
    FriendsProviderState friendsState{FriendsProviderState::Disconnected};
    StoreProviderState storeState{StoreProviderState::Disconnected};
    RuntimeState runtimeState{RuntimeState::Idle};
};

// Central coordinator for refreshing and snapshotting long-lived service state.
// Feature views never independently invent refresh schedules or service truth.
class ServiceStateCoordinator final {
public:
    ServiceStateCoordinator(GameRegistry& registry,
                            CaptureLibrary& captures,
                            IFriendsProvider& friends,
                            IStoreProvider& store,
                            ProductionRuntime& runtime) noexcept;

    void RefreshLocal();
    void RefreshNetworkAdapters();
    void RefreshAll();
    void PollRuntime();

    ServiceStateSnapshot Snapshot() const noexcept;
    std::uint64_t Revision() const noexcept { return revision_; }

private:
    GameRegistry& registry_;
    CaptureLibrary& captures_;
    IFriendsProvider& friends_;
    IStoreProvider& store_;
    ProductionRuntime& runtime_;
    std::uint64_t revision_{0};
};

} // namespace zero::application
