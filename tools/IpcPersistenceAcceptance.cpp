#include "RuntimeIpcServer.h"
#include "ZeroSdk.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                             out.data(), bytes, nullptr, nullptr)) return {};
    return out;
}

std::filesystem::path makeTempRoot() {
    wchar_t temp[MAX_PATH]{};
    const DWORD count = GetTempPathW(MAX_PATH, temp);
    if (!count || count >= MAX_PATH) return {};
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(temp) / (L"zero-ipc-persistence-" + std::to_wstring(tick));
}

bool writeBootstrap(const std::filesystem::path& root,
                    const std::string& sessionId,
                    const std::string& packageId,
                    const std::wstring& pipeName,
                    const std::string& authToken) {
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) return false;

    const auto pipe = utf8(pipeName);
    if (pipe.empty()) return false;

    std::ofstream f(root / "runtime-v4.bootstrap", std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << "4\n" << sessionId << "\n" << packageId << "\n" << pipe << "\n" << authToken
      << "\n0\n\n\n\n";
    f.flush();
    return f.good();
}

void printCheck(bool passed, const char* name) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << "\n";
}

bool contains(const std::wstring& value, const wchar_t* token) {
    return value.find(token) != std::wstring::npos;
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

    const auto tempRoot = makeTempRoot();
    if (tempRoot.empty() || !writeBootstrap(tempRoot, sessionId, packageId, server.PipeName(), authToken)) {
        std::cerr << "[FAIL] bootstrap_setup\n";
        server.Stop();
        return 2;
    }

    if (!SetEnvironmentVariableW(L"ZERO_TEMP_ROOT", tempRoot.c_str())) {
        std::cerr << "[FAIL] environment_setup\n";
        server.Stop();
        std::error_code ec;
        std::filesystem::remove_all(tempRoot, ec);
        return 2;
    }

    zero::sdk::Client client;
    bool allPassed = true;

    const bool initPassed = client.Initialize(error, 5000);
    printCheck(initPassed, "sdk_connects_and_authenticates");
    allPassed &= initPassed;

    if (initPassed) {
        zero::sdk::ResumeContext failedResume{"force-resume-failure", "Failure checkpoint", "payload"};
        error.clear();
        const bool resumeFailed = !client.SetResumeActivity(failedResume, error) &&
                                  contains(error, L"RESUME_PERSIST_FAILED");
        printCheck(resumeFailed, "resume_persistence_failure_reaches_sdk");
        allPassed &= resumeFailed;

        error.clear();
        const bool achievementFailed = !client.UnlockAchievement("force-achievement-failure", "Failure achievement", error) &&
                                       contains(error, L"ACHIEVEMENT_PERSIST_FAILED");
        printCheck(achievementFailed, "achievement_persistence_failure_reaches_sdk");
        allPassed &= achievementFailed;

        zero::sdk::ResumeContext goodResume{"checkpoint-ok", "Checkpoint OK", "payload"};
        error.clear();
        const bool resumeSucceeded = client.SetResumeActivity(goodResume, error);
        printCheck(resumeSucceeded, "resume_success_still_acks");
        allPassed &= resumeSucceeded;

        error.clear();
        const bool achievementSucceeded = client.UnlockAchievement("achievement-ok", "Achievement OK", error);
        printCheck(achievementSucceeded, "achievement_success_still_acks");
        allPassed &= achievementSucceeded;
    }

    // Stop the server first so its pipe disconnect unblocks the SDK receiver's synchronous read.
    server.Stop();
    client.Shutdown();
    SetEnvironmentVariableW(L"ZERO_TEMP_ROOT", nullptr);
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);

    std::cout << "Result: " << (allPassed ? "PASS" : "FAIL") << "\n";
    return allPassed ? 0 : 2;
}
