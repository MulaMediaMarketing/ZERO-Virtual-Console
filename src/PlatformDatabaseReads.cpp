#include "PlatformDatabase.h"
#include <winsqlite/winsqlite3.h>
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace zero {
namespace {

class DbHandle {
public:
    ~DbHandle() { if (db_) sqlite3_close(db_); }
    sqlite3** out() { return &db_; }
    sqlite3* get() const { return db_; }
private:
    sqlite3* db_{nullptr};
};

class Statement {
public:
    explicit Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}
    ~Statement() { if (stmt_) sqlite3_finalize(stmt_); }
private:
    sqlite3_stmt* stmt_{nullptr};
};

std::filesystem::path dbPath() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO" / "Data" / "zero.db";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::temp_directory_path() / "ZERO" / "Data" / "zero.db";
}

bool openDb(DbHandle& handle, std::wstring& error) {
    const auto path = dbPath();
    if (sqlite3_open16(path.c_str(), handle.out()) != SQLITE_OK) {
        error = L"ZERO could not open the canonical platform database for shell reads.";
        return false;
    }
    sqlite3_busy_timeout(handle.get(), 3000);
    return true;
}

std::string text(sqlite3_stmt* stmt, int column) {
    const auto* value = sqlite3_column_text(stmt, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}

bool bindPackage(sqlite3_stmt* stmt, const std::string& packageId) {
    return sqlite3_bind_text(stmt, 1, packageId.c_str(), static_cast<int>(packageId.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

} // namespace

std::optional<GamePlatformState> PlatformDatabase::LoadGameState(const std::string& packageId,
                                                                 std::wstring& error) const {
    if (!Initialize(error)) return std::nullopt;
    DbHandle db;
    if (!openDb(db, error)) return std::nullopt;

    static constexpr const char* sql =
        "SELECT total_playtime_seconds, launch_count, last_session_id, last_played_utc, last_exit_code, last_crashed "
        "FROM game_stats WHERE package_id=? LIMIT 1;";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK) {
        error = L"ZERO could not prepare the canonical game-state query.";
        return std::nullopt;
    }
    Statement stmt(raw);
    if (!bindPackage(raw, packageId)) {
        error = L"ZERO could not bind the canonical game-state query.";
        return std::nullopt;
    }

    const int rc = sqlite3_step(raw);
    if (rc == SQLITE_DONE) return std::nullopt;
    if (rc != SQLITE_ROW) {
        error = L"ZERO could not read canonical game state.";
        return std::nullopt;
    }

    GamePlatformState state{};
    state.packageId = packageId;
    state.totalPlaytimeSeconds = static_cast<uint64_t>(sqlite3_column_int64(raw, 0));
    state.launchCount = static_cast<uint64_t>(sqlite3_column_int64(raw, 1));
    state.lastSessionId = text(raw, 2);
    state.lastPlayedAtUtc = text(raw, 3);
    state.lastExitCode = static_cast<uint32_t>(sqlite3_column_int64(raw, 4));
    state.lastSessionCrashed = sqlite3_column_int(raw, 5) != 0;
    return state;
}

std::optional<ResumeMetadata> PlatformDatabase::LoadResume(const std::string& packageId,
                                                           std::wstring& error) const {
    if (!Initialize(error)) return std::nullopt;
    DbHandle db;
    if (!openDb(db, error)) return std::nullopt;

    static constexpr const char* sql =
        "SELECT activity_id, display_label, payload, updated_at FROM resume_activities WHERE package_id=? LIMIT 1;";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK) {
        error = L"ZERO could not prepare the canonical Resume query.";
        return std::nullopt;
    }
    Statement stmt(raw);
    if (!bindPackage(raw, packageId)) {
        error = L"ZERO could not bind the canonical Resume query.";
        return std::nullopt;
    }

    const int rc = sqlite3_step(raw);
    if (rc == SQLITE_DONE) return std::nullopt;
    if (rc != SQLITE_ROW) {
        error = L"ZERO could not read canonical Resume metadata.";
        return std::nullopt;
    }

    ResumeMetadata resume{};
    resume.packageId = packageId;
    resume.activityId = text(raw, 0);
    resume.displayLabel = text(raw, 1);
    resume.payload = text(raw, 2);
    resume.updatedAtUtc = text(raw, 3);
    return resume;
}

std::vector<AchievementRecord> PlatformDatabase::LoadAchievements(const std::string& packageId,
                                                                   std::wstring& error) const {
    std::vector<AchievementRecord> out;
    if (!Initialize(error)) return out;
    DbHandle db;
    if (!openDb(db, error)) return out;

    static constexpr const char* sql =
        "SELECT achievement_id, title, unlocked_at FROM achievements WHERE package_id=? ORDER BY unlocked_at DESC, achievement_id ASC;";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK) {
        error = L"ZERO could not prepare the canonical achievement query.";
        return out;
    }
    Statement stmt(raw);
    if (!bindPackage(raw, packageId)) {
        error = L"ZERO could not bind the canonical achievement query.";
        return out;
    }

    for (;;) {
        const int rc = sqlite3_step(raw);
        if (rc == SQLITE_DONE) break;
        if (rc != SQLITE_ROW) {
            out.clear();
            error = L"ZERO could not read canonical achievement data.";
            break;
        }
        out.push_back({text(raw, 0), text(raw, 1), text(raw, 2)});
    }
    return out;
}

} // namespace zero
