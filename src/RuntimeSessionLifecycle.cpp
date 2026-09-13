#include "RuntimeSession.h"
#include <objbase.h>
#include <string>
#include <vector>

namespace zero {
namespace {

std::wstring widenUtf8Lifecycle(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), out.data(), count);
    return out;
}

std::string narrowUtf8Lifecycle(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string out(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                        out.data(), count, nullptr, nullptr);
    return out;
}

std::string makeSessionIdLifecycle() {
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) return {};
    wchar_t buffer[40]{};
    if (!StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)))) return {};
    std::wstring value(buffer);
    if (!value.empty() && value.front() == L'{') value.erase(value.begin());
    if (!value.empty() && value.back() == L'}') value.pop_back();
    return narrowUtf8Lifecycle(value);
}

bool pathInsideLifecycle(const std::filesystem::path& child, const std::filesystem::path& parent) {
    std::error_code ec;
    const auto c = std::filesystem::weakly_canonical(child, ec);
    if (ec) return false;
    const auto p = std::filesystem::weakly_canonical(parent, ec);
    if (ec) return false;
    auto ci = c.begin();
    auto pi = p.begin();
    for (; pi != p.end(); ++pi, ++ci) {
        if (ci == c.end() || _wcsicmp(ci->c_str(), pi->c_str()) != 0) return false;
    }
    return true;
}

std::vector<wchar_t> buildEnvironmentLifecycle(const GameManifest& game,
                                               const RuntimeSessionInfo& info) {
    std::vector<std::wstring> entries;
    LPWCH env = GetEnvironmentStringsW();
    if (env) {
        for (LPWCH p = env; *p; ) {
            std::wstring item(p);
            entries.push_back(item);
            p += item.size() + 1;
        }
        FreeEnvironmentStringsW(env);
    }

    auto setEntry = [&](const std::wstring& key, const std::wstring& value) {
        const std::wstring prefix = key + L"=";
        for (auto& e : entries) {
            if (_wcsnicmp(e.c_str(), prefix.c_str(), prefix.size()) == 0) {
                e = prefix + value;
                return;
            }
        }
        entries.push_back(prefix + value);
    };

    // Environment blocks passed to CreateProcessW are expected to be sorted
    // case-insensitively when CREATE_UNICODE_ENVIRONMENT is used.
    setEntry(L"ZERO_RUNTIME", L"4");
    setEntry(L"ZERO_SESSION_ID", widenUtf8Lifecycle(info.sessionId));
    setEntry(L"ZERO_PACKAGE_ID", widenUtf8Lifecycle(game.packageId));
    setEntry(L"ZERO_CONTENT_ROOT", game.root.wstring());
    setEntry(L"ZERO_SAVE_ROOT", info.saveRoot.wstring());
    setEntry(L"ZERO_CACHE_ROOT", info.cacheRoot.wstring());
    setEntry(L"ZERO_TEMP_ROOT", info.tempRoot.wstring());

    std::sort(entries.begin(), entries.end(), [](const std::wstring& a, const std::wstring& b) {
        return _wcsicmp(a.c_str(), b.c_str()) < 0;
    });

    std::vector<wchar_t> block;
    for (const auto& e : entries) {
        block.insert(block.end(), e.begin(), e.end());
        block.push_back(L'\0');
    }
    block.push_back(L'\0');
    return block;
}

} // namespace

