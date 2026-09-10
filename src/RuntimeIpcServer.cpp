#include "RuntimeIpcServer.h"
#include <sstream>
#include <vector>

namespace zero {
namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return std::wstring(s.begin(), s.end());
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::vector<std::string> splitTabs(const std::string& line) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        const auto pos = line.find('\t', start);
        if (pos == std::string::npos) {
            out.emplace_back(line.substr(start));
            break;
        }
        out.emplace_back(line.substr(start, pos - start));
        start = pos + 1;
    }
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
        error = L"Runtime V3 IPC requires session, package, and authentication identities.";
        return false;
    }

    pipeName_ = L"\\\\.\\pipe\\zero-runtime-" + widen(sessionId);
    packageId_ = packageId;
    authToken_ = authToken;
    callbacks_ = std::move(callbacks);
    stop_.store(false);
    authenticated_.store(false);
    ready_.store(false);
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

bool RuntimeIpcServer::WriteLine(HANDLE pipe, const std::string& line) {
    const std::string msg = line + "\n";
    DWORD written = 0;
    return WriteFile(pipe, msg.data(), static_cast<DWORD>(msg.size()), &written, nullptr) && written == msg.size();
}

bool RuntimeIpcServer::SendOverlayFocus(bool focused) {
    std::scoped_lock lock(pipeMutex_);
    if (activePipe_ == INVALID_HANDLE_VALUE || !authenticated_.load()) return false;
    return WriteLine(activePipe_, focused ? "OVERLAY\t1" : "OVERLAY\t0");
}

bool RuntimeIpcServer::HandleLine(const std::string& line, HANDLE pipe) {
    const auto fields = splitTabs(line);
    if (fields.empty()) return true;

    if (!authenticated_.load()) {
        if (fields.size() != 4 || fields[0] != "HELLO" || fields[1] != authToken_ ||
            fields[2] != packageId_ || fields[3] != "3") {
            WriteLine(pipe, "ERROR\tAUTH");
            return false;
        }
        authenticated_.store(true);
        WriteLine(pipe, "WELCOME\t3");
        return true;
    }

    if (fields[0] == "READY") {
        ready_.store(true);
        if (callbacks_.onReady) callbacks_.onReady();
        WriteLine(pipe, "ACK\tREADY");
        return true;
    }

    if (fields[0] == "RESUME" && fields.size() >= 4) {
        if (callbacks_.onResume) callbacks_.onResume(fields[1], fields[2], fields[3]);
        WriteLine(pipe, "ACK\tRESUME");
        return true;
    }

    if (fields[0] == "OVERLAY_ACK" && fields.size() >= 2) {
        if (callbacks_.onOverlayFocus) callbacks_.onOverlayFocus(fields[1] == "1");
        return true;
    }

    if (fields[0] == "PING") {
        WriteLine(pipe, "PONG");
        return true;
    }

    WriteLine(pipe, "ERROR\tUNKNOWN_COMMAND");
    return true;
}

void RuntimeIpcServer::ServerLoop() {
    HANDLE pipe = CreateNamedPipeW(
        pipeName_.c_str(),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return;

    {
        std::scoped_lock lock(pipeMutex_);
        activePipe_ = pipe;
    }

    const BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (connected && !stop_.load()) {
        std::string pending;
        char buffer[1024];
        while (!stop_.load()) {
            DWORD read = 0;
            if (!ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) || read == 0) break;
            pending.append(buffer, buffer + read);
            size_t newline = 0;
            while ((newline = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, newline);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                pending.erase(0, newline + 1);
                if (!HandleLine(line, pipe)) {
                    stop_.store(true);
                    break;
                }
            }
        }
    }

    std::scoped_lock lock(pipeMutex_);
    if (activePipe_ == pipe) activePipe_ = INVALID_HANDLE_VALUE;
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
}

} // namespace zero
