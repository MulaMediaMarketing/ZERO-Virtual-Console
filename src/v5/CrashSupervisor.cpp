#include "v5/CrashSupervisor.h"

namespace zero::v5 {

std::optional<CrashEvent> CrashSupervisor::Observe(const RuntimeSnapshot& snapshot,
                                                   CaptureDump captureDump) {
    if (snapshot.state != RuntimeLifecycleState::Failed ||
        snapshot.identity.sessionId.empty() ||
        snapshot.identity.packageId.empty()) {
        return std::nullopt;
    }
    if (!reportedSessions_.insert(snapshot.identity.sessionId).second) return std::nullopt;

    CrashEvent event;
    event.sessionId = snapshot.identity.sessionId;
    event.packageId = snapshot.identity.packageId;
    event.accountId = snapshot.identity.accountId;
    event.exitCode = snapshot.exitCode;
    if (captureDump) event.dumpCaptured = captureDump();
    return event;
}

void CrashSupervisor::Reset(const std::string& sessionId) {
    if (!sessionId.empty()) reportedSessions_.erase(sessionId);
}

} // namespace zero::v5
