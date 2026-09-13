#pragma once
#include "ZeroProtocol.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <windows.h>

namespace zero {

struct RuntimeIpcCallbacks {
    std::function<void()> onReady;
    std::function<bool(const std::string&, const std::string&, const std::string&)> onResume;
    std::function<bool(const std::string&, const std::string&)> onAchievement;
    std::function<void(bool)> onOverlayFocus;
};

class RuntimeIpcServer {
public:
    RuntimeIpcServer();
    ~RuntimeIpcServer();
    RuntimeIpcServer(const RuntimeIpcServer&) = delete;
    RuntimeIpcServer& operator=(const RuntimeIpcServer&) = delete;

    bool Start(const std::string& sessionId,
               const std::string& packageId,
               const std::string& authToken,
               RuntimeIpcCallbacks callbacks,
               std::wstring& error);
    void Stop();
    bool SendOverlayFocus(bool focused);

    const std::wstring& PipeName() const noexcept { return pipeName_; }
    bool ClientAuthenticated() const noexcept { return authenticated_.load(); }
    bool ReadyReceived() const noexcept { return ready_.load(); }
    std::chrono::milliseconds ClientSilence() const noexcept;

private:
    void ServerLoop();
    bool HandleMessage(const protocol::Message& message, HANDLE pipe);
    bool Send(HANDLE pipe, protocol::MessageType type, uint64_t requestId,
              std::vector<std::string> fields = {});
    void TouchClientActivity() noexcept;

    std::wstring pipeName_;
    std::string packageId_;
    std::string authToken_;
    RuntimeIpcCallbacks callbacks_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> authenticated_{false};
    std::atomic<bool> ready_{false};
    std::atomic<uint64_t> nextServerRequestId_{1};
    std::atomic<int64_t> lastClientActivityMs_{0};
    mutable std::mutex pipeMutex_;
    std::mutex writeMutex_;
    HANDLE activePipe_{INVALID_HANDLE_VALUE};
};

} // namespace zero
