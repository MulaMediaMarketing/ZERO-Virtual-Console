#include "v5/AchievementProfileDomain.h"
#include <iostream>

using namespace zero::v5;

int main() {
    AchievementAuthority achievements;
    std::string error;
    AchievementDefinition definition{"ach-1", "content-1", "First Step", 100, AuthoritySource::ZeroService,
                                     "Complete your first verified objective.", "icon-first-step", false, 10};
    if (!achievements.ApplyDefinition(definition, error)) return 10;

    const auto loadedDefinition = achievements.Definition("content-1", "ach-1");
    if (!loadedDefinition || loadedDefinition->description != definition.description ||
        loadedDefinition->iconId != definition.iconId || loadedDefinition->secret ||
        loadedDefinition->targetProgress != 10) return 11;
    if (achievements.DefinitionsFor("content-1").size() != 1) return 12;

    AchievementProgress progress{"ach-1", "content-1", "acct-1", 4, 900, AuthoritySource::ZeroService};
    if (!achievements.ApplyProgress(progress, error)) return 13;
    const auto loadedProgress = achievements.ProgressFor("acct-1", "content-1", "ach-1");
    if (!loadedProgress || loadedProgress->currentProgress != 4) return 14;

    auto forgedProgress = progress;
    forgedProgress.authority = AuthoritySource::LocalPackage;
    if (achievements.ApplyProgress(forgedProgress, error)) return 15;

    auto excessiveProgress = progress;
    excessiveProgress.currentProgress = 11;
    if (achievements.ApplyProgress(excessiveProgress, error)) return 16;

    AchievementUnlock unlock{"ach-1", "content-1", "acct-1", 1000, 420, AuthoritySource::ZeroService};
    if (!achievements.ApplyUnlock(unlock, error)) return 17;
    if (achievements.ScoreFor("acct-1") != 100) return 18;
    if (achievements.UnlocksFor("acct-1", "content-1").size() != 1) return 19;

    auto forged = unlock;
    forged.achievementId = "ach-2";
    forged.authority = AuthoritySource::LocalPackage;
    if (achievements.ApplyUnlock(forged, error)) return 20;

    auto missingDefinition = unlock;
    missingDefinition.achievementId = "unknown";
    if (achievements.ApplyUnlock(missingDefinition, error)) return 21;

    auto invalidTarget = definition;
    invalidTarget.achievementId = "ach-invalid-target";
    invalidTarget.targetProgress = 0;
    if (achievements.ApplyDefinition(invalidTarget, error)) return 22;

    ProfileAuthority profiles;
    ProfileSnapshot profile{"acct-1", "ZERO_KING", "avatar-1", "banner-1", 48, 100, 4200, 1, true,
                            AuthoritySource::ZeroService, "In game", "Ready to play"};
    if (!profiles.Apply(profile, error)) return 30;
    const auto current = profiles.Current("acct-1");
    if (!current || current->level != 48 || current->achievementCount != 1 ||
        current->presence != "In game" || current->statusMessage != "Ready to play") return 31;

    auto forgedProfile = profile;
    forgedProfile.accountId = "acct-2";
    forgedProfile.authority = AuthoritySource::LocalPackage;
    if (profiles.Apply(forgedProfile, error)) return 32;

    std::cout << "ZERO V5 Achievements + Profile acceptance: PASS\n";
    return 0;
}
