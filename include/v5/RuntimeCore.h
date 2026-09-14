#pragma once

#include "ContentModel.h"
#include <optional>
#include <string>
#include <vector>

namespace zero::v5 {

enum class RuntimeCapability : unsigned char {
    SaveRead,
    SaveWrite,
    Achievements,
    Overlay,
    Input,
    Capture,
    Presence,
    CloudState
};

struct RuntimeSessionIdentity {
    std::string sessionId;
    std::string packageId;
    std::string accountId;
};

struct RuntimeSessionRequest {
    LaunchDescriptor launch;
    RuntimeSessionIdentity identity;
    std::vector<RuntimeCapability> requestedCapabilities;
};

struct RuntimeSessionGrant {
    RuntimeSessionIdentity identity;
    std::vector<RuntimeCapability> grantedCapabilities;
    std::string ipcAuthenticationToken;
};

class ILaunchPolicy {
public:
    virtual ~ILaunchPolicy() = default;
    virtual bool Validate(const RuntimeSessionRequest& request, std::string& error) const = 0;
};

class ICapabilityBroker {
public:
    virtual ~ICapabilityBroker() = default;
    virtual std::optional<RuntimeSessionGrant> Grant(const RuntimeSessionRequest& request,
                                                     std::string& error) const = 0;
};

class IProcessSupervisor {
public:
    virtual ~IProcessSupervisor() = default;
    virtual bool Launch(const LaunchDescriptor& descriptor,
                        const RuntimeSessionGrant& grant,
                        std::string& error) = 0;
};

class RuntimeCore {
public:
    RuntimeCore(ILaunchPolicy& policy,
                ICapabilityBroker& capabilities,
                IProcessSupervisor& processes)
        : policy_(policy), capabilities_(capabilities), processes_(processes) {}

    std::optional<RuntimeSessionGrant> Start(const RuntimeSessionRequest& request,
                                             std::string& error) {
        if (!policy_.Validate(request, error)) return std::nullopt;
        auto grant = capabilities_.Grant(request, error);
        if (!grant) return std::nullopt;
        if (!processes_.Launch(request.launch, *grant, error)) return std::nullopt;
        return grant;
    }

private:
    ILaunchPolicy& policy_;
    ICapabilityBroker& capabilities_;
    IProcessSupervisor& processes_;
};

} // namespace zero::v5
