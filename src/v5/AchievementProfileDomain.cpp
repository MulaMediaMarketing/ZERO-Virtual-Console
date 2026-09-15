#include "v5/AchievementProfileDomain.h"
#include <algorithm>

namespace zero::v5 {

namespace {
std::string DefinitionKeyFor(const std::string& contentId, const std::string& achievementId) {
    return contentId + "\n" + achievementId;
}
}

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
    if (definition.targetProgress == 0) {
        error = "achievement target progress must be greater than zero";
        return false;
    }
    const auto key = DefinitionKeyFor(definition.contentId, definition.achievementId);
    definitions_[key] = std::move(definition);
    error.clear();
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
    const auto definitionKey = DefinitionKeyFor(unlock.contentId, unlock.achievementId);
    if (!definitions_.contains(definitionKey)) {
        error = "achievement unlock has no authoritative definition";
        return false;
    }
    unlocks_[UnlockKey(unlock)] = std::move(unlock);
    error.clear();
    return true;
}

std::optional<AchievementDefinition> AchievementAuthority::Definition(const std::string& contentId,
                                                                      const std::string& achievementId) const {
    const auto it = definitions_.find(DefinitionKeyFor(contentId, achievementId));
    return it == definitions_.end() ? std::nullopt : std::optional<AchievementDefinition>{it->second};
}

std::vector<AchievementDefinition> AchievementAuthority::DefinitionsFor(const std::string& contentId) const {
    std::vector<AchievementDefinition> result;
    for (const auto& [_, definition] : definitions_) {
        if (definition.contentId == contentId) result.push_back(definition);
    }
    std::stable_sort(result.begin(), result.end(), [](const AchievementDefinition& a, const AchievementDefinition& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.title < b.title;
    });
    return result;
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
        const auto definition = definitions_.find(DefinitionKeyFor(unlock.contentId, unlock.achievementId));
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
    error.clear();
    return true;
}

std::optional<ProfileSnapshot> ProfileAuthority::Current(const std::string& accountId) const {
    const auto it = profiles_.find(accountId);
    return it == profiles_.end() ? std::nullopt : std::optional<ProfileSnapshot>{it->second};
}

} // namespace zero::v5
