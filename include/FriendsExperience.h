#pragma once

#include "FriendsProvider.h"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace zero {

enum class FriendsExperienceMode {
    Disconnected,
    Error,
    Empty,
    Ready
};

struct FriendsExperienceState {
    FriendsExperienceMode mode{FriendsExperienceMode::Disconnected};
    std::size_t selectedIndex{0};
    std::size_t scrollOffset{0};
    std::size_t visibleRows{5};
};

inline constexpr FriendsExperienceMode FriendsModeFor(FriendsProviderState providerState, std::size_t friendCount) noexcept {
    if (providerState == FriendsProviderState::Disconnected) return FriendsExperienceMode::Disconnected;
    if (providerState == FriendsProviderState::Error) return FriendsExperienceMode::Error;
    return friendCount == 0 ? FriendsExperienceMode::Empty : FriendsExperienceMode::Ready;
}

inline constexpr std::wstring_view FriendsModeTitle(FriendsExperienceMode mode) noexcept {
    switch (mode) {
        case FriendsExperienceMode::Disconnected: return L"Zero Link is not connected";
        case FriendsExperienceMode::Error: return L"Zero Link is unavailable";
        case FriendsExperienceMode::Empty: return L"No friends to show";
        case FriendsExperienceMode::Ready: return L"Friends";
    }
    return L"Friends";
}

inline constexpr bool FriendCanJoin(const FriendPresence& item) noexcept {
    return item.presence == PresenceState::InGame && item.joinable && !item.gamePackageId.empty();
}

inline constexpr bool FriendIsOnline(const FriendPresence& item) noexcept {
    return item.presence == PresenceState::Online || item.presence == PresenceState::InGame;
}

inline void ClampFriendsExperience(FriendsExperienceState& state, FriendsProviderState providerState,
                                   const std::vector<FriendPresence>& friends) noexcept {
    state.mode = FriendsModeFor(providerState, friends.size());
    if (state.visibleRows == 0) state.visibleRows = 1;

    if (state.mode != FriendsExperienceMode::Ready) {
        state.selectedIndex = 0;
        state.scrollOffset = 0;
        return;
    }

    if (state.selectedIndex >= friends.size()) state.selectedIndex = friends.size() - 1;
    if (state.selectedIndex < state.scrollOffset) state.scrollOffset = state.selectedIndex;
    if (state.selectedIndex >= state.scrollOffset + state.visibleRows)
        state.scrollOffset = state.selectedIndex - state.visibleRows + 1;

    const std::size_t maxStart = friends.size() > state.visibleRows ? friends.size() - state.visibleRows : 0;
    state.scrollOffset = std::min(state.scrollOffset, maxStart);
}

inline bool MoveFriendSelectionUp(FriendsExperienceState& state, FriendsProviderState providerState,
                                  const std::vector<FriendPresence>& friends) noexcept {
    ClampFriendsExperience(state, providerState, friends);
    if (state.mode != FriendsExperienceMode::Ready || state.selectedIndex == 0) return false;
    --state.selectedIndex;
    ClampFriendsExperience(state, providerState, friends);
    return true;
}

inline bool MoveFriendSelectionDown(FriendsExperienceState& state, FriendsProviderState providerState,
                                    const std::vector<FriendPresence>& friends) noexcept {
    ClampFriendsExperience(state, providerState, friends);
    if (state.mode != FriendsExperienceMode::Ready || state.selectedIndex + 1 >= friends.size()) return false;
    ++state.selectedIndex;
    ClampFriendsExperience(state, providerState, friends);
    return true;
}

inline std::optional<std::size_t> SelectedFriendIndex(const FriendsExperienceState& state,
                                                      FriendsProviderState providerState,
                                                      const std::vector<FriendPresence>& friends) noexcept {
    if (FriendsModeFor(providerState, friends.size()) != FriendsExperienceMode::Ready) return std::nullopt;
    if (state.selectedIndex >= friends.size()) return std::nullopt;
    return state.selectedIndex;
}

inline const FriendPresence* SelectedFriend(const FriendsExperienceState& state,
                                            FriendsProviderState providerState,
                                            const std::vector<FriendPresence>& friends) noexcept {
    const auto index = SelectedFriendIndex(state, providerState, friends);
    return index ? &friends[*index] : nullptr;
}

} // namespace zero
