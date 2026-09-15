#include "v5/ProductionRuntime.h"
#include <utility>

namespace zero {

bool ProductionRuntime::ApplyAuthoritativeAchievementProgress(v5::AchievementProgress progress,
                                                              std::string& error) {
    return authoritativeAchievements_.ApplyProgress(std::move(progress), error);
}

std::optional<v5::AchievementProgress> ProductionRuntime::AuthoritativeAchievementProgress(
    const std::string& accountId,
    const std::string& packageId,
    const std::string& achievementId) const {
    return authoritativeAchievements_.ProgressFor(accountId, packageId, achievementId);
}

} // namespace zero
