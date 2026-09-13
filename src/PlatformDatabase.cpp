#include "PlatformDatabase.h"
#include <winsqlite/winsqlite3.h>
#include <windows.h>
#include <shlobj.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

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
    sqlite3_stmt* get() const { return stmt_; }
private:
    sqlite3_stmt* stmt_{nullptr};
};

std::filesystem::path databasePath() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO" / "Data" / "zero.db";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::temp_directory_path() / "ZERO" / "Data" / "zero.db";
}

std::string utcNow() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_s(&utc, &tt);
    std::ostringstream os;
    os << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}

std::wstring widenUtf8(const char* text) {
    if (!text || !*text) return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
    if (n <= 1) return L"SQLite operation failed.";
    std::wstring out(static_cast<size_t>(n - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, out.data(), n);
    return out;
}

bool fail(sqlite3* db, const wchar_t* prefix, std::wstring& error) {
    error = prefix;
    if (db) {
        const auto detail = widenUtf8(sqlite3_errmsg(db));
        if (!detail.empty()) error += L" " + detail;
    }
    return false;
}

bool exec(sqlite3* db, const char* sql, std::wstring& error) {
    char* message = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &message);
    if (rc == SQLITE_OK) return true;
    error = L"ZERO database operation failed.";
    if (message) {
        const auto detail = widenUtf8(message);
        if (!detail.empty()) error += L" " + detail;
        sqlite3_free(message);
    }
    return false;
}

bool openDatabase(DbHandle& handle, std::wstring& error) {
    const auto path = databasePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        error = L"ZERO could not create the platform database directory.";
        return false;
    }

    const int rc = sqlite3_open16(path.c_str(), handle.out());
    if (rc != SQLITE_OK) return fail(handle.get(), L"ZERO could not open the platform database.", error);

    sqlite3_busy_timeout(handle.get(), 3000);
    if (!exec(handle.get(), "PRAGMA journal_mode=WAL;", error)) return false;
    if (!exec(handle.get(), "PRAGMA synchronous=FULL;", error)) return false;
    if (!exec(handle.get(), "PRAGMA foreign_keys=ON;", error)) return false;
    return true;
}

bool ensureSchema(sqlite3* db, std::wstring& error) {
    static constexpr const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS games(
    package_id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    version TEXT NOT NULL,
    executable TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS sessions(
    session_id TEXT PRIMARY KEY,
    package_id TEXT NOT NULL,
    playtime_seconds INTEGER NOT NULL,
    exit_code INTEGER NOT NULL,
    outcome TEXT NOT NULL,
    crashed INTEGER NOT NULL,
    recorded_at TEXT NOT NULL,
    FOREIGN KEY(package_id) REFERENCES games(package_id) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS game_stats(
    package_id TEXT PRIMARY KEY,
    total_playtime_seconds INTEGER NOT NULL DEFAULT 0,
    launch_count INTEGER NOT NULL DEFAULT 0,
    last_session_id TEXT,
    last_played_utc TEXT,
    last_exit_code INTEGER NOT NULL DEFAULT 0,
    last_crashed INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY(package_id) REFERENCES games(package_id) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS resume_activities(
    package_id TEXT PRIMARY KEY,
    activity_id TEXT NOT NULL,
    display_label TEXT NOT NULL,
    payload TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY(package_id) REFERENCES games(package_id) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS achievements(
    package_id TEXT NOT NULL,
    achievement_id TEXT NOT NULL,
    title TEXT NOT NULL,
    unlocked_at TEXT NOT NULL,
    PRIMARY KEY(package_id, achievement_id),
    FOREIGN KEY(package_id) REFERENCES games(package_id) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS settings(
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_sessions_package_recorded
    ON sessions(package_id, recorded_at DESC);
)SQL";
    return exec(db, schema, error);
}

bool prepare(sqlite3* db, const char* sql, Statement*& owner, std::wstring& error) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
        return fail(db, L"ZERO could not prepare a database statement.", error);
    }
    owner = new Statement(raw);
    return true;
}

bool bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    return sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string pathUtf8(const std::filesystem::path& path) {
    const auto wide = path.wstring();
    if (wide.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
                                          static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()),
                        out.data(), bytes, nullptr, nullptr);
    return out;
}

bool stepDone(sqlite3* db, sqlite3_stmt* stmt, std::wstring& error) {
    if (sqlite3_step(stmt) == SQLITE_DONE) return true;
    return fail(db, L"ZERO database write failed.", error);
}

} // namespace

bool PlatformDatabase::Initialize(std::wstring& error) const {
    DbHandle db;
    return openDatabase(db, error) && ensureSchema(db.get(), error);
}

bool PlatformDatabase::UpsertGame(const GameManifest& game, std::wstring& error) const {
    DbHandle db;
    if (!openDatabase(db, error) || !ensureSchema(db.get(), error)) return false;

    static constexpr const char* sql =
        "INSERT INTO games(package_id,title,version,executable,updated_at) VALUES(?,?,?,?,?) "
        "ON CONFLICT(package_id) DO UPDATE SET title=excluded.title, version=excluded.version, "
        "executable=excluded.executable, updated_at=excluded.updated_at;";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK)
        return fail(db.get(), L"ZERO could not prepare the game database write.", error);
    Statement stmt(raw);
    const auto stamp = utcNow();
    const auto executable = pathUtf8(game.executable);
    if (!bindText(raw, 1, game.packageId) || !bindText(raw, 2, game.title) ||
        !bindText(raw, 3, game.version) || !bindText(raw, 4, executable) || !bindText(raw, 5, stamp))
        return fail(db.get(), L"ZERO could not bind the game database write.", error);
    return stepDone(db.get(), raw, error);
}

