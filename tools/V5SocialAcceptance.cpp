#include "v5/SocialDomain.h"
#include <iostream>

using namespace zero::v5;

int main() {
    SocialAuthority social;
    std::string error;

    FriendRecord friendRecord{"friend-1", "PLAYER2", FriendshipState::Friends,
                              PresenceState::InGame, "content-1", true, AuthoritySource::ZeroService};
    if (!social.ApplyFriendRecord(friendRecord, error)) return 10;
    const auto friendView = social.Friend("friend-1");
    if (!friendView || !friendView->joinable) return 11;

    auto forged = friendRecord;
    forged.accountId = "friend-2";
    forged.authority = AuthoritySource::LocalPackage;
    if (social.ApplyFriendRecord(forged, error)) return 12;

    auto blocked = friendRecord;
    blocked.accountId = "blocked-1";
    blocked.relationship = FriendshipState::Blocked;
    if (!social.ApplyFriendRecord(blocked, error)) return 13;
    const auto blockedView = social.Friend("blocked-1");
    if (!blockedView || blockedView->joinable || blockedView->presence != PresenceState::Offline) return 14;

    PartySnapshot party{"party-1", {{"acct-1", true, true}, {"friend-1", false, true}},
                        "content-1", AuthoritySource::ZeroService};
    if (!social.ApplyParty(party, "acct-1", error)) return 20;

    JoinAuthorization join{"party-1", "acct-1", "content-1", "join-token", 2000, AuthoritySource::ZeroService};
    if (!SocialAuthority::ValidateJoin(join, party, 1000, error)) return 21;

    auto expired = join;
    expired.expiresAtEpochSeconds = 900;
    if (SocialAuthority::ValidateJoin(expired, party, 1000, error)) return 22;

    auto outsider = join;
    outsider.accountId = "outsider";
    if (SocialAuthority::ValidateJoin(outsider, party, 1000, error)) return 23;

    auto badParty = party;
    badParty.members.push_back({"leader-2", true, false});
    if (social.ApplyParty(badParty, "acct-1", error)) return 24;

    std::cout << "ZERO V5 Friends + Presence + Party acceptance: PASS\n";
    return 0;
}
