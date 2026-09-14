#include "PlatformDatabase.h"
#include "StrictJson.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

using Clock = std::chrono::steady_clock;
using namespace zero;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

long long millis(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
}

int main() {
    bool ok = true;

    const std::string manifest = R"({"schema":1,"minimum_runtime_major":4,"package_id":"zero.benchmark.game","title":"Benchmark","version":"1.0.0","executable":"game.exe","zero_resume":true,"zero_achievements":true,"zero_overlay":true,"zero_input":true})";
    constexpr int kJsonIterations = 50000;
    const auto jsonStart = Clock::now();
    for (int i = 0; i < kJsonIterations; ++i) {
        const auto parsed = json::Parse(manifest);
        if (!parsed.ok || !parsed.root.AsObject()) {
            std::cerr << "FAIL: strict JSON benchmark parse failed\n";
            return 1;
        }
    }
    const auto jsonMs = millis(jsonStart, Clock::now());
    ok &= expect(jsonMs <= 5000, "50k strict JSON parses exceeded 5 second CI budget");

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / (L"zero-production-benchmark-" + std::to_wstring(stamp));
    PlatformDatabase db(root / L"Data" / L"zero.db");
    std::wstring error;
    if (!db.Initialize(error)) {
        std::wcerr << L"FAIL: benchmark database initialization failed: " << error << L'\n';
        return 1;
    }

    GameManifest game;
    game.packageId = "zero.benchmark.game";
    game.title = "Benchmark";
    game.version = "1.0.0";
    game.executable = root / L"game.exe";
    if (!db.UpsertGame(game, error)) return 1;

    constexpr int kTransactions = 250;
    const auto dbStart = Clock::now();
    for (int i = 0; i < kTransactions; ++i) {
        const std::string session = "session-" + std::to_string(i);
        if (!db.RecordSession(game.packageId, session, 1, 0, "clean_exit", false, error)) {
            std::wcerr << L"FAIL: benchmark transaction failed: " << error << L'\n';
            return 1;
        }
    }
    const auto dbMs = millis(dbStart, Clock::now());
    ok &= expect(dbMs <= 10000, "250 FULL-synchronous SQLite session transactions exceeded 10 second CI budget");

    const auto state = db.LoadGameState(game.packageId, error);
    ok &= expect(state && state->launchCount == kTransactions, "benchmark database state must remain correct");

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    std::cout << "METRIC strict_json_50000_ms=" << jsonMs << '\n';
    std::cout << "METRIC sqlite_full_250_sessions_ms=" << dbMs << '\n';
    std::cout << "METRIC sqlite_sessions_per_second=" << (dbMs > 0 ? (kTransactions * 1000LL / dbMs) : kTransactions * 1000LL) << '\n';
    if (!ok) return 1;
    std::cout << "PASS: ZERO production core performance benchmark\n";
    return 0;
}
