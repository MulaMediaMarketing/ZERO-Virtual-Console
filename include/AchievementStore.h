#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace zero {

struct AchievementRecord {
    std::string id;
    std::string title;
    std::string unlockedAtUtc;
};

class AchievementStore {
public:
    bool Unlock(const std::string& packageId,
                const std::string& achievementId,
                const std::string& title,
                std::wstring& error) const;
    std::vector<AchievementRecord> Load(const std::string& packageId) const;
private:
    std::filesystem::path PathFor(const std::string& packageId) const;
};

} // namespace zero
