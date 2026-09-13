#include "RuntimeIpcServer.h"

namespace zero {
namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

} // namespace

RuntimeIpcServer::RuntimeIpcServer() = default;
RuntimeIpcServer::~RuntimeIpcServer() { Stop(); }

bool RuntimeIpcServer::Start(const std::string& sessionId,
                             const std::string& packageId,
                             const std::string& authToken,
                             RuntimeIpcCallbacks callbacks,
                             std::wstring& error) {
    Stop();
    if (sessionId.empty() || packageId.empty() || authToken.empty()) {
        error = L"Runtime IPC requires session, package, and authentication identities.";
        return false;
    }
    const auto sessionW = widen(sessionId);
    if (sessionW.empty()) {
        error = L"Runtime IPC session identity is not valid UTF-8.";
        return false;
    }

    pipeName_ = L"\\\\.\\pipe\\zero-runtime-" + sessionW;
    packageId_ = packageId;
    authToken_ = authToken;
    callbacks_ = std::move(callbacks);
    stop_.store(false);
    authenticated_.store(false);
    ready_.store(false);
    nextServerRequestId_.store(1);
    thread_ = std::thread(&RuntimeIpcServer::ServerLoop, this);
    return true;
}

void RuntimeIpcServer::Stop() {
    stop_.store(true);
    {
        std::scoped_lock lock(pipeMutex_);
        if (activePipe_ != INVALID_HANDLE_VALUE) {
            CancelIoEx(activePipe_, nullptr);
            DisconnectNamedPipe(activePipe_);
        }
    }
    if (thread_.joinable()) thread_.join();
    std::scoped_lock lock(pipeMutex_);
    if (activePipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(activePipe_);
        activePipe_ = INVALID_HANDLE_VALUE;
    }
}

bool RuntimeIpcServer::Send(HANDLE pipe, protocol::MessageType type, uint64_t requestId,
                            std::vector<std::string> fields) {
    protocol::Message message;
    message.type = type;
    message.requestId = requestId;
    message.fields = std::move(fields);
    return protocol::WriteMessage(pipe, message);
}

bool RuntimeIpcServer::SendOverlayFocus(bool focused) {
    std::scoped_lock lock(pipeMutex_);
    if (activePipe_ == INVALID_HANDLE_VALUE || !authenticated_.load()) return false;
    const uint64_t requestId = nextServerRequestId_.fetch_add(1);
    return Send(activePipe_, protocol::MessageType::Overlay, requestId, {focused ? "1" : "0"});
}

bool RuntimeIpcServer::HandleMessage(const protocol::Message& message, HANDLE pipe) {
    using protocol::MessageType;

    if (!authenticated_.load()) {
        if (message.type != MessageType::Hello || message.fields.size() != 3 ||
            message.fields[0] != authToken_ || message.fields[1] != packageId_ ||
            message.fields[2] != "4") {
            Send(pipe, MessageType::Error, message.requestId, {"AUTH"});
            return false;
        }
        authenticated_.store(true);
        return Send(pipe, MessageType::Welcome, message.requestId, {"4"});
    }

    switch (message.type) {
        case MessageType::Ready:
            if (!message.fields.empty()) return Send(pipe, MessageType::Error, message.requestId, {"BAD_READY"});
            ready_.store(true);
            if (callbacks_.onReady) callbacks_.onReady();
            return Send(pipe, MessageType::Ack, message.requestId, {"READY"});

        case MessageType::Resume:
            if (message.fields.size() != 3) return Send(pipe, MessageType::Error, message.requestId, {"BAD_RESUME"});
            if (callbacks_.onResume) callbacks_.onResume(message.fields[0], message.fields[1], message.fields[2]);
            return Send(pipe, MessageType::Ack, message.requestId, {"RESUME"});

        case MessageType::Achievement:
            if (message.fields.size() != 2) return Send(pipe, MessageType::Error, message.requestId, {"BAD_ACHIEVEMENT"});
            if (callbacks_.onAchievement) callbacks_.onAchievement(message.fields[0], message.fields[1]);
            return Send(pipe, MessageType::Ack, message.requestId, {"ACHIEVEMENT"});

        case MessageType::OverlayAck:
            if (message.fields.size() != 1) return Send(pipe, MessageType::Error, message.requestId, {"BAD_OVERLAY_ACK"});
            if (callbacks_.onOverlayFocus) callbacks_.onOverlayFocus(message.fields[0] == "1");
            return true;

        case MessageType::Ping:
            if (!message.fields.empty()) return Send(pipe, MessageType::Error, message.requestId, {"BAD_PING"});
            return Send(pipe, MessageType::Pong, message.requestId);

        default:
            return Send(pipe, MessageType::Error, message.requestId, {"UNKNOWN_COMMAND"});
    }
}

void RuntimeIpcServer::ServerLoop() {
    HANDLE pipe = CreateNamedPipeW(
        pipeName_.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        protocol::kMaxFrameBytes + 4,
        protocol::kMaxFrameBytes + 4,
        0,
        nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return;

    {
        std::scoped_lock lock(pipeMutex_);
        activePipe_ = pipe;
    }

    const BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (connected && !stop_.load()) {
        while (!stop_.load()) {
            protocol::Message message;
            if (!protocol::ReadMessage(pipe, message)) break;
            if (!HandleMessage(message, pipe)) break;
        }
    }

    std::scoped_lock lock(pipeMutex_);
    if (activePipe_ == pipe) activePipe_ = INVALID_HANDLE_VALUE;
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
}

} // namespace zero
