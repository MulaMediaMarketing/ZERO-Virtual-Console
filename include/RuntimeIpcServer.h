#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <windows.h>

namespace zero {

struct RuntimeIpcCallbacks {
    std::function<void()> onReady;
    std::function<void(const std::string&, const std::string&, const std::string&)> onResume;
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

private:
    void ServerLoop();
    bool HandleLine(const std::string& line, HANDLE pipe);
    bool WriteLine(HANDLE pipe, const std::string& line);

    std::wstring pipeName_;
    std::string packageId_;
    std::string authToken_;
    RuntimeIpcCallbacks callbacks_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> authenticated_{false};
    std::atomic<bool> ready_{false};
    mutable std::mutex pipeMutex_;
    HANDLE activePipe_{INVALID_HANDLE_VALUE};
};

} // namespace zero
