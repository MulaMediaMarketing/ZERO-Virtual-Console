#pragma once
#include "ZeroProtocol.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace zero::sdk {

struct ResumeContext {
    std::string activityId;
    std::string displayLabel;
    std::string payload;
};

class Client {
public:
    Client();
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    bool Initialize(std::wstring& error, unsigned timeoutMs = 5000);
    void Shutdown();
    bool ReportReady(std::wstring& error);
    bool SetResumeActivity(const ResumeContext& context, std::wstring& error);
    bool UnlockAchievement(const std::string& achievementId,
                           const std::string& title,
                           std::wstring& error);
    bool Ping(std::wstring& error);

    void SetOverlayCallback(std::function<void(bool)> callback);
    bool Poll(std::wstring& error);

    bool IsConnected() const noexcept;
    const std::string& PackageId() const noexcept { return packageId_; }
    const std::string& SessionId() const noexcept { return sessionId_; }
    const std::optional<ResumeContext>& LaunchResume() const noexcept { return launchResume_; }

private:
    void* pipe_{reinterpret_cast<void*>(-1)};
    std::string packageId_;
    std::string sessionId_;
    std::optional<ResumeContext> launchResume_;
    std::function<void(bool)> overlayCallback_;
    uint64_t nextRequestId_{1};
    std::mutex ioMutex_;
    std::atomic<bool> heartbeatStop_{true};
    std::thread heartbeatThread_;

    bool SendRequest(protocol::MessageType type,
                     std::vector<std::string> fields,
                     protocol::MessageType expectedType,
                     const std::string& expectedAck,
                     std::wstring& error);
    void HeartbeatLoop();
};

} // namespace zero::sdk
