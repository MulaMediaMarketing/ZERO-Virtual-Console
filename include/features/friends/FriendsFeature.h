#pragma once

#include "FriendsExperience.h"
#include "FriendsProvider.h"
#include <cstddef>
#include <string>
#include <vector>

namespace zero::features::friends {

struct FriendRowViewModel {
    std::string zeroId;
    std::string displayName;
    std::string gamePackageId;
    PresenceState presence{PresenceState::Offline};
    bool joinable{false};
    bool selected{false};
};

struct FriendsViewModel {
    FriendsProviderState providerState{FriendsProviderState::Disconnected};
    FriendsExperienceMode mode{FriendsExperienceMode::Disconnected};
    std::size_t selectedIndex{0};
    std::vector<FriendRowViewModel> rows;
};

class FriendsController final {
public:
    static FriendsViewModel Build(const IFriendsProvider& provider,
                                  const FriendsExperienceState& state);
    static bool MoveSelection(FriendsViewModel& model, int direction) noexcept;
};

class FriendsView final {
public:
    static std::string PresenceLabel(PresenceState state);
};

} // namespace zero::features::friends
