#include "v5/AchievementProfileDomain.h"
#include <algorithm>

namespace zero::v5 {

std::string AchievementAuthority::UnlockKey(const AchievementUnlock& unlock) {
    return unlock.accountId + "\n" + unlock.contentId + "\n" + unlock.achievementId;
}

bool AchievementAuthority::ApplyDefinition(AchievementDefinition definition, std::string& error) {
    if (definition.authority != AuthoritySource::ZeroService) {
        error = "achievement definition was not issued by ZERO service";
        return false;
    }
    if (definition.achievementId.empty() || definition.contentId.empty() || definition.title.empty()) {
        error = "achievement definition is missing identity or title";
        return false;
    }
    const auto key = definition.contentId + "\n" + definition.achievementId;
    definitions_[key] = std::move(definition);
    return true;
}

bool AchievementAuthority::ApplyUnlock(AchievementUnlock unlock, std::string& error) {
    if (unlock.authority != AuthoritySource::ZeroService) {
        error = "achievement unlock was not validated by ZERO service";
        return false;
    }
    if (unlock.achievementId.empty() || unlock.contentId.empty() || unlock.accountId.empty()) {
        error = "achievement unlock is missing identity";
        return false;
    }
    if (unlock.rarityBasisPoints > 10000) {
        error = "achievement rarity is out of range";
        return false;
    }
    const auto definitionKey = unlock.contentId + "\n" + unlock.achievementId;
    if (!definitions_.contains(definitionKey)) {
        error = "achievement unlock has no authoritative definition";
        return false;
    }
    unlocks_[UnlockKey(unlock)] = std::move(unlock);
    return true;
}

std::vector<AchievementUnlock> AchievementAuthority::UnlocksFor(const std::string& accountId,
                                                                 const std::string& contentId) const {
    std::vector<AchievementUnlock> result;
    for (const auto& [_, unlock] : unlocks_) {
        if (unlock.accountId == accountId && (contentId.empty() || unlock.contentId == contentId))
            result.push_back(unlock);
    }
    std::stable_sort(result.begin(), result.end(), [](const AchievementUnlock& a, const AchievementUnlock& b) {
        return a.unlockedEpochSeconds > b.unlockedEpochSeconds;
    });
    return result;
}

std::uint64_t AchievementAuthority::ScoreFor(const std::string& accountId) const {
    std::uint64_t total = 0;
    for (const auto& [_, unlock] : unlocks_) {
        if (unlock.accountId != accountId) continue;
        const auto key = unlock.contentId + "\n" + unlock.achievementId;
        const auto definition = definitions_.find(key);
        if (definition != definitions_.end()) total += definition->second.score;
    }
    return total;
}

bool ProfileAuthority::Apply(ProfileSnapshot snapshot, std::string& error) {
    if (snapshot.authority != AuthoritySource::ZeroService) {
        error = "profile snapshot was not issued by ZERO service";
        return false;
    }
    if (snapshot.accountId.empty() || snapshot.displayName.empty()) {
        error = "profile snapshot is missing identity";
        return false;
    }
    profiles_[snapshot.accountId] = std::move(snapshot);
    return true;
}

std::optional<ProfileSnapshot> ProfileAuthority::Current(const std::string& accountId) const {
    const auto it = profiles_.find(accountId);
    return it == profiles_.end() ? std::nullopt : std::optional<ProfileSnapshot>{it->second};
}

} // namespace zero::v5
