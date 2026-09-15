#include "v5/AchievementProfileDomain.h"
#include <iostream>

using namespace zero::v5;

int main() {
    AchievementAuthority achievements;
    std::string error;
    AchievementDefinition definition{"ach-1", "content-1", "First Step", 100, AuthoritySource::ZeroService};
    if (!achievements.ApplyDefinition(definition, error)) return 10;

    AchievementUnlock unlock{"ach-1", "content-1", "acct-1", 1000, 420, AuthoritySource::ZeroService};
    if (!achievements.ApplyUnlock(unlock, error)) return 11;
    if (achievements.ScoreFor("acct-1") != 100) return 12;
    if (achievements.UnlocksFor("acct-1", "content-1").size() != 1) return 13;

    auto forged = unlock;
    forged.achievementId = "ach-2";
    forged.authority = AuthoritySource::LocalPackage;
    if (achievements.ApplyUnlock(forged, error)) return 14;

    auto missingDefinition = unlock;
    missingDefinition.achievementId = "unknown";
    if (achievements.ApplyUnlock(missingDefinition, error)) return 15;

    ProfileAuthority profiles;
    ProfileSnapshot profile{"acct-1", "ZERO_KING", "avatar-1", "banner-1", 48, 100, 4200, 1, true,
                            AuthoritySource::ZeroService};
    if (!profiles.Apply(profile, error)) return 20;
    const auto current = profiles.Current("acct-1");
    if (!current || current->level != 48 || current->achievementCount != 1) return 21;

    auto forgedProfile = profile;
    forgedProfile.accountId = "acct-2";
    forgedProfile.authority = AuthoritySource::LocalPackage;
    if (profiles.Apply(forgedProfile, error)) return 22;

    std::cout << "ZERO V5 Achievements + Profile acceptance: PASS\n";
    return 0;
}
