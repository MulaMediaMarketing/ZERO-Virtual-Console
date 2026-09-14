#pragma once

#include "RuntimeAuthority.h"
#include "RuntimeIpcServer.h"
#include "RuntimeSession.h"
#include <chrono>
#include <string>

namespace zero::v5 {

class NativeRuntimeProcessHost final : public IRuntimeProcessHost {
public:
    bool Start(const LaunchDescriptor& launch,
               const RuntimeSessionGrant& grant,
               std::string& error) override;
    RuntimeProcessStatus Poll() override;
    void Terminate() override;

    bool ClientAuthenticated() const noexcept { return ipc_.ClientAuthenticated(); }
    bool ReadyReceived() const noexcept { return ipc_.ReadyReceived(); }
    const std::wstring& PipeName() const noexcept { return ipc_.PipeName(); }

private:
    zero::RuntimeSession process_;
    zero::RuntimeIpcServer ipc_;
    RuntimeSessionGrant activeGrant_{};
    bool started_{false};

    static bool ValidateGrant(const LaunchDescriptor& launch,
                              const RuntimeSessionGrant& grant,
                              std::string& error);
};

} // namespace zero::v5
