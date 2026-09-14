#include "AchievementStore.h"
#include "PlatformDatabase.h"
#include "PlatformPaths.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <objbase.h>
#include <sstream>

namespace zero {
namespace {
std::string nowUtc() {
    const auto tp = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm utc{};
    gmtime_s(&utc, &tt);
    std::ostringstream os;
    os << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}
std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}
}

std::filesystem::path AchievementStore::PathFor(const std::string& packageId) const {
    return PlatformPaths::AchievementsMirrorRoot() / (std::filesystem::path(packageId).wstring() + L".jsonl");
}

std::vector<AchievementRecord> AchievementStore::Load(const std::string& packageId) const {
    std::vector<AchievementRecord> out;
    std::ifstream f(PathFor(packageId), std::ios::binary);
    if (!f) return out;
    std::string line;
    while (std::getline(f, line)) {
        const auto idPos = line.find("\"id\":\"");
        const auto titlePos = line.find("\"title\":\"");
        const auto timePos = line.find("\"unlocked_at\":\"");
        if (idPos == std::string::npos || titlePos == std::string::npos || timePos == std::string::npos) continue;
        auto read = [&](size_t p, const char* key) {
            p += std::char_traits<char>::length(key);
            const auto e = line.find('"', p);
            return e == std::string::npos ? std::string{} : line.substr(p, e - p);
        };
        out.push_back({read(idPos, "\"id\":\""), read(titlePos, "\"title\":\""), read(timePos, "\"unlocked_at\":\"")});
    }
    return out;
}

bool AchievementStore::Unlock(const std::string& packageId,
                              const std::string& achievementId,
                              const std::string& title,
                              std::wstring& error) const {
    if (packageId.empty() || achievementId.empty()) {
        error = L"Achievement identity is invalid.";
        return false;
    }

    PlatformDatabase database;
    for (const auto& existing : Load(packageId)) {
        if (existing.id == achievementId) {
            return database.UpsertAchievement(packageId, achievementId,
                                              existing.title.empty() ? title : existing.title,
                                              existing.unlockedAtUtc, error);
        }
    }

    const auto stamp = nowUtc();
    if (!database.UpsertAchievement(packageId, achievementId, title, stamp, error)) return false;

    std::error_code ec;
    std::filesystem::create_directories(PlatformPaths::AchievementsMirrorRoot(), ec);
    if (ec) { error = L"ZERO could not create the achievement mirror directory."; return false; }
    std::ofstream f(PathFor(packageId), std::ios::binary | std::ios::app);
    if (!f) { error = L"ZERO could not persist the achievement mirror."; return false; }
    f << "{\"id\":\"" << escapeJson(achievementId) << "\",\"title\":\"" << escapeJson(title)
      << "\",\"unlocked_at\":\"" << stamp << "\"}\n";
    if (!f.good()) { error = L"ZERO could not finalize the achievement mirror."; return false; }
    return true;
}

} // namespace zero
