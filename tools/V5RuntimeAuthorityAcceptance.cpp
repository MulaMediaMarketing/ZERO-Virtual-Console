#include "v5/RuntimeAuthority.h"
#include <iostream>
#include <optional>
#include <string>

using namespace zero::v5;

namespace {
struct Policy final : ILaunchPolicy {
    bool allow{true};
    bool Validate(const RuntimeSessionRequest&, std::string& error) const override {
        if (!allow) { error = "policy denied"; return false; }
        return true;
    }
};

struct Broker final : ICapabilityBroker {
    bool escalate{false};
    std::optional<RuntimeSessionGrant> Grant(const RuntimeSessionRequest& request,
                                             std::string&) const override {
        RuntimeSessionGrant grant{request.identity, request.requestedCapabilities, "ipc-token"};
        if (escalate) grant.grantedCapabilities.push_back(RuntimeCapability::CloudState);
        return grant;
    }
};

struct Host final : IRuntimeProcessHost {
    bool startOk{true};
    bool started{false};
    bool terminated{false};
    RuntimeProcessStatus status{RuntimeProcessState::Running, 0};

    bool Start(const LaunchDescriptor&, const RuntimeSessionGrant&, std::string& error) override {
        if (!startOk) { error = "process start failed"; return false; }
        started = true;
        return true;
    }
    RuntimeProcessStatus Poll() override { return status; }
    void Terminate() override { terminated = true; }
};

RuntimeSessionRequest request() {
    RuntimeSessionRequest value;
    value.identity = {"session-1", "pkg.game", "acct-1"};
    value.launch.packageId = "pkg.game";
    value.launch.version = "1.0.0";
    value.launch.executable = "game.exe";
    value.launch.sessionToken = "session-token";
    value.launch.entitlementToken = "entitlement-token";
    value.requestedCapabilities = {RuntimeCapability::Input, RuntimeCapability::SaveRead};
    return value;
}
}

int main() {
    Policy policy;
    Broker broker;
    Host host;
    RuntimeAuthority runtime(policy, broker, host);

    auto invalid = request();
    invalid.identity.packageId = "pkg.other";
    if (runtime.Start(invalid).failure != RuntimeStartFailure::InvalidIdentity) return 10;

    auto noAuthority = request();
    noAuthority.launch.sessionToken.clear();
    if (runtime.Start(noAuthority).failure != RuntimeStartFailure::InvalidLaunchAuthority) return 11;

    auto duplicate = request();
    duplicate.requestedCapabilities.push_back(RuntimeCapability::Input);
    if (runtime.Start(duplicate).failure != RuntimeStartFailure::InvalidCapabilityRequest) return 12;

    broker.escalate = true;
    if (runtime.Start(request()).failure != RuntimeStartFailure::CapabilityEscalation) return 13;
    broker.escalate = false;

    const auto started = runtime.Start(request());
    if (!started.Started() || !host.started) return 20;
    if (runtime.Snapshot().state != RuntimeLifecycleState::Running) return 21;

    if (runtime.Start(request()).failure != RuntimeStartFailure::StateConflict) return 22;

    host.status = {RuntimeProcessState::Exited, 7};
    const auto exited = runtime.Poll();
    if (exited.state != RuntimeLifecycleState::Exited || exited.exitCode != 7) return 23;

    Host secondHost;
    RuntimeAuthority second(policy, broker, secondHost);
    if (!second.Start(request()).Started()) return 30;
    second.Terminate();
    if (!secondHost.terminated || !second.Snapshot().forcedTermination ||
        second.Snapshot().state != RuntimeLifecycleState::Exited) return 31;

    std::cout << "ZERO V5 runtime authority acceptance: PASS\n";
    return 0;
}
