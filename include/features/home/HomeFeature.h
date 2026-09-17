#pragma once

#include "GameRegistry.h"
#include "IdentityProvider.h"
#include "Settings.h"
#include "v5/ProductionRuntime.h"
#include <cstddef>
#include <string>
#include <vector>

namespace zero::features::home {

struct HomeCardViewModel {
    std::string packageId;
    std::string title;
    std::string version;
    std::string lastPlayedAtUtc;
    std::uint64_t playtimeSeconds{0};
    bool resumeAvailable{false};
};

struct HomeViewModel {
    std::string greetingName{"Player"};
    bool empty{true};
    std::size_t selectedIndex{0};
    std::vector<HomeCardViewModel> games;
};

class HomeController final {
public:
    static HomeViewModel Build(const GameRegistry& registry,
                               const ProductionRuntime& runtime,
                               const IdentityProfile& profile,
                               const UserSettings& settings,
                               std::size_t selectedIndex);
};

// HomeView is the presentation contract. Rendering backends consume only the
// immutable HomeViewModel; they do not query services or mutate product state.
class HomeView final {
public:
    static std::string PrimaryActionLabel(const HomeViewModel& model);
};

} // namespace zero::features::home
