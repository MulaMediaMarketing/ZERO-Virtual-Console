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
    void SetLaunchResume(std::optional<zero::ResumeMetadata> resume) {
        if (resume) {
            resume->activityId = HexEncode(resume->activityId);
            resume->displayLabel = HexEncode(resume->displayLabel);
            resume->payload = HexEncode(resume->payload);
        }
        launchResume_ = std::move(resume);
    }
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

    static std::string HexEncode(const std::string& value) {
        static constexpr char lut[] = "0123456789abcdef";
        std::string encoded;
        encoded.reserve(value.size() * 2);
        for (const unsigned char byte : value) {
            encoded.push_back(lut[(byte >> 4) & 0x0f]);
            encoded.push_back(lut[byte & 0x0f]);
        }
        return encoded;
    }

    static bool ValidateGrant(const LaunchDescriptor& launch,
                              const RuntimeSessionGrant& grant,
                              std::string& error);
};

} // namespace zero::v5
