#include "GameRegistry.h"
#include "PlatformDatabase.h"
#include "StrictJson.h"
#include "Utf8Path.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using Clock = std::chrono::steady_clock;
using namespace zero;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; return false; }
    return true;
}
long long millis(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
std::string idFor(int index) {
    std::ostringstream out;
    out << "zero.benchmark.game." << std::setw(4) << std::setfill('0') << index;
    return out.str();
}
}

int main() {
    bool ok = true;
    const std::string manifest = R"({"schema":1,"minimum_runtime_major":4,"package_id":"zero.benchmark.game","title":"Benchmark","version":"1.0.0","executable":"game.exe","zero_resume":true,"zero_achievements":true,"zero_overlay":true,"zero_input":true})";

    constexpr int kJsonIterations = 50000;
    const auto jsonStart = Clock::now();
    for (int i = 0; i < kJsonIterations; ++i) {
        const auto parsed = json::Parse(manifest);
        if (!parsed.ok || !parsed.root.AsObject()) { std::cerr << "FAIL: strict JSON benchmark parse failed\n"; return 1; }
    }
    const auto jsonMs = millis(jsonStart, Clock::now());
    ok &= expect(jsonMs <= 5000, "50k strict JSON parses exceeded 5 second CI budget");

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / (L"zero-production-benchmark-" + std::to_wstring(stamp));
    std::filesystem::create_directories(root);

    PlatformDatabase db(root / L"Data" / L"zero.db");
    std::wstring error;
    if (!db.Initialize(error)) { std::wcerr << L"FAIL: benchmark database initialization failed: " << error << L'\n'; return 1; }
    GameManifest game; game.packageId="zero.benchmark.game"; game.title="Benchmark"; game.version="1.0.0"; game.executable=root/L"game.exe";
    if (!db.UpsertGame(game,error)) return 1;

    constexpr int kTransactions = 250;
    constexpr long long kDurableSessionBudgetMs = 80;
    const auto dbStart=Clock::now();
    for(int i=0;i<kTransactions;++i){
        if(!db.RecordSession(game.packageId,"session-"+std::to_string(i),1,0,"clean_exit",false,error)){
            std::wcerr<<L"FAIL: benchmark transaction failed: "<<error<<L'\n';return 1;
        }
    }
    const auto dbMs=millis(dbStart,Clock::now());
    const auto dbAverageMs = dbMs / static_cast<long long>(kTransactions);
    ok &= expect(dbAverageMs<=kDurableSessionBudgetMs,"FULL-synchronous SQLite session persistence exceeded 80 ms average per durable session write");
    const auto state=db.LoadGameState(game.packageId,error);
    ok &= expect(state && state->launchCount==static_cast<uint64_t>(kTransactions),"benchmark database state must remain correct");

    constexpr int kCatalogGames=1000;
    const auto library=root/L"Library";
    std::filesystem::create_directories(library);
    for(int i=0;i<kCatalogGames;++i){
        const auto id=idFor(i);
        const auto relative=PathFromUtf8(id);
        if(!relative){std::cerr<<"FAIL: benchmark package id could not convert to a filesystem path\n";return 1;}
        const auto package=library / *relative;
        std::filesystem::create_directories(package);
        std::ofstream exe(package/L"game.exe",std::ios::binary); exe.put('\0'); exe.close();
        std::ofstream mf(package/L"zero.manifest.json",std::ios::binary);
        mf << "{\"schema\":1,\"minimum_runtime_major\":4,\"package_id\":\"" << id
           << "\",\"title\":\"Benchmark " << i
           << "\",\"version\":\"1.0.0\",\"executable\":\"game.exe\"}";
    }
    GameRegistry registry(library);
    const auto registryStart=Clock::now();
    registry.Refresh();
    const auto registryMs=millis(registryStart,Clock::now());
    ok &= expect(registry.Games().size()==static_cast<size_t>(kCatalogGames),"1,000-game registry must discover every valid package");
    ok &= expect(registryMs<=10000,"1,000-game registry refresh exceeded 10 second CI budget");

    std::error_code ec; std::filesystem::remove_all(root,ec);
    std::cout << "METRIC strict_json_50000_ms=" << jsonMs << '\n';
    std::cout << "METRIC sqlite_full_250_sessions_ms=" << dbMs << '\n';
    std::cout << "METRIC sqlite_full_average_session_ms=" << dbAverageMs << '\n';
    std::cout << "METRIC sqlite_sessions_per_second=" << (dbMs>0?(static_cast<long long>(kTransactions)*1000LL/dbMs):static_cast<long long>(kTransactions)*1000LL) << '\n';
    std::cout << "METRIC registry_1000_games_ms=" << registryMs << '\n';
    if(!ok)return 1;
    std::cout << "PASS: ZERO production core performance and 1000-game scale benchmark\n";
    return 0;
}
