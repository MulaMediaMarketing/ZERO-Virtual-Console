#pragma once

#include "RuntimeCore.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace zero::v5 {

enum class RuntimeLifecycleState : std::uint8_t {
    Idle,
    Starting,
    Running,
    Exited,
    Failed,
    Terminating
};

enum class RuntimeStartFailure : std::uint8_t {
    None,
    StateConflict,
    InvalidIdentity,
    InvalidLaunchAuthority,
    InvalidCapabilityRequest,
    PolicyDenied,
    CapabilityDenied,
    CapabilityEscalation,
    ProcessStartFailed
};

enum class RuntimeProcessState : std::uint8_t {
    Starting,
    Running,
    Exited,
    Crashed
};

struct RuntimeProcessStatus {
    RuntimeProcessState state{RuntimeProcessState::Starting};
    std::uint32_t exitCode{0};
};

struct RuntimeStartResult {
    RuntimeStartFailure failure{RuntimeStartFailure::None};
    std::optional<RuntimeSessionGrant> grant;
    std::string detail;

    bool Started() const noexcept {
        return failure == RuntimeStartFailure::None && grant.has_value();
    }
};

struct RuntimeSnapshot {
    RuntimeLifecycleState state{RuntimeLifecycleState::Idle};
    RuntimeSessionIdentity identity{};
    std::uint32_t exitCode{0};
    bool forcedTermination{false};
};

class IRuntimeProcessHost {
public:
    virtual ~IRuntimeProcessHost() = default;
    virtual bool Start(const LaunchDescriptor& launch,
                       const RuntimeSessionGrant& grant,
                       std::string& error) = 0;
    virtual RuntimeProcessStatus Poll() = 0;
    virtual void Terminate() = 0;
};

class RuntimeAuthority {
public:
    RuntimeAuthority(ILaunchPolicy& launchPolicy,
                     ICapabilityBroker& capabilityBroker,
                     IRuntimeProcessHost& processHost)
        : launchPolicy_(launchPolicy), capabilityBroker_(capabilityBroker), processHost_(processHost) {}

    RuntimeStartResult Start(const RuntimeSessionRequest& request);
    RuntimeSnapshot Poll();
    void Terminate();
    const RuntimeSnapshot& Snapshot() const noexcept { return snapshot_; }

private:
    ILaunchPolicy& launchPolicy_;
    ICapabilityBroker& capabilityBroker_;
    IRuntimeProcessHost& processHost_;
    RuntimeSnapshot snapshot_{};

    static bool ValidIdentity(const RuntimeSessionRequest& request);
    static bool ValidLaunchAuthority(const RuntimeSessionRequest& request);
    static bool ValidCapabilityRequest(const std::vector<RuntimeCapability>& requested);
    static bool GrantIsSubset(const RuntimeSessionRequest& request,
                              const RuntimeSessionGrant& grant);
};

} // namespace zero::v5
