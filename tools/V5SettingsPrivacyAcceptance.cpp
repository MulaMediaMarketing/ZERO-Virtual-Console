#include "Settings.h"
#include <filesystem>
#include <iostream>

using namespace zero;

int main() {
    const auto root = std::filesystem::temp_directory_path() / L"zero-v5-settings-privacy-acceptance";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    SettingsStore store(root);
    const auto defaults = store.Load();
    if (defaults.shareActivity || defaults.shareAchievements || defaults.sharePlaytime) return 10;

    UserSettings saved;
    saved.profileName = "PrivacyTester";
    saved.volume = 65;
    saved.reducedMotion = true;
    saved.shareActivity = true;
    saved.shareAchievements = false;
    saved.sharePlaytime = true;
    if (!store.Save(saved)) return 11;

    const auto loaded = store.Load();
    if (loaded.profileName != "PrivacyTester") return 12;
    if (loaded.volume != 65 || !loaded.reducedMotion) return 13;
    if (!loaded.shareActivity || loaded.shareAchievements || !loaded.sharePlaytime) return 14;

    saved.shareActivity = false;
    saved.shareAchievements = false;
    saved.sharePlaytime = false;
    if (!store.Save(saved)) return 15;
    const auto privateAgain = store.Load();
    if (privateAgain.shareActivity || privateAgain.shareAchievements || privateAgain.sharePlaytime) return 16;

    std::filesystem::remove_all(root, ec);
    std::cout << "ZERO V5 settings privacy persistence acceptance: PASS\n";
    return 0;
}
