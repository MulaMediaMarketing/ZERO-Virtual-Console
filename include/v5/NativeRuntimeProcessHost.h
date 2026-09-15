#pragma once

#include "ResumeStore.h"
#include "RuntimeAuthority.h"
#include "RuntimeIpcServer.h"
#include "RuntimeSession.h"
#include <chrono>
#include <optional>
#include <string>
#include <utility>

namespace zero::v5 {

class NativeRuntimeProcessHost final : public IRuntimeProcessHost {
public:
    explicit NativeRuntimeProcessHost(zero::RuntimeIpcCallbacks callbacks = {})
        : callbacks_(std::move(callbacks)) {}

    bool Start(const LaunchDescriptor& launch,
               const RuntimeSessionGrant& grant,
               std::string& error) override;
    RuntimeProcessStatus Poll() override;
    void Terminate() override;

    void SetCallbacks(zero::RuntimeIpcCallbacks callbacks) { callbacks_ = std::move(callbacks); }
    void SetLaunchResume(std::optional<zero::ResumeMetadata> resume) { launchResume_ = std::move(resume); }
    bool SendOverlayFocus(bool visible) { return ipc_.SendOverlayFocus(visible); }
    std::chrono::milliseconds ClientSilence() const noexcept { return ipc_.ClientSilence(); }
    bool CaptureDiagnosticDump() { return process_.CaptureDiagnosticDump(); }
    bool ClientAuthenticated() const noexcept { return ipc_.ClientAuthenticated(); }
    bool ReadyReceived() const noexcept { return ipc_.ReadyReceived(); }
    const std::wstring& PipeName() const noexcept { return ipc_.PipeName(); }
    const zero::RuntimeSessionInfo& ProcessInfo() const noexcept { return process_.Info(); }

private:
    zero::RuntimeSession process_;
    zero::RuntimeIpcServer ipc_;
    zero::RuntimeIpcCallbacks callbacks_{};
    RuntimeSessionGrant activeGrant_{};
    std::optional<zero::ResumeMetadata> launchResume_;
    bool started_{false};

    static bool ValidateGrant(const LaunchDescriptor& launch,
                              const RuntimeSessionGrant& grant,
                              std::string& error);
};

} // namespace zero::v5
