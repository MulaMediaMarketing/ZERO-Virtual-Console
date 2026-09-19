#include "features/friends/FriendsFeature.h"
#include <algorithm>

namespace zero::features::friends {

FriendsViewModel FriendsController::Build(const IFriendsProvider& provider,
                                          const FriendsExperienceState& state) {
    FriendsViewModel model;
    model.providerState = provider.State();
    model.mode = state.mode;
    const auto& friends = provider.Friends();
    model.selectedIndex = friends.empty() ? 0 : std::min(state.selectedIndex, friends.size() - 1);
    model.rows.reserve(friends.size());
    for (std::size_t i = 0; i < friends.size(); ++i) {
        const auto& friendState = friends[i];
        model.rows.push_back(FriendRowViewModel{
            friendState.zeroId,
            friendState.displayName,
            friendState.gamePackageId,
            friendState.presence,
            friendState.joinable,
            i == model.selectedIndex
        });
    }
    return model;
}

bool FriendsController::MoveSelection(FriendsViewModel& model, int direction) noexcept {
    if (model.rows.empty() || direction == 0) return false;
    const auto before = model.selectedIndex;
    if (direction < 0 && model.selectedIndex > 0) --model.selectedIndex;
    if (direction > 0 && model.selectedIndex + 1 < model.rows.size()) ++model.selectedIndex;
    if (before == model.selectedIndex) return false;
    for (std::size_t i = 0; i < model.rows.size(); ++i) model.rows[i].selected = i == model.selectedIndex;
    return true;
}

std::string FriendsView::PresenceLabel(PresenceState state) {
    switch (state) {
        case PresenceState::Offline: return "OFFLINE";
        case PresenceState::Online: return "ONLINE";
        case PresenceState::InGame: return "IN GAME";
    }
    return "OFFLINE";
}

} // namespace zero::features::friends
