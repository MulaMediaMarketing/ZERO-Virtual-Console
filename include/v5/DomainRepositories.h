#pragma once

#include "AchievementStore.h"
#include "PlatformDatabase.h"
#include "ResumeStore.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace zero::v5 {

struct SessionRecord {
    std::string packageId;
    std::string sessionId;
    std::uint64_t playtimeSeconds{0};
    std::uint32_t exitCode{0};
    std::string outcome;
    bool crashed{false};
};

class ISettingsRepository {
public:
    virtual ~ISettingsRepository() = default;
    virtual bool Set(const std::string& key, const std::string& value, std::string& error) = 0;
    virtual std::optional<std::string> Get(const std::string& key, std::string& error) const = 0;
};

class IResumeRepository {
public:
    virtual ~IResumeRepository() = default;
    virtual bool Save(const zero::ResumeMetadata& metadata, std::string& error) = 0;
    virtual std::optional<zero::ResumeMetadata> Load(const std::string& packageId, std::string& error) const = 0;
    virtual bool Clear(const std::string& packageId, std::string& error) = 0;
};

class IAchievementRepository {
public:
    virtual ~IAchievementRepository() = default;
    virtual bool Save(const std::string& packageId,
                      const std::string& achievementId,
                      const std::string& title,
                      const std::string& unlockedAtUtc,
                      std::string& error) = 0;
    virtual std::vector<zero::AchievementRecord> Load(const std::string& packageId,
                                                       std::string& error) const = 0;
};

class ISessionRepository {
public:
    virtual ~ISessionRepository() = default;
    virtual bool Record(const SessionRecord& record, std::string& error) = 0;
};

class LocalDomainDatabase {
public:
    explicit LocalDomainDatabase(std::filesystem::path databasePath = {});
    bool Initialize(std::string& error) const;
    std::shared_ptr<zero::PlatformDatabase> Database() const noexcept { return database_; }
private:
    std::shared_ptr<zero::PlatformDatabase> database_;
};

class LocalSettingsRepository final : public ISettingsRepository {
public:
    explicit LocalSettingsRepository(std::shared_ptr<zero::PlatformDatabase> database) : database_(std::move(database)) {}
    bool Set(const std::string& key, const std::string& value, std::string& error) override;
    std::optional<std::string> Get(const std::string& key, std::string& error) const override;
private:
    std::shared_ptr<zero::PlatformDatabase> database_;
};

class LocalResumeRepository final : public IResumeRepository {
public:
    explicit LocalResumeRepository(std::shared_ptr<zero::PlatformDatabase> database) : database_(std::move(database)) {}
    bool Save(const zero::ResumeMetadata& metadata, std::string& error) override;
    std::optional<zero::ResumeMetadata> Load(const std::string& packageId, std::string& error) const override;
    bool Clear(const std::string& packageId, std::string& error) override;
private:
    std::shared_ptr<zero::PlatformDatabase> database_;
};

class LocalAchievementRepository final : public IAchievementRepository {
public:
    explicit LocalAchievementRepository(std::shared_ptr<zero::PlatformDatabase> database) : database_(std::move(database)) {}
    bool Save(const std::string& packageId,
              const std::string& achievementId,
              const std::string& title,
              const std::string& unlockedAtUtc,
              std::string& error) override;
    std::vector<zero::AchievementRecord> Load(const std::string& packageId,
                                               std::string& error) const override;
private:
    std::shared_ptr<zero::PlatformDatabase> database_;
};

class LocalSessionRepository final : public ISessionRepository {
public:
    explicit LocalSessionRepository(std::shared_ptr<zero::PlatformDatabase> database) : database_(std::move(database)) {}
    bool Record(const SessionRecord& record, std::string& error) override;
private:
    std::shared_ptr<zero::PlatformDatabase> database_;
};

} // namespace zero::v5
