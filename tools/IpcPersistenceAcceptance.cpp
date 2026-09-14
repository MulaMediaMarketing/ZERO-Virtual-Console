#include "RuntimeIpcServer.h"
#include "ZeroSdk.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

std::filesystem::path makeTempRoot(const std::wstring& suffix) {
    wchar_t temp[MAX_PATH]{};
    const DWORD count = GetTempPathW(MAX_PATH, temp);
    if (!count || count >= MAX_PATH) return {};
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(temp) / (L"zero-ipc-persistence-" + suffix + L"-" + std::to_wstring(tick));
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

bool contains(const std::wstring& value, const wchar_t* token) {
    return value.find(token) != std::wstring::npos;
}

[[noreturn]] void finishChild(bool passed, const std::filesystem::path& tempRoot, const char* name) {
    SetEnvironmentVariableW(L"ZERO_TEMP_ROOT", nullptr);
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << "\n";
    std::cout.flush();
    ExitProcess(passed ? 0 : 2);
}

[[noreturn]] void runChild(const std::wstring& mode) {
    constexpr const char* packageId = "zero.system.ipc-acceptance";
    constexpr const char* authToken = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    const std::string modeUtf8 = utf8(mode);
    const std::string sessionId = "ipc-persistence-" + modeUtf8;

    zero::RuntimeIpcServer server;
    zero::RuntimeIpcCallbacks callbacks;
    callbacks.onResume = [mode](const std::string&, const std::string&, const std::string&) {
        return mode != L"resume-fail";
    };
    callbacks.onAchievement = [mode](const std::string&, const std::string&) {
        return mode != L"achievement-fail";
    };

    std::wstring error;
    if (!server.Start(sessionId, packageId, authToken, std::move(callbacks), error)) {
        std::wcerr << L"[FAIL] server_start - " << error << L"\n";
        ExitProcess(2);
    }

    const auto tempRoot = makeTempRoot(mode);
    if (tempRoot.empty() || !writeBootstrap(tempRoot, sessionId, packageId, server.PipeName(), authToken)) {
        std::cerr << "[FAIL] bootstrap_setup\n";
        ExitProcess(2);
    }
    if (!SetEnvironmentVariableW(L"ZERO_TEMP_ROOT", tempRoot.c_str())) {
        std::cerr << "[FAIL] environment_setup\n";
        ExitProcess(2);
    }

    zero::sdk::Client client;
    if (!client.Initialize(error, 5000)) {
        std::wcerr << L"[FAIL] sdk_connects_and_authenticates - " << error << L"\n";
        finishChild(false, tempRoot, "sdk_connects_and_authenticates");
    }

    if (mode == L"resume-fail") {
        zero::sdk::ResumeContext context{"force-resume-failure", "Failure checkpoint", "payload"};
        error.clear();
        const bool passed = !client.SetResumeActivity(context, error) && contains(error, L"RESUME_PERSIST_FAILED");
        finishChild(passed, tempRoot, "resume_persistence_failure_reaches_sdk");
    }

    if (mode == L"achievement-fail") {
        error.clear();
        const bool passed = !client.UnlockAchievement("force-achievement-failure", "Failure achievement", error) &&
                            contains(error, L"ACHIEVEMENT_PERSIST_FAILED");
        finishChild(passed, tempRoot, "achievement_persistence_failure_reaches_sdk");
    }

    if (mode == L"resume-success") {
        zero::sdk::ResumeContext context{"checkpoint-ok", "Checkpoint OK", "payload"};
        error.clear();
        finishChild(client.SetResumeActivity(context, error), tempRoot, "resume_success_still_acks");
    }

    if (mode == L"achievement-success") {
        error.clear();
        finishChild(client.UnlockAchievement("achievement-ok", "Achievement OK", error),
                    tempRoot, "achievement_success_still_acks");
    }

    finishChild(false, tempRoot, "unknown_test_mode");
}

bool runCase(const std::filesystem::path& exe, const std::wstring& mode) {
    std::wstring command = L"\"" + exe.wstring() + L"\" --case " + mode;
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, buffer.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) {
        std::wcerr << L"[FAIL] could not start child case " << mode << L"\n";
        return false;
    }

    const DWORD wait = WaitForSingleObject(process.hProcess, 30000);
    bool passed = false;
    if (wait == WAIT_OBJECT_0) {
        DWORD exitCode = 0;
        passed = GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == 0;
    } else {
        TerminateProcess(process.hProcess, 3);
        std::wcerr << L"[FAIL] child case timed out: " << mode << L"\n";
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return passed;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--case") {
        runChild(argv[2]);
    }

    wchar_t module[MAX_PATH]{};
    const DWORD count = GetModuleFileNameW(nullptr, module, MAX_PATH);
    if (!count || count >= MAX_PATH) {
        std::cerr << "[FAIL] module_path\n";
        return 2;
    }

    const std::filesystem::path exe(module);
    const std::vector<std::wstring> modes = {
        L"resume-fail",
        L"achievement-fail",
        L"resume-success",
        L"achievement-success"
    };

    bool allPassed = true;
    for (const auto& mode : modes) {
        const bool passed = runCase(exe, mode);
        std::wcout << (passed ? L"[PASS] " : L"[FAIL] ") << L"child_" << mode << L"\n";
        allPassed &= passed;
    }

    std::cout << "Result: " << (allPassed ? "PASS" : "FAIL") << "\n";
    return allPassed ? 0 : 2;
}
