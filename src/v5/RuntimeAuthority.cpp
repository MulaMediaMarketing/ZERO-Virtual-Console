#include "v5/RuntimeAuthority.h"
#include <algorithm>
#include <set>

namespace zero::v5 {

namespace {
bool hasCapability(const std::vector<RuntimeCapability>& values, RuntimeCapability capability) {
    return std::find(values.begin(), values.end(), capability) != values.end();
}
}

bool RuntimeAuthority::ValidIdentity(const RuntimeSessionRequest& request) {
    return !request.identity.sessionId.empty() &&
           !request.identity.packageId.empty() &&
           !request.identity.accountId.empty() &&
           request.identity.packageId == request.launch.packageId;
}

bool RuntimeAuthority::ValidLaunchAuthority(const RuntimeSessionRequest& request) {
    return !request.launch.sessionToken.empty() &&
           !request.launch.entitlementToken.empty() &&
           !request.launch.packageId.empty() &&
           !request.launch.version.empty() &&
           !request.launch.executable.empty();
}

bool RuntimeAuthority::ValidCapabilityRequest(const std::vector<RuntimeCapability>& requested) {
    std::set<RuntimeCapability> unique;
    for (const auto capability : requested) {
        if (!unique.insert(capability).second) return false;
    }
    return true;
}

bool RuntimeAuthority::GrantIsSubset(const RuntimeSessionRequest& request,
                                     const RuntimeSessionGrant& grant) {
    if (grant.identity.sessionId != request.identity.sessionId ||
        grant.identity.packageId != request.identity.packageId ||
        grant.identity.accountId != request.identity.accountId ||
        grant.ipcAuthenticationToken.empty()) return false;

    for (const auto capability : grant.grantedCapabilities) {
        if (!hasCapability(request.requestedCapabilities, capability)) return false;
    }
    return true;
}

RuntimeStartResult RuntimeAuthority::Start(const RuntimeSessionRequest& request) {
    RuntimeStartResult result;
    if (snapshot_.state == RuntimeLifecycleState::Starting ||
        snapshot_.state == RuntimeLifecycleState::Running ||
        snapshot_.state == RuntimeLifecycleState::Terminating) {
        result.failure = RuntimeStartFailure::StateConflict;
        result.detail = "runtime session already active";
        return result;
    }
    if (!ValidIdentity(request)) {
        result.failure = RuntimeStartFailure::InvalidIdentity;
        result.detail = "runtime identity does not match launch package";
        return result;
    }
    if (!ValidLaunchAuthority(request)) {
        result.failure = RuntimeStartFailure::InvalidLaunchAuthority;
        result.detail = "authoritative launch tokens or executable metadata are missing";
        return result;
    }
    if (!ValidCapabilityRequest(request.requestedCapabilities)) {
        result.failure = RuntimeStartFailure::InvalidCapabilityRequest;
        result.detail = "runtime requested duplicate capabilities";
        return result;
    }

    std::string error;
    if (!launchPolicy_.Validate(request, error)) {
        result.failure = RuntimeStartFailure::PolicyDenied;
        result.detail = error.empty() ? "runtime launch policy denied session" : error;
        return result;
    }

    auto grant = capabilityBroker_.Grant(request, error);
    if (!grant) {
        result.failure = RuntimeStartFailure::CapabilityDenied;
        result.detail = error.empty() ? "runtime capability broker denied session" : error;
        return result;
    }
    if (!GrantIsSubset(request, *grant)) {
        result.failure = RuntimeStartFailure::CapabilityEscalation;
        result.detail = "runtime capability grant exceeds request or changes session identity";
        return result;
    }

    snapshot_ = {};
    snapshot_.state = RuntimeLifecycleState::Starting;
    snapshot_.identity = request.identity;

    if (!processHost_.Start(request.launch, *grant, error)) {
        snapshot_.state = RuntimeLifecycleState::Failed;
        result.failure = RuntimeStartFailure::ProcessStartFailed;
        result.detail = error.empty() ? "runtime process failed to start" : error;
        return result;
    }

    snapshot_.state = RuntimeLifecycleState::Running;
    result.grant = std::move(grant);
    return result;
}

RuntimeSnapshot RuntimeAuthority::Poll() {
    if (snapshot_.state != RuntimeLifecycleState::Running &&
        snapshot_.state != RuntimeLifecycleState::Starting) return snapshot_;

    const auto status = processHost_.Poll();
    switch (status.state) {
        case RuntimeProcessState::Starting:
            snapshot_.state = RuntimeLifecycleState::Starting;
            break;
        case RuntimeProcessState::Running:
            snapshot_.state = RuntimeLifecycleState::Running;
            break;
        case RuntimeProcessState::Exited:
            snapshot_.state = RuntimeLifecycleState::Exited;
            snapshot_.exitCode = status.exitCode;
            break;
        case RuntimeProcessState::Crashed:
            snapshot_.state = RuntimeLifecycleState::Failed;
            snapshot_.exitCode = status.exitCode;
            break;
    }
    return snapshot_;
}

void RuntimeAuthority::Terminate() {
    if (snapshot_.state != RuntimeLifecycleState::Running &&
        snapshot_.state != RuntimeLifecycleState::Starting) return;
    snapshot_.state = RuntimeLifecycleState::Terminating;
    processHost_.Terminate();
    snapshot_.forcedTermination = true;
    snapshot_.state = RuntimeLifecycleState::Exited;
}

} // namespace zero::v5
