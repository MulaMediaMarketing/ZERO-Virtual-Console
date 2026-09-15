#include "v5/DomainRepositories.h"
#include <windows.h>

namespace zero::v5 {
namespace {

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                          value.data(), static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                             value.data(), static_cast<int>(value.size()),
                             out.data(), bytes, nullptr, nullptr)) return {};
    return out;
}

bool databaseReady(const std::shared_ptr<zero::PlatformDatabase>& database, std::string& error) {
    if (!database) {
        error = "domain repository database is not configured";
        return false;
    }
    return true;
}

} // namespace

LocalDomainDatabase::LocalDomainDatabase(std::filesystem::path databasePath)
    : database_(std::make_shared<zero::PlatformDatabase>(std::move(databasePath))) {}

bool LocalDomainDatabase::Initialize(std::string& error) const {
    std::wstring wideError;
    if (!database_ || !database_->Initialize(wideError)) {
        error = utf8(wideError);
        if (error.empty()) error = "local domain database initialization failed";
        return false;
    }
    return true;
}

bool LocalSettingsRepository::Set(const std::string& key, const std::string& value, std::string& error) {
    if (!databaseReady(database_, error) || key.empty()) {
        if (error.empty()) error = "setting key is required";
        return false;
    }
    std::wstring wideError;
    if (!database_->SetSetting(key, value, wideError)) {
        error = utf8(wideError);
        return false;
    }
    return true;
}

std::optional<std::string> LocalSettingsRepository::Get(const std::string& key, std::string& error) const {
    if (!databaseReady(database_, error) || key.empty()) {
        if (error.empty()) error = "setting key is required";
        return std::nullopt;
    }
    std::wstring wideError;
    auto value = database_->GetSetting(key, wideError);
    if (!wideError.empty()) error = utf8(wideError);
    return value;
}

bool LocalResumeRepository::Save(const zero::ResumeMetadata& metadata, std::string& error) {
    if (!databaseReady(database_, error) || metadata.packageId.empty()) {
        if (error.empty()) error = "resume package identity is required";
        return false;
    }
    std::wstring wideError;
    if (!database_->UpsertResume(metadata.packageId, metadata.activityId, metadata.displayLabel,
                                 metadata.payload, metadata.updatedAtUtc, wideError)) {
        error = utf8(wideError);
        return false;
    }
    return true;
}

std::optional<zero::ResumeMetadata> LocalResumeRepository::Load(const std::string& packageId,
                                                                 std::string& error) const {
    if (!databaseReady(database_, error) || packageId.empty()) {
        if (error.empty()) error = "resume package identity is required";
        return std::nullopt;
    }
    std::wstring wideError;
    auto metadata = database_->LoadResume(packageId, wideError);
    if (!wideError.empty()) error = utf8(wideError);
    return metadata;
}

bool LocalResumeRepository::Clear(const std::string& packageId, std::string& error) {
    if (!databaseReady(database_, error) || packageId.empty()) {
        if (error.empty()) error = "resume package identity is required";
        return false;
    }
    std::wstring wideError;
    if (!database_->ClearResume(packageId, wideError)) {
        error = utf8(wideError);
        return false;
    }
    return true;
}

bool LocalAchievementRepository::Save(const std::string& packageId,
                                      const std::string& achievementId,
                                      const std::string& title,
                                      const std::string& unlockedAtUtc,
                                      std::string& error) {
    if (!databaseReady(database_, error) || packageId.empty() || achievementId.empty()) {
        if (error.empty()) error = "achievement package and achievement identity are required";
        return false;
    }
    std::wstring wideError;
    if (!database_->UpsertAchievement(packageId, achievementId, title, unlockedAtUtc, wideError)) {
        error = utf8(wideError);
        return false;
    }
    return true;
}

std::vector<zero::AchievementRecord> LocalAchievementRepository::Load(const std::string& packageId,
                                                                       std::string& error) const {
    if (!databaseReady(database_, error) || packageId.empty()) {
        if (error.empty()) error = "achievement package identity is required";
        return {};
    }
    std::wstring wideError;
    auto records = database_->LoadAchievements(packageId, wideError);
    if (!wideError.empty()) error = utf8(wideError);
    return records;
}

bool LocalSessionRepository::Record(const SessionRecord& record, std::string& error) {
    if (!databaseReady(database_, error) || record.packageId.empty() || record.sessionId.empty()) {
        if (error.empty()) error = "session package and session identity are required";
        return false;
    }
    std::wstring wideError;
    if (!database_->RecordSession(record.packageId, record.sessionId, record.playtimeSeconds,
                                  record.exitCode, record.outcome, record.crashed, wideError)) {
        error = utf8(wideError);
        return false;
    }
    return true;
}

} // namespace zero::v5