bool PlatformDatabase::RecordSession(const std::string& packageId,
                                     const std::string& sessionId,
                                     uint64_t playtimeSeconds,
                                     uint32_t exitCode,
                                     const std::string& outcome,
                                     bool crashed,
                                     std::wstring& error) const {
    DbHandle db;
    if (!openDatabase(db, error) || !ensureSchema(db.get(), error)) return false;
    if (!exec(db.get(), "BEGIN IMMEDIATE;", error)) return false;

    bool ok = true;
    const auto stamp = utcNow();
    static constexpr const char* sessionSql =
        "INSERT OR REPLACE INTO sessions(session_id,package_id,playtime_seconds,exit_code,outcome,crashed,recorded_at) "
        "VALUES(?,?,?,?,?,?,?);";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sessionSql, -1, &raw, nullptr) != SQLITE_OK) ok = false;
    if (ok) {
        Statement stmt(raw);
        ok = bindText(raw, 1, sessionId) && bindText(raw, 2, packageId) &&
             sqlite3_bind_int64(raw, 3, static_cast<sqlite3_int64>(playtimeSeconds)) == SQLITE_OK &&
             sqlite3_bind_int64(raw, 4, static_cast<sqlite3_int64>(exitCode)) == SQLITE_OK &&
             bindText(raw, 5, outcome) && sqlite3_bind_int(raw, 6, crashed ? 1 : 0) == SQLITE_OK &&
             bindText(raw, 7, stamp) && sqlite3_step(raw) == SQLITE_DONE;
    }

    static constexpr const char* statsSql =
        "INSERT INTO game_stats(package_id,total_playtime_seconds,launch_count,last_session_id,last_played_utc,last_exit_code,last_crashed) "
        "VALUES(?,?,?,?,?,?,?) "
        "ON CONFLICT(package_id) DO UPDATE SET "
        "total_playtime_seconds=game_stats.total_playtime_seconds+excluded.total_playtime_seconds, "
        "launch_count=game_stats.launch_count+1, last_session_id=excluded.last_session_id, "
        "last_played_utc=excluded.last_played_utc, last_exit_code=excluded.last_exit_code, "
        "last_crashed=excluded.last_crashed;";
    raw = nullptr;
    if (ok && sqlite3_prepare_v2(db.get(), statsSql, -1, &raw, nullptr) == SQLITE_OK) {
        Statement stmt(raw);
        ok = bindText(raw, 1, packageId) &&
             sqlite3_bind_int64(raw, 2, static_cast<sqlite3_int64>(playtimeSeconds)) == SQLITE_OK &&
             sqlite3_bind_int(raw, 3, 1) == SQLITE_OK && bindText(raw, 4, sessionId) &&
             bindText(raw, 5, stamp) && sqlite3_bind_int64(raw, 6, static_cast<sqlite3_int64>(exitCode)) == SQLITE_OK &&
             sqlite3_bind_int(raw, 7, crashed ? 1 : 0) == SQLITE_OK && sqlite3_step(raw) == SQLITE_DONE;
    } else if (ok) {
        ok = false;
    }

    if (!ok) {
        std::wstring ignored;
        exec(db.get(), "ROLLBACK;", ignored);
        return fail(db.get(), L"ZERO could not transactionally record the runtime session.", error);
    }
    if (!exec(db.get(), "COMMIT;", error)) return false;
    return true;
}

