#include "AchievementStore.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <objbase.h>
#include <shlobj.h>
#include <sstream>

namespace zero {
namespace {
std::filesystem::path root() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO" / "Achievements";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::temp_directory_path() / "ZERO" / "Achievements";
}
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
    return root() / (std::filesystem::path(packageId).wstring() + L".jsonl");
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
    for (const auto& a : Load(packageId)) if (a.id == achievementId) return true;
    std::error_code ec;
    std::filesystem::create_directories(root(), ec);
    if (ec) { error = L"ZERO could not create the achievement directory."; return false; }
    std::ofstream f(PathFor(packageId), std::ios::binary | std::ios::app);
    if (!f) { error = L"ZERO could not persist the achievement."; return false; }
    f << "{\"id\":\"" << escapeJson(achievementId) << "\",\"title\":\"" << escapeJson(title)
      << "\",\"unlocked_at\":\"" << nowUtc() << "\"}\n";
    return true;
}

} // namespace zero
