#pragma once
#include <functional>
#include <string>

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
    bool Ping(std::wstring& error);

    void SetOverlayCallback(std::function<void(bool)> callback);
    bool Poll(std::wstring& error);

    bool IsConnected() const noexcept;
    const std::string& PackageId() const noexcept { return packageId_; }
    const std::string& SessionId() const noexcept { return sessionId_; }

private:
    void* pipe_{reinterpret_cast<void*>(-1)};
    std::string packageId_;
    std::string sessionId_;
    std::function<void(bool)> overlayCallback_;

    bool WriteLine(const std::string& line, std::wstring& error);
    bool ReadLine(std::string& line, std::wstring& error);
};

} // namespace zero::sdk