bool PlatformDatabase::UpsertResume(const std::string& packageId,
                                    const std::string& activityId,
                                    const std::string& displayLabel,
                                    const std::string& payload,
                                    const std::string& updatedAtUtc,
                                    std::wstring& error) const {
    DbHandle db;
    if (!openDatabase(db, error) || !ensureSchema(db.get(), error)) return false;
    static constexpr const char* sql =
        "INSERT INTO resume_activities(package_id,activity_id,display_label,payload,updated_at) VALUES(?,?,?,?,?) "
        "ON CONFLICT(package_id) DO UPDATE SET activity_id=excluded.activity_id, display_label=excluded.display_label, "
        "payload=excluded.payload, updated_at=excluded.updated_at;";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK)
        return fail(db.get(), L"ZERO could not prepare the Resume database write.", error);
    Statement stmt(raw);
    const auto stamp = updatedAtUtc.empty() ? utcNow() : updatedAtUtc;
    if (!bindText(raw, 1, packageId) || !bindText(raw, 2, activityId) || !bindText(raw, 3, displayLabel) ||
        !bindText(raw, 4, payload) || !bindText(raw, 5, stamp))
        return fail(db.get(), L"ZERO could not bind the Resume database write.", error);
    return stepDone(db.get(), raw, error);
}

bool PlatformDatabase::UpsertAchievement(const std::string& packageId,
                                         const std::string& achievementId,
                                         const std::string& title,
                                         const std::string& unlockedAtUtc,
                                         std::wstring& error) const {
    DbHandle db;
    if (!openDatabase(db, error) || !ensureSchema(db.get(), error)) return false;
    static constexpr const char* sql =
        "INSERT OR IGNORE INTO achievements(package_id,achievement_id,title,unlocked_at) VALUES(?,?,?,?);";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK)
        return fail(db.get(), L"ZERO could not prepare the achievement database write.", error);
    Statement stmt(raw);
    const auto stamp = unlockedAtUtc.empty() ? utcNow() : unlockedAtUtc;
    if (!bindText(raw, 1, packageId) || !bindText(raw, 2, achievementId) ||
        !bindText(raw, 3, title) || !bindText(raw, 4, stamp))
        return fail(db.get(), L"ZERO could not bind the achievement database write.", error);
    return stepDone(db.get(), raw, error);
}

bool PlatformDatabase::SetSetting(const std::string& key,
                                  const std::string& value,
                                  std::wstring& error) const {
    DbHandle db;
    if (!openDatabase(db, error) || !ensureSchema(db.get(), error)) return false;
    static constexpr const char* sql =
        "INSERT INTO settings(key,value,updated_at) VALUES(?,?,?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value, updated_at=excluded.updated_at;";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db.get(), sql, -1, &raw, nullptr) != SQLITE_OK)
        return fail(db.get(), L"ZERO could not prepare the settings database write.", error);
    Statement stmt(raw);
    const auto stamp = utcNow();
    if (!bindText(raw, 1, key) || !bindText(raw, 2, value) || !bindText(raw, 3, stamp))
        return fail(db.get(), L"ZERO could not bind the settings database write.", error);
    return stepDone(db.get(), raw, error);
}

} // namespace zero
