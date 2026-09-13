#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace zero {

struct GamePlatformState {
    std::string packageId;
    uint64_t totalPlaytimeSeconds{0};
    uint64_t launchCount{0};
    std::string lastSessionId;
    std::string lastPlayedAtUtc;
    uint32_t lastExitCode{0};
    bool lastSessionCrashed{false};
};

class PlatformStateStore {
public:
    GamePlatformState Load(const std::string& packageId) const;
    bool Save(const GamePlatformState& state, std::wstring& error) const;
    bool RecordSession(const std::string& packageId,
                       const std::string& sessionId,
                       uint64_t playtimeSeconds,
                       uint32_t exitCode,
                       bool crashed,
                       std::wstring& error) const;
private:
    std::filesystem::path PathFor(const std::string& packageId) const;
};

} // namespace zero
