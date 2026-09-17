#include "features/settings/SettingsFeature.h"
#include <algorithm>

namespace zero::features::settings {

SettingsViewModel SettingsController::Build(const UserSettings& settings) {
    return SettingsViewModel{
        settings.profileName,
        std::clamp(settings.volume, 0, 100),
        settings.reducedMotion,
        settings.shareActivity,
        settings.shareAchievements,
        settings.sharePlaytime
    };
}

std::string SettingsView::PrivacySummary(const SettingsViewModel& model) {
    if (!model.shareActivity && !model.shareAchievements && !model.sharePlaytime) return "PRIVATE";
    if (model.shareActivity && model.shareAchievements && model.sharePlaytime) return "SHARING ENABLED";
    return "CUSTOM PRIVACY";
}

} // namespace zero::features::settings
