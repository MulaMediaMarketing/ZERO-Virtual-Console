#pragma once

#include "Settings.h"
#include <string>

namespace zero::features::settings {

struct SettingsViewModel {
    std::string profileName;
    int volume{100};
    bool reducedMotion{false};
    bool shareActivity{false};
    bool shareAchievements{false};
    bool sharePlaytime{false};
};

class SettingsController final {
public:
    static SettingsViewModel Build(const UserSettings& settings);
};

class SettingsView final {
public:
    static std::string PrivacySummary(const SettingsViewModel& model);
};

} // namespace zero::features::settings
