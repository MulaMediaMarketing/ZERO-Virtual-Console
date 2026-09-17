#pragma once

#include "StoreProvider.h"
#include "StoreSettingsFirstBootExperience.h"
#include <cstddef>
#include <string>
#include <vector>

namespace zero::features::store {

struct StoreItemViewModel {
    std::string productId;
    std::string packageId;
    std::string title;
    std::string version;
    std::string description;
    std::string price;
    bool owned{false};
    bool purchasable{false};
    bool selected{false};
};

struct StoreViewModel {
    StoreProviderState providerState{StoreProviderState::Disconnected};
    StoreExperienceMode mode{StoreExperienceMode::Disconnected};
    std::size_t selectedIndex{0};
    std::size_t scrollOffset{0};
    std::size_t visibleRows{5};
    std::vector<StoreItemViewModel> items;
};

class StoreController final {
public:
    static StoreViewModel Build(const IStoreProvider& provider,
                                const StoreExperienceState& state);
    static bool MoveSelection(StoreViewModel& model, int direction) noexcept;
};

class StoreView final {
public:
    static std::string EmptyStateTitle(const StoreViewModel& model);
};

} // namespace zero::features::store
