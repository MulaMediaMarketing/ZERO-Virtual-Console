#pragma once
#include <filesystem>
#include <string>

namespace zero {
struct UserSettings {
    std::string profileName{"Player"};
    bool reducedMotion{false};
    int volume{80};
    bool shareActivity{false};
    bool shareAchievements{false};
    bool sharePlaytime{false};
};

class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path root);
    UserSettings Load() const;
    bool Save(const UserSettings& settings) const;
private:
    std::filesystem::path root_;
};
}
