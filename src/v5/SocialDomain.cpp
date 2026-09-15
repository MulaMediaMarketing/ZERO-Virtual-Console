#include "v5/SocialDomain.h"
#include <algorithm>

namespace zero::v5 {

bool SocialAuthority::ApplyFriendRecord(FriendRecord record, std::string& error) {
    if (record.authority != AuthoritySource::ZeroService) {
        error = "friend record was not issued by ZERO social authority";
        return false;
    }
    if (record.accountId.empty() || record.displayName.empty()) {
        error = "friend record is missing account identity";
        return false;
    }
    if (record.relationship == FriendshipState::Blocked) {
        record.joinable = false;
        record.contentId.clear();
        record.presence = PresenceState::Offline;
    }
    if (record.relationship != FriendshipState::Friends) record.joinable = false;
    if (record.joinable && record.contentId.empty()) {
        error = "joinable presence requires content identity";
        return false;
    }
    friends_[record.accountId] = std::move(record);
    return true;
}

std::optional<FriendRecord> SocialAuthority::Friend(const std::string& accountId) const {
    const auto it = friends_.find(accountId);
    return it == friends_.end() ? std::nullopt : std::optional<FriendRecord>{it->second};
}

std::vector<FriendRecord> SocialAuthority::Friends() const {
    std::vector<FriendRecord> result;
    for (const auto& [_, record] : friends_) {
        if (record.relationship == FriendshipState::Friends) result.push_back(record);
    }
    std::stable_sort(result.begin(), result.end(), [](const FriendRecord& a, const FriendRecord& b) {
        if (a.presence != b.presence) return static_cast<unsigned>(a.presence) > static_cast<unsigned>(b.presence);
        return a.displayName < b.displayName;
    });
    return result;
}

bool SocialAuthority::ApplyParty(PartySnapshot party, const std::string& localAccountId, std::string& error) {
    if (party.authority != AuthoritySource::ZeroService || party.partyId.empty() || localAccountId.empty()) {
        error = "party snapshot is not authoritative or identity is missing";
        return false;
    }
    if (party.members.empty()) {
        error = "party snapshot contains no members";
        return false;
    }
    std::unordered_set<std::string> unique;
    size_t leaders = 0;
    bool localPresent = false;
    for (const auto& member : party.members) {
        if (member.accountId.empty() || !unique.insert(member.accountId).second) {
            error = "party contains invalid or duplicate member identity";
            return false;
        }
        if (member.leader) ++leaders;
        if (member.accountId == localAccountId) localPresent = true;
    }
    if (leaders != 1 || !localPresent) {
        error = "party must contain one leader and the local account";
        return false;
    }
    party_ = std::move(party);
    return true;
}

bool SocialAuthority::ValidateJoin(const JoinAuthorization& authorization,
                                   const PartySnapshot& party,
                                   std::uint64_t nowEpochSeconds,
                                   std::string& error) {
    if (authorization.authority != AuthoritySource::ZeroService || party.authority != AuthoritySource::ZeroService) {
        error = "join authorization or party is not server authoritative";
        return false;
    }
    if (authorization.partyId != party.partyId || authorization.partyId.empty() ||
        authorization.accountId.empty() || authorization.contentId.empty() ||
        authorization.joinToken.empty()) {
        error = "join authorization identity is invalid";
        return false;
    }
    if (authorization.expiresAtEpochSeconds <= nowEpochSeconds) {
        error = "join authorization expired";
        return false;
    }
    if (!party.joinContentId.empty() && authorization.contentId != party.joinContentId) {
        error = "join authorization content does not match party target";
        return false;
    }
    const bool member = std::any_of(party.members.begin(), party.members.end(), [&](const PartyMember& item) {
        return item.accountId == authorization.accountId;
    });
    if (!member) {
        error = "join authorization account is not a party member";
        return false;
    }
    return true;
}

} // namespace zero::v5
