#include "Settings.h"
#include <fstream>
#include <sstream>

namespace zero {
SettingsStore::SettingsStore(std::filesystem::path root) : root_(std::move(root)) {}

UserSettings SettingsStore::Load() const {
    UserSettings s;
    std::ifstream f(root_ / "settings.ini");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("profile=",0)==0) s.profileName = line.substr(8);
        else if (line.rfind("reduced_motion=",0)==0) s.reducedMotion = line.substr(15)=="1";
        else if (line.rfind("volume=",0)==0) s.volume = std::stoi(line.substr(7));
    }
    return s;
}

bool SettingsStore::Save(const UserSettings& s) const {
    std::error_code ec; std::filesystem::create_directories(root_, ec);
    const auto tmp = root_ / "settings.tmp";
    const auto dst = root_ / "settings.ini";
    std::ofstream f(tmp, std::ios::trunc);
    if (!f) return false;
    f << "profile=" << s.profileName << "\n";
    f << "reduced_motion=" << (s.reducedMotion?1:0) << "\n";
    f << "volume=" << s.volume << "\n";
    f.close();
    std::filesystem::rename(tmp, dst, ec);
    if (ec) {
        std::filesystem::remove(dst, ec);
        ec.clear(); std::filesystem::rename(tmp, dst, ec);
    }
    return !ec;
}
}
