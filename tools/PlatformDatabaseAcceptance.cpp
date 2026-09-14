#include "PlatformDatabase.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

using namespace zero;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

std::filesystem::path tempRoot() {
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / (L"zero-db-acceptance-" + std::to_wstring(stamp));
}
}

int main() {
    bool ok = true;
    const auto root = tempRoot();
    const auto dbPath = root / L"Data" / L"zero.db";
    std::error_code ec;
    std::wstring error;

    PlatformDatabase db(dbPath);
    ok &= expect(db.Path() == dbPath, "database must use injected path");
    ok &= expect(db.Initialize(error), "isolated database must initialize");
    ok &= expect(std::filesystem::exists(dbPath), "isolated database file must exist");

    GameManifest game;
    game.packageId = "zero.acceptance.database";
    game.title = "Database Acceptance";
    game.version = "1.0.0";
    game.executable = root / L"game.exe";
    ok &= expect(db.UpsertGame(game, error), "game row must upsert");

    ok &= expect(db.RecordSession(game.packageId, "session-1", 15, 0, "clean_exit", false, error),
                 "first session must record");
    ok &= expect(db.RecordSession(game.packageId, "session-1", 15, 0, "clean_exit", false, error),
                 "duplicate session must be idempotently accepted");
    auto stats = db.LoadGameState(game.packageId, error);
    ok &= expect(stats.has_value(), "game stats must load");
    if (stats) {
        ok &= expect(stats->launchCount == 1, "duplicate session ID must not increment launch count twice");
        ok &= expect(stats->totalPlaytimeSeconds == 15, "duplicate session ID must not double playtime");
        ok &= expect(!stats->lastSessionCrashed, "clean session must not be marked crashed");
    }

    ok &= expect(db.SetSetting("volume", "73", error), "setting write must succeed");
    const auto volume = db.GetSetting("volume", error);
    ok &= expect(volume && *volume == "73", "setting must round-trip from canonical DB");

    ok &= expect(db.UpsertResume(game.packageId, "activity-1", "Checkpoint", "payload", "2026-09-14T00:00:00Z", error),
                 "Resume upsert must succeed");
    const auto resume = db.LoadResume(game.packageId, error);
    ok &= expect(resume && resume->activityId == "activity-1" && resume->payload == "payload",
                 "Resume metadata must round-trip");
    ok &= expect(db.ClearResume(game.packageId, error), "Resume clear must succeed");
    ok &= expect(!db.LoadResume(game.packageId, error).has_value(), "Resume must be absent after clear");

    ok &= expect(db.UpsertAchievement(game.packageId, "ach-1", "First Achievement", "2026-09-14T00:00:00Z", error),
                 "achievement upsert must succeed");
    ok &= expect(db.UpsertAchievement(game.packageId, "ach-1", "Duplicate", "2026-09-14T00:00:01Z", error),
                 "duplicate achievement must be idempotently accepted");
    const auto achievements = db.LoadAchievements(game.packageId, error);
    ok &= expect(achievements.size() == 1, "duplicate achievement ID must not duplicate records");

    std::filesystem::remove_all(root, ec);
    if (!ok) return 1;
    std::cout << "PASS: ZERO canonical platform database hermetic contract\n";
    return 0;
}
