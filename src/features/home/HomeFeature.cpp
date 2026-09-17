#include "features/home/HomeFeature.h"
#include <algorithm>

namespace zero::features::home {

HomeViewModel HomeController::Build(const GameRegistry& registry,
                                    const ProductionRuntime& runtime,
                                    const IdentityProfile& profile,
                                    const UserSettings& settings,
                                    std::size_t selectedIndex) {
    HomeViewModel model;
    model.greetingName = profile.displayName.empty() ? settings.profileName : profile.displayName;
    if (model.greetingName.empty()) model.greetingName = "Player";

    const auto& games = registry.Games();
    model.empty = games.empty();
    if (games.empty()) return model;

    model.selectedIndex = std::min(selectedIndex, games.size() - 1);
    model.games.reserve(games.size());
    for (const auto& game : games) {
        const auto state = runtime.PlatformState(game.packageId);
        HomeCardViewModel card;
        card.packageId = game.packageId;
        card.title = game.title;
        card.version = game.version;
        card.lastPlayedAtUtc = state.lastPlayedAtUtc;
        card.playtimeSeconds = state.totalPlaytimeSeconds;
        card.resumeAvailable = game.zeroResume && runtime.Resume(game.packageId).has_value();
        model.games.push_back(std::move(card));
    }
    return model;
}

std::string HomeView::PrimaryActionLabel(const HomeViewModel& model) {
    if (model.empty || model.games.empty()) return {};
    const auto index = std::min(model.selectedIndex, model.games.size() - 1);
    return model.games[index].resumeAvailable ? "RESUME" : "PLAY";
}

} // namespace zero::features::home
