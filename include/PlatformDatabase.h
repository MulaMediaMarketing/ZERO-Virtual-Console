#pragma once
#include "AchievementStore.h"
#include "PlatformStateStore.h"
#include "ResumeStore.h"
#include "ZeroTypes.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace zero {

class PlatformDatabase {
public:
    explicit PlatformDatabase(std::filesystem::path databasePath = {});

    bool Initialize(std::wstring& error) const;
    bool UpsertGame(const GameManifest& game, std::wstring& error) const;
    bool RecordSession(const std::string& packageId,
                       const std::string& sessionId,
                       uint64_t playtimeSeconds,
                       uint32_t exitCode,
                       const std::string& outcome,
                       bool crashed,
                       std::wstring& error) const;
    bool UpsertResume(const std::string& packageId,
                      const std::string& activityId,
                      const std::string& displayLabel,
                      const std::string& payload,
                      const std::string& updatedAtUtc,
                      std::wstring& error) const;
    bool ClearResume(const std::string& packageId, std::wstring& error) const;
    bool UpsertAchievement(const std::string& packageId,
                           const std::string& achievementId,
                           const std::string& title,
                           const std::string& unlockedAtUtc,
                           std::wstring& error) const;
    bool SetSetting(const std::string& key,
                    const std::string& value,
                    std::wstring& error) const;

    std::optional<std::string> GetSetting(const std::string& key,
                                          std::wstring& error) const;
    std::optional<GamePlatformState> LoadGameState(const std::string& packageId,
                                                   std::wstring& error) const;
    std::optional<ResumeMetadata> LoadResume(const std::string& packageId,
                                             std::wstring& error) const;
    std::vector<AchievementRecord> LoadAchievements(const std::string& packageId,
                                                     std::wstring& error) const;

    const std::filesystem::path& Path() const noexcept { return databasePath_; }

private:
    std::filesystem::path databasePath_;
};

} // namespace zero