bool RuntimeSession::PrepareLaunch(const GameManifest& game, std::wstring& error) {
    if (IsActive()) {
        error = L"A game session is already active.";
        return false;
    }

    ResetHandles();
    state_ = RuntimeState::Launching;
    exitCode_ = 0;
    packageId_ = game.packageId;
    info_ = {};
    info_.sessionId = makeSessionIdLifecycle();
    info_.packageId = game.packageId;
    info_.title = game.title;
    info_.version = game.version;
    info_.executable = game.executable;
    info_.startedAt = std::chrono::system_clock::now();

    if (info_.sessionId.empty()) {
        state_ = RuntimeState::Failed;
        error = L"ZERO could not allocate a secure runtime session identity.";
        return false;
    }
    if (game.packageId.empty() || game.title.empty()) {
        state_ = RuntimeState::Failed;
        error = L"The game manifest is missing required identity fields.";
        return false;
    }
    if (!std::filesystem::exists(game.executable) || !std::filesystem::is_regular_file(game.executable)) {
        state_ = RuntimeState::Failed;
        error = L"The registered game executable no longer exists.";
        return false;
    }
    if (_wcsicmp(game.executable.extension().c_str(), L".exe") != 0) {
        state_ = RuntimeState::Failed;
        error = L"ZERO Runtime currently supports native Windows .exe game payloads only.";
        return false;
    }
    if (!pathInsideLifecycle(game.executable, game.root)) {
        state_ = RuntimeState::Failed;
        error = L"The game executable must be located inside its registered game directory.";
        return false;
    }
    if (!PrepareStorage(game, error)) {
        state_ = RuntimeState::Failed;
        return false;
    }
    if (!CreateContainmentJob(error)) {
        state_ = RuntimeState::Failed;
        WriteSessionRecord("launch_failed");
        return false;
    }

    auto env = buildEnvironmentLifecycle(game, info_);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    std::wstring command = L"\"" + game.executable.wstring() + L"\"";
    std::wstring working = game.executable.parent_path().wstring();

    const DWORD flags = CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED;
    const BOOL ok = CreateProcessW(
        game.executable.c_str(), command.data(), nullptr, nullptr, FALSE, flags,
        env.data(), working.c_str(), &si, &process_);
    if (!ok) {
        state_ = RuntimeState::Failed;
        error = L"ZERO could not prepare the game executable. Windows error: " +
                std::to_wstring(GetLastError());
        WriteSessionRecord("launch_failed");
        ResetHandles();
        return false;
    }

    info_.processId = process_.dwProcessId;
    info_.exitCode = STILL_ACTIVE;

    if (!AssignProcessToJobObject(job_, process_.hProcess)) {
        const DWORD assignmentError = GetLastError();
        TerminateProcess(process_.hProcess, 0xE101);
        WaitForSingleObject(process_.hProcess, 1500);
        state_ = RuntimeState::Failed;
        info_.exitCode = 0xE101;
        info_.endedAt = std::chrono::system_clock::now();
        error = L"ZERO could not contain the prepared game process. Windows error: " +
                std::to_wstring(assignmentError);
        WriteSessionRecord("launch_failed");
        ResetHandles();
        return false;
    }

    WriteSessionRecord("prepared");
    return true;
}

bool RuntimeSession::ResumePrepared(std::wstring& error) {
    if (!IsPrepared()) {
        error = L"ZERO runtime does not have a prepared game process to resume.";
        return false;
    }
    if (ResumeThread(process_.hThread) == static_cast<DWORD>(-1)) {
        const DWORD resumeError = GetLastError();
        FailPrepared(0xE102, "resume_failed");
        error = L"ZERO could not start the prepared game process. Windows error: " +
                std::to_wstring(resumeError);
        return false;
    }

    CloseHandle(process_.hThread);
    process_.hThread = nullptr;
    state_ = RuntimeState::Running;
    WriteSessionRecord("running");
    return true;
}

void RuntimeSession::FailPrepared(DWORD code, const char* phase) {
    if (process_.hProcess) {
        if (job_) TerminateJobObject(job_, code);
        else TerminateProcess(process_.hProcess, code);
        WaitForSingleObject(process_.hProcess, 1500);
    }
    exitCode_ = code;
    state_ = RuntimeState::Failed;
    info_.exitCode = code;
    info_.forcedTermination = true;
    info_.endedAt = std::chrono::system_clock::now();
    WriteSessionRecord(phase ? phase : "launch_failed");
    ResetHandles();
}

} // namespace zero
