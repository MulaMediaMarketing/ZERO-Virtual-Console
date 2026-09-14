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

bool Client::OpenAndAuthenticate(unsigned timeoutMs, std::wstring& error) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    HANDLE h = INVALID_HANDLE_VALUE;
    while (!stop_.load()) {
        h = CreateFileW(pipeName_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) break;
        const DWORD last = GetLastError();
        if ((last != ERROR_PIPE_BUSY && last != ERROR_FILE_NOT_FOUND) || std::chrono::steady_clock::now() >= deadline) {
            error = L"Could not connect to ZERO Runtime V4 IPC.";
            return false;
        }
        WaitNamedPipeW(pipeName_.c_str(), 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if (h == INVALID_HANDLE_VALUE) {
        error = L"ZERO SDK connection was cancelled.";
        return false;
    }

    protocol::Message hello;
    hello.type = protocol::MessageType::Hello;
    hello.requestId = nextRequestId_.fetch_add(1);
    hello.fields = {authToken_, packageId_, "4"};
    if (!protocol::WriteMessage(h, hello)) {
        CloseHandle(h);
        error = L"ZERO SDK IPC authentication write failed.";
        return false;
    }

    protocol::Message response;
    if (!protocol::ReadMessage(h, response) ||
        response.type != protocol::MessageType::Welcome ||
        response.requestId != hello.requestId || response.fields != std::vector<std::string>{"4"}) {
        CloseHandle(h);
        error = L"ZERO Runtime V4 authentication failed.";
        return false;
    }

    {
        std::scoped_lock writeLock(writeMutex_);
        std::scoped_lock connectionLock(connectionMutex_);
        if (pipe_ != reinterpret_cast<void*>(-1) && pipe_ != nullptr) {
            CloseHandle(static_cast<HANDLE>(pipe_));
        }
        pipe_ = h;
        connected_.store(true);
    }
    return true;
}

bool Client::Initialize(std::wstring& error, unsigned timeoutMs) {
    Shutdown();
    stop_.store(false);

    const auto tempRoot = envUtf8(L"ZERO_TEMP_ROOT");
    if (tempRoot.empty()) {
        error = L"ZERO_TEMP_ROOT is missing. This process was not launched by ZERO Runtime.";
        stop_.store(true);
        return false;
    }

    const auto bootstrap = std::filesystem::path(widen(tempRoot)) / L"runtime-v4.bootstrap";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!std::filesystem::exists(bootstrap)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            error = L"Timed out waiting for ZERO Runtime V4 bootstrap.";
            stop_.store(true);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    std::ifstream f(bootstrap, std::ios::binary);
    std::string version, pipeNameUtf8;
    std::getline(f, version);
    std::getline(f, sessionId_);
    std::getline(f, packageId_);
    std::getline(f, pipeNameUtf8);
    std::getline(f, authToken_);
    if (version != "4" || sessionId_.empty() || packageId_.empty() || pipeNameUtf8.empty() || authToken_.empty()) {
        error = L"ZERO Runtime V4 bootstrap is invalid.";
        stop_.store(true);
        return false;
    }
    pipeName_ = widen(pipeNameUtf8);
    if (pipeName_.empty()) {
        error = L"ZERO Runtime V4 pipe identity is invalid UTF-8.";
        stop_.store(true);
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
            stop_.store(true);
            return false;
        }
        launchResume_ = std::move(context);
    }

    if (!OpenAndAuthenticate(timeoutMs, error)) {
        stop_.store(true);
        return false;
    }

    receiverThread_ = std::thread(&Client::ReceiverLoop, this);
    heartbeatThread_ = std::thread(&Client::HeartbeatLoop, this);
    return true;
}

void Client::FailAllPending() {
    std::vector<std::shared_ptr<PendingResponse>> waiters;
    {
        std::scoped_lock lock(pendingMutex_);
        for (auto& [_, pending] : pending_) waiters.push_back(pending);
        pending_.clear();
    }
    for (auto& pending : waiters) {
        {
            std::scoped_lock lock(pending->mutex);
            pending->failed = true;
            pending->completed = true;
        }
        pending->cv.notify_all();
    }
}

void Client::CloseTransport() {
    std::scoped_lock writeLock(writeMutex_);
    std::scoped_lock connectionLock(connectionMutex_);
    connected_.store(false);
    if (pipe_ != reinterpret_cast<void*>(-1) && pipe_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(pipe_));
    }
    pipe_ = reinterpret_cast<void*>(-1);
}

void Client::Shutdown() {
    stop_.store(true);
    FailAllPending();

    // The receiver performs synchronous named-pipe reads. Explicitly cancel that
    // thread's blocking I/O before closing the shared transport and joining it.
    if (receiverThread_.joinable()) {
        CancelSynchronousIo(static_cast<HANDLE>(receiverThread_.native_handle()));
    }

    CloseTransport();
    if (heartbeatThread_.joinable()) heartbeatThread_.join();
    if (receiverThread_.joinable()) receiverThread_.join();

    pipeName_.clear();
    authToken_.clear();
    packageId_.clear();
    sessionId_.clear();
    launchResume_.reset();
    nextRequestId_.store(1);
}

