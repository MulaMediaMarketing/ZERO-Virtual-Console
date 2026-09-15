#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace zero::v5 {

enum class PresenceState : std::uint8_t { Offline, Online, InGame, InParty };

enum class FriendshipState : std::uint8_t { None, PendingIncoming, PendingOutgoing, Friends, Blocked };

struct FriendRecord {
    std::string accountId;
    std::string displayName;
    FriendshipState relationship{FriendshipState::None};
    PresenceState presence{PresenceState::Offline};
    std::string contentId;
    bool joinable{false};
    AuthoritySource authority{AuthoritySource::None};
};

struct PartyMember {
    std::string accountId;
    bool leader{false};
    bool ready{false};
};

struct PartySnapshot {
    std::string partyId;
    std::vector<PartyMember> members;
    std::string joinContentId;
    AuthoritySource authority{AuthoritySource::None};
};

struct JoinAuthorization {
    std::string partyId;
    std::string accountId;
    std::string contentId;
    std::string joinToken;
    std::uint64_t expiresAtEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

class SocialAuthority {
public:
    bool ApplyFriendRecord(FriendRecord record, std::string& error);
    std::optional<FriendRecord> Friend(const std::string& accountId) const;
    std::vector<FriendRecord> Friends() const;
    bool ApplyParty(PartySnapshot party, const std::string& localAccountId, std::string& error);
    std::optional<PartySnapshot> Party() const { return party_; }
    static bool ValidateJoin(const JoinAuthorization& authorization,
                             const PartySnapshot& party,
                             std::uint64_t nowEpochSeconds,
                             std::string& error);

private:
    std::unordered_map<std::string, FriendRecord> friends_;
    std::optional<PartySnapshot> party_;
};

} // namespace zero::v5
