#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace zero::v5 {

struct AchievementDefinition {
    std::string achievementId;
    std::string contentId;
    std::string title;
    std::uint32_t score{0};
    AuthoritySource authority{AuthoritySource::None};
    std::string description;
    std::string iconId;
    bool secret{false};
    std::uint32_t targetProgress{1};
};

struct AchievementUnlock {
    std::string achievementId;
    std::string contentId;
    std::string accountId;
    std::uint64_t unlockedEpochSeconds{0};
    std::uint32_t rarityBasisPoints{0};
    AuthoritySource authority{AuthoritySource::None};
};

struct AchievementProgress {
    std::string achievementId;
    std::string contentId;
    std::string accountId;
    std::uint32_t currentProgress{0};
    std::uint64_t updatedEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

struct ProfileSnapshot {
    std::string accountId;
    std::string displayName;
    std::string avatarId;
    std::string bannerId;
    std::uint32_t level{0};
    std::uint64_t zeroScore{0};
    std::uint64_t playtimeSeconds{0};
    std::uint64_t achievementCount{0};
    bool online{false};
    AuthoritySource authority{AuthoritySource::None};
    std::string presence;
    std::string statusMessage;
};

class AchievementAuthority {
public:
    bool ApplyDefinition(AchievementDefinition definition, std::string& error);
    bool ApplyUnlock(AchievementUnlock unlock, std::string& error);
    bool ApplyProgress(AchievementProgress progress, std::string& error);
    std::optional<AchievementDefinition> Definition(const std::string& contentId,
                                                    const std::string& achievementId) const;
    std::vector<AchievementDefinition> DefinitionsFor(const std::string& contentId) const;
    std::vector<AchievementUnlock> UnlocksFor(const std::string& accountId,
                                              const std::string& contentId = {}) const;
    std::optional<AchievementProgress> ProgressFor(const std::string& accountId,
                                                   const std::string& contentId,
                                                   const std::string& achievementId) const;
    std::uint64_t ScoreFor(const std::string& accountId) const;

private:
    std::unordered_map<std::string, AchievementDefinition> definitions_;
    std::unordered_map<std::string, AchievementUnlock> unlocks_;
    std::unordered_map<std::string, AchievementProgress> progress_;
    static std::string UnlockKey(const AchievementUnlock& unlock);
    static std::string ProgressKey(const AchievementProgress& progress);
};

class ProfileAuthority {
public:
    bool Apply(ProfileSnapshot snapshot, std::string& error);
    std::optional<ProfileSnapshot> Current(const std::string& accountId) const;
private:
    std::unordered_map<std::string, ProfileSnapshot> profiles_;
};

} // namespace zero::v5