bool Client::TryReconnect() {
    if (stop_.load()) return false;
    static constexpr unsigned delaysMs[] = {250, 500, 1000, 2000, 2000};
    for (unsigned delay : delaysMs) {
        if (stop_.load()) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        std::wstring ignored;
        if (OpenAndAuthenticate(1500, ignored)) return true;
    }
    return false;
}

bool Client::WriteFrame(const protocol::Message& message) {
    std::scoped_lock writeLock(writeMutex_);
    std::scoped_lock connectionLock(connectionMutex_);
    if (!connected_.load() || pipe_ == reinterpret_cast<void*>(-1) || pipe_ == nullptr) return false;
    return protocol::WriteMessage(static_cast<HANDLE>(pipe_), message);
}

void Client::DispatchMessage(const protocol::Message& message) {
    if (message.type == protocol::MessageType::Overlay && message.fields.size() == 1) {
        const bool visible = message.fields[0] == "1";
        std::function<void(bool)> callback;
        {
            std::scoped_lock lock(callbackMutex_);
            callback = overlayCallback_;
        }
        if (callback) callback(visible);

        protocol::Message ack;
        ack.type = protocol::MessageType::OverlayAck;
        ack.requestId = message.requestId;
        ack.fields = {visible ? "1" : "0"};
        WriteFrame(ack);
        return;
    }

    std::shared_ptr<PendingResponse> pending;
    {
        std::scoped_lock lock(pendingMutex_);
        const auto it = pending_.find(message.requestId);
        if (it != pending_.end()) pending = it->second;
    }
    if (!pending) return;

    {
        std::scoped_lock lock(pending->mutex);
        pending->message = message;
        pending->completed = true;
    }
    pending->cv.notify_all();
}

void Client::ReceiverLoop() {
    while (!stop_.load()) {
        HANDLE h = INVALID_HANDLE_VALUE;
        {
            std::scoped_lock lock(connectionMutex_);
            if (connected_.load() && pipe_ != reinterpret_cast<void*>(-1) && pipe_ != nullptr) {
                h = static_cast<HANDLE>(pipe_);
            }
        }

        if (h == INVALID_HANDLE_VALUE) {
            if (!TryReconnect()) break;
            continue;
        }

        protocol::Message message;
        if (!protocol::ReadMessage(h, message)) {
            FailAllPending();
            CloseTransport();
            if (!stop_.load() && TryReconnect()) continue;
            break;
        }
        DispatchMessage(message);
    }
    connected_.store(false);
}

void Client::HeartbeatLoop() {
    while (!stop_.load()) {
        for (int i = 0; i < 20 && !stop_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (stop_.load()) break;
        if (!connected_.load()) continue;

        protocol::Message heartbeat;
        heartbeat.type = protocol::MessageType::Heartbeat;
        heartbeat.requestId = nextRequestId_.fetch_add(1);
        if (!WriteFrame(heartbeat)) {
            connected_.store(false);
        }
    }
}

bool Client::SendRequest(protocol::MessageType type,
                         std::vector<std::string> fields,
                         protocol::MessageType expectedType,
                         const std::string& expectedAck,
                         std::wstring& error) {
    if (!connected_.load()) {
        error = L"ZERO SDK is not connected.";
        return false;
    }

    protocol::Message request;
    request.type = type;
    request.requestId = nextRequestId_.fetch_add(1);
    request.fields = std::move(fields);

    auto pending = std::make_shared<PendingResponse>();
    {
        std::scoped_lock lock(pendingMutex_);
        pending_[request.requestId] = pending;
    }

    if (!WriteFrame(request)) {
        {
            std::scoped_lock lock(pendingMutex_);
            pending_.erase(request.requestId);
        }
        error = L"ZERO SDK IPC write failed.";
        return false;
    }

    std::unique_lock lock(pending->mutex);
    const bool signaled = pending->cv.wait_for(lock, std::chrono::seconds(5), [&] {
        return pending->completed;
    });
    lock.unlock();

    {
        std::scoped_lock pendingLock(pendingMutex_);
        pending_.erase(request.requestId);
    }

    if (!signaled || pending->failed) {
        error = L"ZERO SDK IPC request timed out or the transport disconnected.";
        return false;
    }

    const auto& response = pending->message;
    if (response.type == protocol::MessageType::Error) {
        error = L"ZERO Runtime rejected the SDK request.";
        if (response.fields.size() == 1 && protocol::IsValidUtf8(response.fields[0])) {
            const auto detail = widen(response.fields[0]);
            if (!detail.empty()) error += L" " + detail;
        }
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

void Client::SetOverlayCallback(std::function<void(bool)> callback) {
    std::scoped_lock lock(callbackMutex_);
    overlayCallback_ = std::move(callback);
}

bool Client::Poll(std::wstring& error) {
    if (!connected_.load()) {
        error = L"ZERO SDK transport is reconnecting or disconnected.";
        return false;
    }
    return true;
}

} // namespace zero::sdk
