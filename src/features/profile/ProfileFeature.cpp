#include "features/profile/ProfileFeature.h"
#include <algorithm>

namespace zero::features::profile {

ProfileViewModel ProfileController::Build(const IIdentityProvider& identity,
                                          const GameRegistry& registry,
                                          const ProductionRuntime& runtime,
                                          const UserSettings& settings) {
    ProfileViewModel model;
    model.identity = identity.CurrentProfile();
    model.gameCount = registry.Games().size();
    model.shareActivity = settings.shareActivity;
    model.shareAchievements = settings.shareAchievements;
    model.sharePlaytime = settings.sharePlaytime;

    std::vector<RecentActivityViewModel> recent;
    recent.reserve(registry.Games().size());
    for (const auto& game : registry.Games()) {
        const auto state = runtime.PlatformState(game.packageId);
        model.playtimeSeconds += state.totalPlaytimeSeconds;
        model.launchCount += state.launchCount;
        model.achievementCount += runtime.Achievements(game.packageId).size();
        recent.push_back(RecentActivityViewModel{game.packageId, game.title, state.lastPlayedAtUtc});
    }

    std::stable_sort(recent.begin(), recent.end(), [](const auto& a, const auto& b) {
        return a.lastPlayedAtUtc > b.lastPlayedAtUtc;
    });
    if (recent.size() > 3) recent.resize(3);
    model.recent = std::move(recent);
    return model;
}

std::string ProfileView::PrivacySummary(const ProfileViewModel& model) {
    if (!model.shareActivity && !model.shareAchievements && !model.sharePlaytime) return "PRIVATE";
    if (model.shareActivity && model.shareAchievements && model.sharePlaytime) return "SHARING ENABLED";
    return "CUSTOM PRIVACY";
}

} // namespace zero::features::profile
