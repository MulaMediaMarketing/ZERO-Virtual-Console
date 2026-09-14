#include "FirstBootService.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}
}

int main() {
    using namespace zero;
    bool ok = true;

    const auto root = std::filesystem::temp_directory_path() /
        (L"zero-firstboot-acceptance-" + std::to_wstring(GetCurrentProcessId()));
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        std::cerr << "FAIL: could not create temp root\n";
        return 1;
    }

    FirstBootService service(root);

    {
        std::ofstream f(root / "first_boot.ini", std::ios::trunc);
        f << "completed=1\n";
        f << "profile=\n";
        f << "controller=1\n";
        f << "display=1\n";
        f << "audio=1\n";
        f << "volume=80\n";
    }
    auto malformed = service.Load();
    ok &= expect(!malformed.completed, "malformed persisted completion must fail closed");
    ok &= expect(service.IsRequired(), "malformed setup state must require First Boot");

    FirstBootState invalid{};
    invalid.completed = true;
    invalid.profileName.clear();
    invalid.controllerConfirmed = true;
    invalid.displayConfirmed = true;
    invalid.audioConfirmed = true;
    invalid.volume = 250;
    std::wstring error;
    ok &= expect(service.Save(invalid, error), "invalid state should save in normalized form");
    const auto normalized = service.Load();
    ok &= expect(!normalized.completed, "invalid completed state must persist as incomplete");
    ok &= expect(normalized.volume == 100, "volume must persist clamped to 100");

    FirstBootState valid{};
    valid.completed = true;
    valid.profileName = "Player One";
    valid.controllerConfirmed = true;
    valid.controllerDetected = true;
    valid.displayConfirmed = true;
    valid.audioConfirmed = true;
    valid.volume = 75;
    valid.displayWidth = 1920;
    valid.displayHeight = 1080;
    error.clear();
    ok &= expect(service.Save(valid, error), "valid First Boot state must persist");
    const auto loaded = service.Load();
    ok &= expect(loaded.completed, "valid persisted setup must remain complete");
    ok &= expect(!service.IsRequired(), "valid setup must not rerun First Boot");
    ok &= expect(loaded.profileName == "Player One", "profile must round trip");
    ok &= expect(loaded.volume == 75, "volume must round trip");
    ok &= expect(loaded.displayWidth == 1920 && loaded.displayHeight == 1080, "display metadata must round trip");

    std::filesystem::remove_all(root, ec);
    if (!ok) return 1;
    std::cout << "PASS: ZERO First Boot persistence\n";
    return 0;
}
