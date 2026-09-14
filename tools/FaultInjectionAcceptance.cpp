#include "FirstBootService.h"
#include "PlatformDatabase.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace zero;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}
}

int main() {
    bool ok = true;
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / (L"zero-fault-acceptance-" + std::to_wstring(stamp));
    std::filesystem::create_directories(root);

    const auto blocker = root / L"blocked";
    {
        std::ofstream out(blocker, std::ios::binary | std::ios::trunc);
        out << "sentinel";
    }

    std::wstring error;
    PlatformDatabase badDb(blocker / L"zero.db");
    const bool dbInitialized = badDb.Initialize(error);
    ok &= expect(!dbInitialized, "database initialization through a file parent must fail closed");
    ok &= expect(!error.empty(), "database failure must expose diagnostic error text");
    ok &= expect(std::filesystem::is_regular_file(blocker), "fault injection must not replace blocking file");

    FirstBootService badFirstBoot(blocker / L"firstboot");
    FirstBootState state;
    state.profileName = "Player";
    state.controllerConfirmed = true;
    state.displayConfirmed = true;
    state.audioConfirmed = true;
    state.completed = true;
    error.clear();
    const bool saved = badFirstBoot.Save(state, error);
    ok &= expect(!saved, "First Boot persistence through a file parent must fail closed");
    ok &= expect(!error.empty(), "First Boot storage failure must expose diagnostic error text");
    ok &= expect(std::filesystem::is_regular_file(blocker), "First Boot failure must not corrupt blocking file");

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    if (!ok) return 1;
    std::cout << "PASS: ZERO local storage fault-injection contract\n";
    return 0;
}
