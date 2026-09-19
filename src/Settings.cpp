#include "Settings.h"
#include "PlatformDatabase.h"
#include <algorithm>
#include <fstream>

namespace zero {
namespace {
int parseVolume(const std::string& value, int fallback) {
    try { return std::clamp(std::stoi(value), 0, 100); }
    catch (...) { return std::clamp(fallback, 0, 100); }
}
}

SettingsStore::SettingsStore(std::filesystem::path root) : root_(std::move(root)) {}

UserSettings SettingsStore::Load() const {
    UserSettings settings;
    PlatformDatabase database(root_ / L"Data" / L"zero.db");
    std::wstring error;

    const auto profile = database.GetSetting("profile", error);
    error.clear();
    const auto reducedMotion = database.GetSetting("reduced_motion", error);
    error.clear();
    const auto volume = database.GetSetting("volume", error);
    error.clear();
    const auto shareActivity = database.GetSetting("share_activity", error);
    error.clear();
    const auto shareAchievements = database.GetSetting("share_achievements", error);
    error.clear();
    const auto sharePlaytime = database.GetSetting("share_playtime", error);

    const bool hasCanonicalState = profile.has_value() || reducedMotion.has_value() || volume.has_value() ||
                                   shareActivity.has_value() || shareAchievements.has_value() || sharePlaytime.has_value();
    if (hasCanonicalState) {
        if (profile && !profile->empty()) settings.profileName = *profile;
        if (reducedMotion) settings.reducedMotion = *reducedMotion == "1";
        if (volume) settings.volume = parseVolume(*volume, settings.volume);
        if (shareActivity) settings.shareActivity = *shareActivity == "1";
        if (shareAchievements) settings.shareAchievements = *shareAchievements == "1";
        if (sharePlaytime) settings.sharePlaytime = *sharePlaytime == "1";
        return settings;
    }

    const auto legacyPath = root_ / "settings.ini";
    std::ifstream legacy(legacyPath);
    if (!legacy) return settings;

    std::string line;
    while (std::getline(legacy, line)) {
        if (line.rfind("profile=", 0) == 0) {
            const auto value = line.substr(8);
            if (!value.empty()) settings.profileName = value;
        } else if (line.rfind("reduced_motion=", 0) == 0) {
            settings.reducedMotion = line.substr(15) == "1";
        } else if (line.rfind("volume=", 0) == 0) {
            settings.volume = parseVolume(line.substr(7), settings.volume);
        } else if (line.rfind("share_activity=", 0) == 0) {
            settings.shareActivity = line.substr(15) == "1";
        } else if (line.rfind("share_achievements=", 0) == 0) {
            settings.shareAchievements = line.substr(19) == "1";
        } else if (line.rfind("share_playtime=", 0) == 0) {
            settings.sharePlaytime = line.substr(15) == "1";
        }
    }

    Save(settings);
    return settings;
}

bool SettingsStore::Save(const UserSettings& input) const {
    UserSettings settings = input;
    settings.volume = std::clamp(settings.volume, 0, 100);
    if (settings.profileName.empty()) settings.profileName = "Player";

    PlatformDatabase database(root_ / L"Data" / L"zero.db");
    std::wstring error;
    if (!database.SetSetting("profile", settings.profileName, error)) return false;
    if (!database.SetSetting("reduced_motion", settings.reducedMotion ? "1" : "0", error)) return false;
    if (!database.SetSetting("volume", std::to_string(settings.volume), error)) return false;
    if (!database.SetSetting("share_activity", settings.shareActivity ? "1" : "0", error)) return false;
    if (!database.SetSetting("share_achievements", settings.shareAchievements ? "1" : "0", error)) return false;
    if (!database.SetSetting("share_playtime", settings.sharePlaytime ? "1" : "0", error)) return false;

    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (ec) return false;
    const auto tmp = root_ / "settings.tmp";
    const auto dst = root_ / "settings.ini";
    std::ofstream f(tmp, std::ios::trunc);
    if (!f) return false;
    f << "profile=" << settings.profileName << "\n";
    f << "reduced_motion=" << (settings.reducedMotion ? 1 : 0) << "\n";
    f << "volume=" << settings.volume << "\n";
    f << "share_activity=" << (settings.shareActivity ? 1 : 0) << "\n";
    f << "share_achievements=" << (settings.shareAchievements ? 1 : 0) << "\n";
    f << "share_playtime=" << (settings.sharePlaytime ? 1 : 0) << "\n";
    f.close();
    std::filesystem::rename(tmp, dst, ec);
    if (ec) {
        std::filesystem::remove(dst, ec);
        ec.clear();
        std::filesystem::rename(tmp, dst, ec);
    }
    return !ec;
}
} // namespace zero
