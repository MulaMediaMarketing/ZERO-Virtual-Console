#pragma once

#include "IdentityProvider.h"
#include "v5/AchievementProfileDomain.h"
#include "v5/ProductionRuntime.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace zero::features::achievements {

struct AchievementRowViewModel {
    std::string achievementId;
    std::string title;
    std::string description;
    std::string iconId;
    std::uint32_t score{0};
    std::uint32_t currentProgress{0};
    std::uint32_t targetProgress{1};
    std::uint32_t rarityBasisPoints{0};
    bool secret{false};
    bool unlocked{false};
};

struct AchievementsViewModel {
    std::string packageId;
    std::string accountId;
    std::uint64_t zeroScore{0};
    std::vector<AchievementRowViewModel> rows;
};

class AchievementsController final {
public:
    static AchievementsViewModel Build(const ProductionRuntime& runtime,
                                       const IdentityProfile& identity,
                                       const std::string& packageId);
};

class AchievementsView final {
public:
    static std::string ProgressLabel(const AchievementRowViewModel& row);
};

} // namespace zero::features::achievements
