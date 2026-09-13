#include "FirstBootService.h"
#include <fstream>

namespace zero {

FirstBootService::FirstBootService(std::filesystem::path zeroRoot) : root_(std::move(zeroRoot)) {}

std::filesystem::path FirstBootService::StatePath() const {
    return root_ / "first_boot.ini";
}

FirstBootState FirstBootService::Load() const {
    FirstBootState state;
    std::ifstream f(StatePath());
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("completed=", 0) == 0) state.completed = line.substr(10) == "1";
        else if (line.rfind("profile=", 0) == 0) state.profileName = line.substr(8);
        else if (line.rfind("controller=", 0) == 0) state.controllerConfirmed = line.substr(11) == "1";
        else if (line.rfind("display=", 0) == 0) state.displayConfirmed = line.substr(8) == "1";
        else if (line.rfind("audio=", 0) == 0) state.audioConfirmed = line.substr(6) == "1";
    }
    return state;
}

bool FirstBootService::Save(const FirstBootState& state, std::wstring& error) const {
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (ec) { error = L"ZERO could not create its first-boot state directory."; return false; }

    const auto tmp = root_ / "first_boot.tmp";
    const auto dst = StatePath();
    std::ofstream f(tmp, std::ios::trunc);
    if (!f) { error = L"ZERO could not write first-boot state."; return false; }
    f << "completed=" << (state.completed ? 1 : 0) << "\n";
    f << "profile=" << state.profileName << "\n";
    f << "controller=" << (state.controllerConfirmed ? 1 : 0) << "\n";
    f << "display=" << (state.displayConfirmed ? 1 : 0) << "\n";
    f << "audio=" << (state.audioConfirmed ? 1 : 0) << "\n";
    f.close();

    std::filesystem::rename(tmp, dst, ec);
    if (ec) {
        std::filesystem::remove(dst, ec);
        ec.clear();
        std::filesystem::rename(tmp, dst, ec);
    }
    if (ec) { error = L"ZERO could not finalize first-boot state."; return false; }
    return true;
}

bool FirstBootService::IsRequired() const {
    const auto state = Load();
    return !state.completed || !state.controllerConfirmed || !state.displayConfirmed || !state.audioConfirmed;
}

} // namespace zero
