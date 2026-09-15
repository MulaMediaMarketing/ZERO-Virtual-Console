#include "RuntimeIpcServer.h"
#include "ZeroProtocol.h"
#include "v5/NativeRuntimeProcessHost.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace zero::v5;

namespace {

HANDLE connectPipe(const std::wstring& pipeName) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        HANDLE pipe = CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
                                  0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) return pipe;
        if (GetLastError() != ERROR_PIPE_BUSY && GetLastError() != ERROR_FILE_NOT_FOUND) break;
        WaitNamedPipeW(pipeName.c_str(), 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return INVALID_HANDLE_VALUE;
}

bool authenticate(HANDLE pipe,
                  const std::string& token,
                  const std::string& packageId) {
    zero::protocol::Message hello;
    hello.type = zero::protocol::MessageType::Hello;
    hello.requestId = 1;
    hello.fields = {token, packageId, "4"};
    if (!zero::protocol::WriteMessage(pipe, hello)) return false;
    zero::protocol::Message response;
    return zero::protocol::ReadMessage(pipe, response) &&
           response.type == zero::protocol::MessageType::Welcome &&
           response.requestId == hello.requestId;
}

bool commandDenied(HANDLE pipe, zero::protocol::MessageType type,
                   std::vector<std::string> fields) {
    zero::protocol::Message request;
    request.type = type;
    request.requestId = 2;
    request.fields = std::move(fields);
    if (!zero::protocol::WriteMessage(pipe, request)) return false;
    zero::protocol::Message response;
    return zero::protocol::ReadMessage(pipe, response) &&
           response.type == zero::protocol::MessageType::Error &&
           response.requestId == request.requestId &&
           response.fields == std::vector<std::string>{"CAPABILITY_DENIED"};
}

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                          value.data(), static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                             value.data(), static_cast<int>(value.size()),
                             out.data(), bytes, nullptr, nullptr)) return {};
    return out;
}

std::filesystem::path currentDirectory() {
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return {};
    return std::filesystem::path(path).parent_path();
}

std::string hexEncode(const std::string& value) {
    static constexpr char lut[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        out.push_back(lut[(byte >> 4) & 0x0f]);
        out.push_back(lut[byte & 0x0f]);
    }
    return out;
}

std::vector<std::string> readLines(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) lines.push_back(line);
    return lines;
}

} // namespace

int main() {
    {
        zero::RuntimeIpcServer ipc;
        zero::RuntimeIpcCallbacks callbacks;
        callbacks.onResume = [](const std::string&, const std::string&, const std::string&) { return true; };
        callbacks.onAchievement = [](const std::string&, const std::string&) { return true; };
        zero::RuntimeIpcPermissions permissions;
        permissions.resumeWrite = false;
        permissions.achievements = false;
        permissions.overlay = false;

        const std::string token(64, 'a');
        std::wstring error;
        if (!ipc.StartSecure("v5-ipc-test", "pkg.v5.test", token, callbacks, permissions, error)) return 10;
        HANDLE pipe = connectPipe(ipc.PipeName());
        if (pipe == INVALID_HANDLE_VALUE) { ipc.Stop(); return 11; }
        if (!authenticate(pipe, token, "pkg.v5.test")) { CloseHandle(pipe); ipc.Stop(); return 12; }
        if (!commandDenied(pipe, zero::protocol::MessageType::Achievement, {"id", "title"})) {
            CloseHandle(pipe); ipc.Stop(); return 13;
        }
        if (!commandDenied(pipe, zero::protocol::MessageType::Resume, {"activity", "label", "payload"})) {
            CloseHandle(pipe); ipc.Stop(); return 14;
        }
        CloseHandle(pipe);
        ipc.Stop();
    }

    const auto dir = currentDirectory();
    if (dir.empty()) return 20;
    const auto child = dir / L"ZeroV5RuntimeChild.exe";
    if (!std::filesystem::exists(child)) return 21;

    LaunchDescriptor launch;
    launch.packageId = "pkg.v5.native";
    launch.version = "1.0.0";
    launch.runtimeType = RuntimeType::NativeWin32;
    launch.contentRoot = utf8(dir.wstring());
    launch.executable = utf8(child.wstring());
    launch.sessionToken = "authoritative-session-token";
    launch.entitlementToken = "authoritative-entitlement-token";

    RuntimeSessionGrant grant;
    grant.identity = {"v5-native-session", launch.packageId, "acct-1"};
    grant.grantedCapabilities = {RuntimeCapability::Input, RuntimeCapability::SaveRead};
    grant.ipcAuthenticationToken = std::string(64, 'b');

    zero::ResumeMetadata resume;
    resume.packageId = launch.packageId;
    resume.activityId = "chapter:2";
    resume.displayLabel = "Chapter Two";
    resume.payload = "{\"checkpoint\":7}";

    NativeRuntimeProcessHost host;
    host.SetLaunchResume(resume);
    std::string error;
    if (!host.Start(launch, grant, error)) {
        std::cerr << error << '\n';
        return 22;
    }
    if (host.ProcessInfo().sessionId != grant.identity.sessionId) return 23;
    if (host.ProcessInfo().packageId != grant.identity.packageId) return 24;
    if (host.PipeName().empty()) return 25;

    const auto bootstrap = host.ProcessInfo().tempRoot / L"runtime-v4.bootstrap";
    if (!std::filesystem::exists(bootstrap)) return 26;
    const auto lines = readLines(bootstrap);
    if (lines.size() < 10) return 27;
    if (lines[5] != "1") return 28;
    if (lines[6] != hexEncode(resume.activityId)) return 29;
    if (lines[7] != hexEncode(resume.displayLabel)) return 30;
    if (lines[8] != hexEncode(resume.payload)) return 31;

    host.Terminate();
    if (host.Poll().state != RuntimeProcessState::Exited) return 32;

    std::cout << "ZERO V5 native runtime + secure IPC acceptance: PASS\n";
    return 0;
}
