#pragma once
#include "ZeroProtocol.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace zero::sdk {

namespace detail {
bool InstallCrashHandler() noexcept;
}

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

    bool IsConnected() const noexcept { return connected_.load(); }
    const std::string& PackageId() const noexcept { return packageId_; }
    const std::string& SessionId() const noexcept { return sessionId_; }
    const std::optional<ResumeContext>& LaunchResume() const noexcept { return launchResume_; }

private:
    struct PendingResponse {
        std::mutex mutex;
        std::condition_variable cv;
        bool completed{false};
        bool failed{false};
        protocol::Message message;
    };

    bool crashHandlerInstalled_{detail::InstallCrashHandler()};
    void* pipe_{reinterpret_cast<void*>(-1)};
    std::wstring pipeName_;
    std::string authToken_;
    std::string packageId_;
    std::string sessionId_;
    std::optional<ResumeContext> launchResume_;
    std::function<void(bool)> overlayCallback_;

    std::atomic<uint64_t> nextRequestId_{1};
    std::atomic<bool> stop_{true};
    std::atomic<bool> connected_{false};
    std::thread receiverThread_;
    std::thread heartbeatThread_;
    std::mutex writeMutex_;
    std::mutex connectionMutex_;
    std::mutex pendingMutex_;
    std::mutex callbackMutex_;
    std::unordered_map<uint64_t, std::shared_ptr<PendingResponse>> pending_;

    bool OpenAndAuthenticate(unsigned timeoutMs, std::wstring& error);
    bool TryReconnect();
    void CloseTransport();
    void ReceiverLoop();
    void HeartbeatLoop();
    void DispatchMessage(const protocol::Message& message);
    void FailAllPending();
    bool WriteFrame(const protocol::Message& message);

    bool SendRequest(protocol::MessageType type,
                     std::vector<std::string> fields,
                     protocol::MessageType expectedType,
                     const std::string& expectedAck,
                     std::wstring& error);
};

} // namespace zero::sdk
