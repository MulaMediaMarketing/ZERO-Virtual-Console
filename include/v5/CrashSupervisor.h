#pragma once

#include "RuntimeAuthority.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>

namespace zero::v5 {

struct CrashEvent {
    std::string sessionId;
    std::string packageId;
    std::string accountId;
    std::uint32_t exitCode{0};
    bool dumpCaptured{false};
};

class CrashSupervisor {
public:
    using CaptureDump = std::function<bool()>;

    std::optional<CrashEvent> Observe(const RuntimeSnapshot& snapshot,
                                      CaptureDump captureDump);
    void Reset(const std::string& sessionId);

private:
    std::unordered_set<std::string> reportedSessions_;
};

} // namespace zero::v5
