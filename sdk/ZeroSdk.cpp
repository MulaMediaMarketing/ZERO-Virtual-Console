#include "ZeroSdk.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

namespace zero::sdk {
namespace {

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return std::wstring(s.begin(), s.end());
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::string envUtf8(const wchar_t* key) {
    wchar_t buffer[32768]{};
    DWORD n = GetEnvironmentVariableW(key, buffer, static_cast<DWORD>(std::size(buffer)));
    if (!n || n >= std::size(buffer)) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast<int>(n), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast<int>(n), out.data(), bytes, nullptr, nullptr);
    return out;
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
        error = L"ZERO_TEMP_ROOT is missing. This process was not launched by ZERO Runtime V3.";
        return false;
    }

    const auto bootstrap = std::filesystem::path(widen(tempRoot)) / L"runtime-v3.bootstrap";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!std::filesystem::exists(bootstrap)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            error = L"Timed out waiting for ZERO Runtime V3 bootstrap.";
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
    if (version != "3" || sessionId_.empty() || packageId_.empty() || pipeName.empty() || token.empty()) {
        error = L"ZERO Runtime V3 bootstrap is invalid.";
        return false;
    }

    const auto pipeW = widen(pipeName);
    while (true) {
        HANDLE h = CreateFileW(pipeW.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            pipe_ = h;
            break;
        }
        if (GetLastError() != ERROR_PIPE_BUSY || std::chrono::steady_clock::now() >= deadline) {
            error = L"Could not connect to ZERO Runtime V3 IPC.";
            return false;
        }
        WaitNamedPipeW(pipeW.c_str(), 100);
    }

    if (!WriteLine("HELLO\t" + token + "\t" + packageId_ + "\t3", error)) return false;
    std::string response;
    if (!ReadLine(response, error) || response != "WELCOME\t3") {
        error = L"ZERO Runtime V3 authentication failed.";
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
}

bool Client::WriteLine(const std::string& line, std::wstring& error) {
    if (!IsConnected()) { error = L"ZERO SDK is not connected."; return false; }
    const std::string msg = line + "\n";
    DWORD written = 0;
    if (!WriteFile(static_cast<HANDLE>(pipe_), msg.data(), static_cast<DWORD>(msg.size()), &written, nullptr) || written != msg.size()) {
        error = L"ZERO SDK IPC write failed.";
        return false;
    }
    return true;
}

bool Client::ReadLine(std::string& line, std::wstring& error) {
    line.clear();
    if (!IsConnected()) { error = L"ZERO SDK is not connected."; return false; }
    char c = 0;
    DWORD read = 0;
    while (true) {
        if (!ReadFile(static_cast<HANDLE>(pipe_), &c, 1, &read, nullptr) || read != 1) {
            error = L"ZERO SDK IPC read failed.";
            return false;
        }
        if (c == '\n') break;
        if (c != '\r') line.push_back(c);
        if (line.size() > 8192) { error = L"ZERO SDK IPC message exceeded limit."; return false; }
    }
    return true;
}

bool Client::ReportReady(std::wstring& error) {
    if (!WriteLine("READY", error)) return false;
    std::string response;
    return ReadLine(response, error) && response == "ACK\tREADY";
}

bool Client::SetResumeActivity(const ResumeContext& c, std::wstring& error) {
    if (c.activityId.find('\t') != std::string::npos || c.displayLabel.find('\t') != std::string::npos || c.payload.find('\t') != std::string::npos) {
        error = L"Resume fields may not contain tabs in Runtime V3 protocol.";
        return false;
    }
    if (!WriteLine("RESUME\t" + c.activityId + "\t" + c.displayLabel + "\t" + c.payload, error)) return false;
    std::string response;
    return ReadLine(response, error) && response == "ACK\tRESUME";
}

bool Client::Ping(std::wstring& error) {
    if (!WriteLine("PING", error)) return false;
    std::string response;
    return ReadLine(response, error) && response == "PONG";
}

void Client::SetOverlayCallback(std::function<void(bool)> callback) { overlayCallback_ = std::move(callback); }

bool Client::Poll(std::wstring& error) {
    if (!IsConnected()) return false;
    DWORD available = 0;
    if (!PeekNamedPipe(static_cast<HANDLE>(pipe_), nullptr, 0, nullptr, &available, nullptr)) {
        error = L"ZERO SDK IPC polling failed.";
        return false;
    }
    if (!available) return true;
    std::string line;
    if (!ReadLine(line, error)) return false;
    if (line == "OVERLAY\t1" || line == "OVERLAY\t0") {
        const bool visible = line.back() == '1';
        if (overlayCallback_) overlayCallback_(visible);
        return WriteLine(visible ? "OVERLAY_ACK\t1" : "OVERLAY_ACK\t0", error);
    }
    return true;
}

} // namespace zero::sdk
