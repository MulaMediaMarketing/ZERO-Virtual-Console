#include "v5/AchievementProfileDomain.h"
#include <iostream>

using namespace zero::v5;

int main() {
    AchievementAuthority achievements;
    std::string error;
    AchievementDefinition definition{"ach-1", "content-1", "First Step", 100, AuthoritySource::ZeroService,
                                     "Complete your first verified objective.", "icon-first-step", false, 1};
    if (!achievements.ApplyDefinition(definition, error)) return 10;

    const auto loadedDefinition = achievements.Definition("content-1", "ach-1");
    if (!loadedDefinition || loadedDefinition->description != definition.description ||
        loadedDefinition->iconId != definition.iconId || loadedDefinition->secret) return 11;
    if (achievements.DefinitionsFor("content-1").size() != 1) return 12;

    AchievementUnlock unlock{"ach-1", "content-1", "acct-1", 1000, 420, AuthoritySource::ZeroService};
    if (!achievements.ApplyUnlock(unlock, error)) return 13;
    if (achievements.ScoreFor("acct-1") != 100) return 14;
    if (achievements.UnlocksFor("acct-1", "content-1").size() != 1) return 15;

    auto forged = unlock;
    forged.achievementId = "ach-2";
    forged.authority = AuthoritySource::LocalPackage;
    if (achievements.ApplyUnlock(forged, error)) return 16;

    auto missingDefinition = unlock;
    missingDefinition.achievementId = "unknown";
    if (achievements.ApplyUnlock(missingDefinition, error)) return 17;

    auto invalidTarget = definition;
    invalidTarget.achievementId = "ach-invalid-target";
    invalidTarget.targetProgress = 0;
    if (achievements.ApplyDefinition(invalidTarget, error)) return 18;

    ProfileAuthority profiles;
    ProfileSnapshot profile{"acct-1", "ZERO_KING", "avatar-1", "banner-1", 48, 100, 4200, 1, true,
                            AuthoritySource::ZeroService, "In game", "Ready to play"};
    if (!profiles.Apply(profile, error)) return 20;
    const auto current = profiles.Current("acct-1");
    if (!current || current->level != 48 || current->achievementCount != 1 ||
        current->presence != "In game" || current->statusMessage != "Ready to play") return 21;

    auto forgedProfile = profile;
    forgedProfile.accountId = "acct-2";
    forgedProfile.authority = AuthoritySource::LocalPackage;
    if (profiles.Apply(forgedProfile, error)) return 22;

    std::cout << "ZERO V5 Achievements + Profile acceptance: PASS\n";
    return 0;
}
