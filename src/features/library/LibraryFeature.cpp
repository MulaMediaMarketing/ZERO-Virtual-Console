#include "features/library/LibraryFeature.h"
#include <algorithm>

namespace zero::features::library {

LibraryViewModel LibraryController::Build(const GameRegistry& registry,
                                          const ProductionRuntime& runtime,
                                          std::size_t selectedIndex) {
    LibraryViewModel model;
    const auto& games = registry.Games();
    model.empty = games.empty();
    if (games.empty()) return model;

    model.selectedIndex = std::min(selectedIndex, games.size() - 1);
    model.items.reserve(games.size());
    for (std::size_t i = 0; i < games.size(); ++i) {
        const auto& game = games[i];
        const auto state = runtime.PlatformState(game.packageId);
        LibraryItemViewModel item;
        item.packageId = game.packageId;
        item.title = game.title;
        item.version = game.version;
        item.playtimeSeconds = state.totalPlaytimeSeconds;
        item.launchCount = state.launchCount;
        item.selected = i == model.selectedIndex;
        item.resumeAvailable = game.zeroResume && runtime.Resume(game.packageId).has_value();
        model.items.push_back(std::move(item));
    }
    return model;
}

bool LibraryController::MoveSelection(LibraryViewModel& model, int direction) noexcept {
    if (model.items.empty() || direction == 0) return false;
    const auto before = model.selectedIndex;
    if (direction < 0 && model.selectedIndex > 0) --model.selectedIndex;
    if (direction > 0 && model.selectedIndex + 1 < model.items.size()) ++model.selectedIndex;
    if (before == model.selectedIndex) return false;
    for (std::size_t i = 0; i < model.items.size(); ++i) model.items[i].selected = i == model.selectedIndex;
    return true;
}

std::string LibraryView::Summary(const LibraryViewModel& model) {
    return std::to_string(model.items.size()) + " INSTALLED";
}

} // namespace zero::features::library
