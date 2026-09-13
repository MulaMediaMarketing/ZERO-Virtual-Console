#include "ZeroSdk.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace zero::sdk {
namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::string envUtf8(const wchar_t* key) {
    wchar_t buffer[32768]{};
    DWORD n = GetEnvironmentVariableW(key, buffer, static_cast<DWORD>(std::size(buffer)));
    if (!n || n >= std::size(buffer)) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, buffer, static_cast<int>(n), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, buffer, static_cast<int>(n), out.data(), bytes, nullptr, nullptr);
    return out;
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool hexDecode(const std::string& encoded, std::string& out) {
    if (encoded.size() % 2 != 0) return false;
    out.clear();
    out.reserve(encoded.size() / 2);
    for (size_t i = 0; i < encoded.size(); i += 2) {
        const int hi = hexNibble(encoded[i]);
        const int lo = hexNibble(encoded[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return protocol::IsValidUtf8(out);
}

} // namespace

Client::Client() = default;
Client::~Client() { Shutdown(); }

bool Client::IsConnected() const noexcept {
    return pipe_ != reinterpret_cast<void*>(-1) && pipe_ != nullptr;
}

bool Client::Initialize(std::wstring& error, unsigned timeoutMs) {
    Shutdown();
    const auto tempRoot = envUtf8(L"ZERO_TEMP_ROOT");
    if (tempRoot.empty()) {
        error = L"ZERO_TEMP_ROOT is missing. This process was not launched by ZERO Runtime.";
        return false;
    }

    const auto bootstrap = std::filesystem::path(widen(tempRoot)) / L"runtime-v4.bootstrap";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!std::filesystem::exists(bootstrap)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            error = L"Timed out waiting for ZERO Runtime V4 bootstrap.";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    std::ifstream f(bootstrap, std::ios::binary);
    std::string version, pipeName, token;
    std::getline(f, version);
    std::getline(f, sessionId_);
    std::getline(f, packageId_);
    std::getline(f, pipeName);
    std::getline(f, token);
    if (version != "4" || sessionId_.empty() || packageId_.empty() || pipeName.empty() || token.empty()) {
        error = L"ZERO Runtime V4 bootstrap is invalid.";
        return false;
    }

    std::string resumeFlag, activityHex, labelHex, payloadHex;
    std::getline(f, resumeFlag);
    std::getline(f, activityHex);
    std::getline(f, labelHex);
    std::getline(f, payloadHex);
    if (resumeFlag == "1") {
        ResumeContext context;
        if (!hexDecode(activityHex, context.activityId) ||
            !hexDecode(labelHex, context.displayLabel) ||
            !hexDecode(payloadHex, context.payload)) {
            error = L"ZERO launch Resume context is invalid.";
            return false;
        }
        launchResume_ = std::move(context);
    }

    const auto pipeW = widen(pipeName);
    while (true) {
        HANDLE h = CreateFileW(pipeW.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) { pipe_ = h; break; }
        if (GetLastError() != ERROR_PIPE_BUSY || std::chrono::steady_clock::now() >= deadline) {
            error = L"Could not connect to ZERO Runtime V4 IPC.";
            return false;
        }
        WaitNamedPipeW(pipeW.c_str(), 100);
    }

    protocol::Message hello;
    hello.type = protocol::MessageType::Hello;
    hello.requestId = nextRequestId_++;
    hello.fields = {token, packageId_, "4"};
    if (!protocol::WriteMessage(static_cast<HANDLE>(pipe_), hello)) {
        error = L"ZERO SDK IPC authentication write failed.";
        Shutdown();
        return false;
    }
    protocol::Message response;
    if (!protocol::ReadMessage(static_cast<HANDLE>(pipe_), response) ||
        response.type != protocol::MessageType::Welcome ||
        response.requestId != hello.requestId || response.fields != std::vector<std::string>{"4"}) {
        error = L"ZERO Runtime V4 authentication failed.";
        Shutdown();
        return false;
    }
    return true;
}

void Client::Shutdown() {
    if (IsConnected()) CloseHandle(static_cast<HANDLE>(pipe_));
    pipe_ = reinterpret_cast<void*>(-1);
    packageId_.clear();
    sessionId_.clear();
    launchResume_.reset();
    nextRequestId_ = 1;
}

bool Client::SendRequest(protocol::MessageType type,
                         std::vector<std::string> fields,
                         protocol::MessageType expectedType,
                         const std::string& expectedAck,
                         std::wstring& error) {
    if (!IsConnected()) { error = L"ZERO SDK is not connected."; return false; }
    protocol::Message request;
    request.type = type;
    request.requestId = nextRequestId_++;
    request.fields = std::move(fields);
    if (!protocol::WriteMessage(static_cast<HANDLE>(pipe_), request)) {
        error = L"ZERO SDK IPC write failed.";
        return false;
    }
    protocol::Message response;
    if (!protocol::ReadMessage(static_cast<HANDLE>(pipe_), response) || response.requestId != request.requestId) {
        error = L"ZERO SDK IPC response failed.";
        return false;
    }
    if (response.type == protocol::MessageType::Error) {
        error = L"ZERO Runtime rejected the SDK request.";
        return false;
    }
    if (response.type != expectedType) {
        error = L"ZERO SDK received an unexpected response type.";
        return false;
    }
    if (!expectedAck.empty() && (response.fields.size() != 1 || response.fields[0] != expectedAck)) {
        error = L"ZERO SDK received an invalid acknowledgement.";
        return false;
    }
    return true;
}

bool Client::ReportReady(std::wstring& error) {
    return SendRequest(protocol::MessageType::Ready, {}, protocol::MessageType::Ack, "READY", error);
}

bool Client::SetResumeActivity(const ResumeContext& c, std::wstring& error) {
    if (c.activityId.empty() || c.activityId.size() > 256 || c.displayLabel.size() > 512 ||
        c.payload.size() > 60 * 1024 || !protocol::IsValidUtf8(c.activityId) ||
        !protocol::IsValidUtf8(c.displayLabel) || !protocol::IsValidUtf8(c.payload)) {
        error = L"Resume fields are invalid or exceed Protocol V4 limits.";
        return false;
    }
    return SendRequest(protocol::MessageType::Resume,
                       {c.activityId, c.displayLabel, c.payload},
                       protocol::MessageType::Ack, "RESUME", error);
}

bool Client::UnlockAchievement(const std::string& achievementId,
                               const std::string& title,
                               std::wstring& error) {
    if (achievementId.empty() || achievementId.size() > 160 || title.empty() || title.size() > 256 ||
        !protocol::IsValidUtf8(achievementId) || !protocol::IsValidUtf8(title)) {
        error = L"Achievement fields are invalid.";
        return false;
    }
    return SendRequest(protocol::MessageType::Achievement,
                       {achievementId, title},
                       protocol::MessageType::Ack, "ACHIEVEMENT", error);
}

bool Client::Ping(std::wstring& error) {
    return SendRequest(protocol::MessageType::Ping, {}, protocol::MessageType::Pong, "", error);
}

void Client::SetOverlayCallback(std::function<void(bool)> callback) { overlayCallback_ = std::move(callback); }

bool Client::Poll(std::wstring& error) {
    if (!IsConnected()) return false;
    DWORD available = 0;
    if (!PeekNamedPipe(static_cast<HANDLE>(pipe_), nullptr, 0, nullptr, &available, nullptr)) {
        error = L"ZERO SDK IPC polling failed.";
        return false;
    }
    if (available < 4) return true;

    protocol::Message message;
    if (!protocol::ReadMessage(static_cast<HANDLE>(pipe_), message)) {
        error = L"ZERO SDK IPC frame read failed.";
        return false;
    }
    if (message.type == protocol::MessageType::Overlay && message.fields.size() == 1) {
        const bool visible = message.fields[0] == "1";
        if (overlayCallback_) overlayCallback_(visible);
        protocol::Message ack;
        ack.type = protocol::MessageType::OverlayAck;
        ack.requestId = message.requestId;
        ack.fields = {visible ? "1" : "0"};
        if (!protocol::WriteMessage(static_cast<HANDLE>(pipe_), ack)) {
            error = L"ZERO SDK overlay acknowledgement failed.";
            return false;
        }
    }
    return true;
}

} // namespace zero::sdk
