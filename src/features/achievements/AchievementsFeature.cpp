#include "features/achievements/AchievementsFeature.h"
#include <algorithm>
#include <unordered_map>

namespace zero::features::achievements {

AchievementsViewModel AchievementsController::Build(const ProductionRuntime& runtime,
                                                     const IdentityProfile& identity,
                                                     const std::string& packageId) {
    AchievementsViewModel model;
    model.packageId = packageId;
    model.accountId = identity.zeroId;
    if (!model.accountId.empty()) model.zeroScore = runtime.AuthoritativeAchievementScore(model.accountId);

    const auto definitions = runtime.AchievementDefinitions(packageId);
    const auto unlocks = model.accountId.empty()
        ? std::vector<v5::AchievementUnlock>{}
        : runtime.AuthoritativeAchievementUnlocks(model.accountId, packageId);

    std::unordered_map<std::string, v5::AchievementUnlock> unlockById;
    for (const auto& unlock : unlocks) unlockById.emplace(unlock.achievementId, unlock);

    model.rows.reserve(definitions.size());
    for (const auto& definition : definitions) {
        AchievementRowViewModel row;
        row.achievementId = definition.achievementId;
        row.title = definition.title;
        row.description = definition.description;
        row.iconId = definition.iconId;
        row.score = definition.score;
        row.secret = definition.secret;
        row.targetProgress = std::max<std::uint32_t>(1u, definition.targetProgress);

        if (const auto it = unlockById.find(definition.achievementId); it != unlockById.end()) {
            row.unlocked = true;
            row.currentProgress = row.targetProgress;
            row.rarityBasisPoints = it->second.rarityBasisPoints;
        } else if (!model.accountId.empty()) {
            if (const auto progress = runtime.AuthoritativeAchievementProgress(model.accountId, packageId, definition.achievementId)) {
                row.currentProgress = std::min(progress->currentProgress, row.targetProgress);
            }
        }
        model.rows.push_back(std::move(row));
    }
    return model;
}

std::string AchievementsView::ProgressLabel(const AchievementRowViewModel& row) {
    if (row.unlocked) return "UNLOCKED";
    return std::to_string(row.currentProgress) + "/" + std::to_string(row.targetProgress);
}

} // namespace zero::features::achievements
