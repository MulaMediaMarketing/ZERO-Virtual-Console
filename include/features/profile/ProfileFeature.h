#pragma once

#include "GameRegistry.h"
#include "IdentityProvider.h"
#include "Settings.h"
#include "v5/ProductionRuntime.h"
#include <cstdint>
#include <string>
#include <vector>

namespace zero::features::profile {

struct RecentActivityViewModel {
    std::string packageId;
    std::string title;
    std::string lastPlayedAtUtc;
};

struct ProfileViewModel {
    IdentityProfile identity;
    std::size_t gameCount{0};
    std::size_t achievementCount{0};
    std::uint64_t playtimeSeconds{0};
    std::uint64_t launchCount{0};
    bool shareActivity{false};
    bool shareAchievements{false};
    bool sharePlaytime{false};
    std::vector<RecentActivityViewModel> recent;
};

class ProfileController final {
public:
    static ProfileViewModel Build(const IIdentityProvider& identity,
                                  const GameRegistry& registry,
                                  const ProductionRuntime& runtime,
                                  const UserSettings& settings);
};

class ProfileView final {
public:
    static std::string PrivacySummary(const ProfileViewModel& model);
};

} // namespace zero::features::profile
