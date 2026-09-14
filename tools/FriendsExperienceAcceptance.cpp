#include "FriendsExperience.h"
#include <iostream>
#include <string_view>
#include <vector>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}
}

int main() {
    using namespace zero;
    bool ok = true;

    ok &= expect(FriendsModeFor(FriendsProviderState::Disconnected, 0) == FriendsExperienceMode::Disconnected,
                 "disconnected provider must render disconnected mode");
    ok &= expect(FriendsModeFor(FriendsProviderState::Error, 0) == FriendsExperienceMode::Error,
                 "error provider must render error mode");
    ok &= expect(FriendsModeFor(FriendsProviderState::Ready, 0) == FriendsExperienceMode::Empty,
                 "ready provider with no records must render empty mode");
    ok &= expect(FriendsModeFor(FriendsProviderState::Ready, 1) == FriendsExperienceMode::Ready,
                 "ready provider with records must render ready mode");

    std::vector<FriendPresence> friends{
        {"zero-1", "Offline Friend", PresenceState::Offline, "", false},
        {"zero-2", "Online Friend", PresenceState::Online, "", false},
        {"zero-3", "In Game", PresenceState::InGame, "game.alpha", false},
        {"zero-4", "Joinable", PresenceState::InGame, "game.beta", true},
        {"zero-5", "Joinable Missing Game", PresenceState::InGame, "", true},
        {"zero-6", "More", PresenceState::Online, "", false},
    };

    ok &= expect(!FriendIsOnline(friends[0]), "offline friend must remain offline");
    ok &= expect(FriendIsOnline(friends[1]), "online friend must be online");
    ok &= expect(FriendIsOnline(friends[3]), "in-game friend must be online");
    ok &= expect(!FriendCanJoin(friends[2]), "in-game friend without joinable flag must not be joinable");
    ok &= expect(FriendCanJoin(friends[3]), "joinable in-game friend with package ID must be joinable");
    ok &= expect(!FriendCanJoin(friends[4]), "join action must fail closed when package ID is missing");

    FriendsExperienceState state{};
    state.visibleRows = 3;
    ClampFriendsExperience(state, FriendsProviderState::Ready, friends);
    ok &= expect(state.selectedIndex == 0 && state.scrollOffset == 0,
                 "initial ready state must select first friend without scrolling");

    ok &= expect(!MoveFriendSelectionUp(state, FriendsProviderState::Ready, friends),
                 "selection must clamp at first friend");
    ok &= expect(MoveFriendSelectionDown(state, FriendsProviderState::Ready, friends),
                 "selection must move down");
    ok &= expect(state.selectedIndex == 1, "selection must move to second friend");
    MoveFriendSelectionDown(state, FriendsProviderState::Ready, friends);
    MoveFriendSelectionDown(state, FriendsProviderState::Ready, friends);
    ok &= expect(state.selectedIndex == 3 && state.scrollOffset == 1,
                 "selection must scroll when moving beyond visible rows");

    const auto selected = SelectedFriend(state, FriendsProviderState::Ready, friends);
    ok &= expect(selected && selected->displayName == "Joinable", "selected friend must match selected index");
    ok &= expect(selected && FriendCanJoin(*selected), "selected joinable friend must expose join eligibility");

    state.selectedIndex = 999;
    ClampFriendsExperience(state, FriendsProviderState::Ready, friends);
    ok &= expect(state.selectedIndex == friends.size() - 1,
                 "selection must clamp after provider list changes");

    ClampFriendsExperience(state, FriendsProviderState::Disconnected, friends);
    ok &= expect(state.mode == FriendsExperienceMode::Disconnected && state.selectedIndex == 0 && state.scrollOffset == 0,
                 "disconnected transition must reset selection and scrolling");
    ok &= expect(SelectedFriend(state, FriendsProviderState::Disconnected, friends) == nullptr,
                 "disconnected experience must not expose stale selected friend");

    if (!ok) return 1;
    std::cout << "PASS: ZERO Friends experience contract\n";
    return 0;
}
