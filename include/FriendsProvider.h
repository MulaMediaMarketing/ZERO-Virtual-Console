#pragma once
#include <string>
#include <vector>

namespace zero {

enum class FriendsProviderState {
    Disconnected,
    Ready,
    Error
};

enum class PresenceState {
    Offline,
    Online,
    InGame
};

struct FriendPresence {
    std::string zeroId;
    std::string displayName;
    PresenceState presence{PresenceState::Offline};
    std::string gamePackageId;
    bool joinable{false};
};

class IFriendsProvider {
public:
    virtual ~IFriendsProvider() = default;
    virtual FriendsProviderState State() const noexcept = 0;
    virtual const std::vector<FriendPresence>& Friends() const noexcept = 0;
    virtual void Refresh() = 0;
};

class DisconnectedFriendsProvider final : public IFriendsProvider {
public:
    FriendsProviderState State() const noexcept override { return FriendsProviderState::Disconnected; }
    const std::vector<FriendPresence>& Friends() const noexcept override { return friends_; }
    void Refresh() override {}

private:
    std::vector<FriendPresence> friends_;
};

} // namespace zero
