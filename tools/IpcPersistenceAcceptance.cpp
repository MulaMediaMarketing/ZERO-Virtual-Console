#include "RuntimeIpcServer.h"
#include "ZeroProtocol.h"
#include <windows.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

using zero::protocol::Message;
using zero::protocol::MessageType;

struct PipeHandle {
    HANDLE value{INVALID_HANDLE_VALUE};
    ~PipeHandle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};

bool connectPipe(const std::wstring& pipeName, PipeHandle& pipe, std::wstring& error) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        pipe.value = CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe.value != INVALID_HANDLE_VALUE) return true;
        const DWORD last = GetLastError();
        if (last != ERROR_PIPE_BUSY && last != ERROR_FILE_NOT_FOUND) {
            error = L"Could not connect to the Runtime IPC acceptance pipe.";
            return false;
        }
        WaitNamedPipeW(pipeName.c_str(), 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    error = L"Timed out connecting to the Runtime IPC acceptance pipe.";
    return false;
}

bool exchange(HANDLE pipe, const Message& request, Message& response) {
    return zero::protocol::WriteMessage(pipe, request) && zero::protocol::ReadMessage(pipe, response);
}

bool expect(const Message& response, MessageType type, uint64_t requestId, const std::string& field) {
    return response.type == type && response.requestId == requestId &&
           response.fields.size() == 1 && response.fields[0] == field;
}

void printCheck(bool passed, const char* name) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << "\n";
}

} // namespace

int wmain() {
    constexpr const char* sessionId = "ipc-persistence-acceptance";
    constexpr const char* packageId = "zero.system.ipc-acceptance";
    constexpr const char* authToken = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    zero::RuntimeIpcServer server;
    zero::RuntimeIpcCallbacks callbacks;
    callbacks.onResume = [](const std::string& activityId, const std::string&, const std::string&) {
        return activityId != "force-resume-failure";
    };
    callbacks.onAchievement = [](const std::string& achievementId, const std::string&) {
        return achievementId != "force-achievement-failure";
    };

    std::wstring error;
    if (!server.Start(sessionId, packageId, authToken, std::move(callbacks), error)) {
        std::wcerr << L"[FAIL] server_start - " << error << L"\n";
        return 2;
    }

    PipeHandle pipe;
    if (!connectPipe(server.PipeName(), pipe, error)) {
        std::wcerr << L"[FAIL] client_connect - " << error << L"\n";
        server.Stop();
        return 2;
    }

    bool allPassed = true;
    Message response;

    const Message hello{MessageType::Hello, 1, {authToken, packageId, "4"}};
    const bool authPassed = exchange(pipe.value, hello, response) && expect(response, MessageType::Welcome, 1, "4");
    printCheck(authPassed, "authenticated_protocol_v4");
    allPassed &= authPassed;

    const Message resumeFailure{MessageType::Resume, 2, {"force-resume-failure", "Failure checkpoint", "payload"}};
    const bool resumeFailurePassed = exchange(pipe.value, resumeFailure, response) &&
                                     expect(response, MessageType::Error, 2, "RESUME_PERSIST_FAILED");
    printCheck(resumeFailurePassed, "resume_failure_returns_error_not_ack");
    allPassed &= resumeFailurePassed;

    const Message achievementFailure{MessageType::Achievement, 3, {"force-achievement-failure", "Failure achievement"}};
    const bool achievementFailurePassed = exchange(pipe.value, achievementFailure, response) &&
                                          expect(response, MessageType::Error, 3, "ACHIEVEMENT_PERSIST_FAILED");
    printCheck(achievementFailurePassed, "achievement_failure_returns_error_not_ack");
    allPassed &= achievementFailurePassed;

    const Message resumeSuccess{MessageType::Resume, 4, {"checkpoint-ok", "Checkpoint OK", "payload"}};
    const bool resumeSuccessPassed = exchange(pipe.value, resumeSuccess, response) &&
                                     expect(response, MessageType::Ack, 4, "RESUME");
    printCheck(resumeSuccessPassed, "resume_success_returns_ack");
    allPassed &= resumeSuccessPassed;

    const Message achievementSuccess{MessageType::Achievement, 5, {"achievement-ok", "Achievement OK"}};
    const bool achievementSuccessPassed = exchange(pipe.value, achievementSuccess, response) &&
                                          expect(response, MessageType::Ack, 5, "ACHIEVEMENT");
    printCheck(achievementSuccessPassed, "achievement_success_returns_ack");
    allPassed &= achievementSuccessPassed;

    server.Stop();
    std::cout << "Result: " << (allPassed ? "PASS" : "FAIL") << "\n";
    return allPassed ? 0 : 2;
}
