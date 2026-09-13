#include "PlatformStateStore.h"
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
        std::filesystem::path out = std::filesystem::path(p) / "ZERO" / "PlatformState";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::temp_directory_path() / "ZERO" / "PlatformState";
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
std::string getString(const std::string& s, const std::string& key) {
    const auto p = s.find("\"" + key + "\""); if (p == std::string::npos) return {};
    const auto c = s.find(':', p); const auto q1 = s.find('"', c + 1); const auto q2 = s.find('"', q1 + 1);
    if (c == std::string::npos || q1 == std::string::npos || q2 == std::string::npos) return {};
    return s.substr(q1 + 1, q2 - q1 - 1);
}
uint64_t getUInt(const std::string& s, const std::string& key) {
    const auto p = s.find("\"" + key + "\""); if (p == std::string::npos) return 0;
    const auto c = s.find(':', p); if (c == std::string::npos) return 0;
    try { return std::stoull(s.substr(c + 1)); } catch (...) { return 0; }
}
bool getBool(const std::string& s, const std::string& key) {
    const auto p = s.find("\"" + key + "\""); if (p == std::string::npos) return false;
    const auto c = s.find(':', p); if (c == std::string::npos) return false;
    return s.find("true", c + 1) < s.find_first_of(",}\n", c + 1);
}
}

std::filesystem::path PlatformStateStore::PathFor(const std::string& packageId) const {
    return root() / (std::filesystem::path(packageId).wstring() + L".json");
}

GamePlatformState PlatformStateStore::Load(const std::string& packageId) const {
    GamePlatformState out{}; out.packageId = packageId;
    std::ifstream f(PathFor(packageId), std::ios::binary); if (!f) return out;
    std::ostringstream ss; ss << f.rdbuf(); const auto text = ss.str();
    out.totalPlaytimeSeconds = getUInt(text, "total_playtime_seconds");
    out.launchCount = getUInt(text, "launch_count");
    out.lastSessionId = getString(text, "last_session_id");
    out.lastPlayedAtUtc = getString(text, "last_played_at");
    out.lastExitCode = static_cast<uint32_t>(getUInt(text, "last_exit_code"));
    out.lastSessionCrashed = getBool(text, "last_session_crashed");
    return out;
}

bool PlatformStateStore::Save(const GamePlatformState& s, std::wstring& error) const {
    std::error_code ec; std::filesystem::create_directories(root(), ec);
    if (ec) { error = L"ZERO could not create the platform state directory."; return false; }
    const auto path = PathFor(s.packageId); const auto tmp = path.wstring() + L".tmp";
    std::ofstream f(std::filesystem::path(tmp), std::ios::binary | std::ios::trunc);
    if (!f) { error = L"ZERO could not write platform state."; return false; }
    f << "{\n"
      << "  \"schema\": 1,\n"
      << "  \"package_id\": \"" << s.packageId << "\",\n"
      << "  \"total_playtime_seconds\": " << s.totalPlaytimeSeconds << ",\n"
      << "  \"launch_count\": " << s.launchCount << ",\n"
      << "  \"last_session_id\": \"" << s.lastSessionId << "\",\n"
      << "  \"last_played_at\": \"" << s.lastPlayedAtUtc << "\",\n"
      << "  \"last_exit_code\": " << s.lastExitCode << ",\n"
      << "  \"last_session_crashed\": " << (s.lastSessionCrashed ? "true" : "false") << "\n"
      << "}\n";
    f.close();
    std::filesystem::rename(std::filesystem::path(tmp), path, ec);
    if (ec) { std::filesystem::remove(path, ec); ec.clear(); std::filesystem::rename(std::filesystem::path(tmp), path, ec); }
    if (ec) { error = L"ZERO could not atomically commit platform state."; return false; }
    return true;
}

bool PlatformStateStore::RecordSession(const std::string& packageId,
                                       const std::string& sessionId,
                                       uint64_t playtimeSeconds,
                                       uint32_t exitCode,
                                       bool crashed,
                                       std::wstring& error) const {
    auto s = Load(packageId);
    s.packageId = packageId;
    s.totalPlaytimeSeconds += playtimeSeconds;
    s.launchCount += 1;
    s.lastSessionId = sessionId;
    s.lastPlayedAtUtc = nowUtc();
    s.lastExitCode = exitCode;
    s.lastSessionCrashed = crashed;
    return Save(s, error);
}

} // namespace zero
