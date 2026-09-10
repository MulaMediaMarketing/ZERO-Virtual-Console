#include "RuntimeSession.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <objbase.h>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <vector>

namespace zero {
namespace {

std::filesystem::path zeroDataRoot() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::temp_directory_path() / "ZERO";
}

std::wstring widenUtf8(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return std::wstring(value.begin(), value.end());
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), out.data(), count);
    return out;
}

std::string narrowUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string out(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        out.data(), count, nullptr, nullptr);
    return out;
}

std::string jsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 16);
    for (unsigned char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[7]{};
                    sprintf_s(buf, "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

std::string isoTime(std::chrono::system_clock::time_point tp) {
    if (tp.time_since_epoch().count() == 0) return {};
    const auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm utc{};
    gmtime_s(&utc, &tt);
    std::ostringstream os;
    os << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}

std::string makeSessionId() {
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid))) {
        const auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return "session-" + std::to_string(ticks);
    }
    wchar_t buffer[40]{};
    StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)));
    std::wstring value(buffer);
    if (!value.empty() && value.front() == L'{') value.erase(value.begin());
    if (!value.empty() && value.back() == L'}') value.pop_back();
    return narrowUtf8(value);
}

bool isSafePackageId(const std::string& id) {
    if (id.empty() || id.size() > 160) return false;
    for (unsigned char c : id) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        if (!ok) return false;
    }
    return id.find("..") == std::string::npos;
}

bool pathInside(const std::filesystem::path& child, const std::filesystem::path& parent) {
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

std::vector<wchar_t> buildEnvironment(const GameManifest& game,
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

    setEntry(L"ZERO_RUNTIME", L"2");
    setEntry(L"ZERO_SESSION_ID", widenUtf8(info.sessionId));
    setEntry(L"ZERO_PACKAGE_ID", widenUtf8(game.packageId));
    setEntry(L"ZERO_CONTENT_ROOT", game.root.wstring());
    setEntry(L"ZERO_SAVE_ROOT", info.saveRoot.wstring());
    setEntry(L"ZERO_CACHE_ROOT", info.cacheRoot.wstring());
    setEntry(L"ZERO_TEMP_ROOT", info.tempRoot.wstring());

    std::vector<wchar_t> block;
    for (const auto& e : entries) {
        block.insert(block.end(), e.begin(), e.end());
        block.push_back(L'\0');
    }
    block.push_back(L'\0');
    return block;
}

BOOL CALLBACK closeWindowsForProcess(HWND hwnd, LPARAM param) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == static_cast<DWORD>(param) && IsWindowVisible(hwnd)) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }
    return TRUE;
}

const char* stateName(RuntimeState state) {
    switch (state) {
        case RuntimeState::Idle: return "idle";
        case RuntimeState::Launching: return "launching";
        case RuntimeState::Running: return "running";
        case RuntimeState::Exited: return "exited";
        case RuntimeState::Crashed: return "crashed";
        case RuntimeState::Failed: return "failed";
    }
    return "unknown";
}

} // namespace

RuntimeSession::RuntimeSession() = default;
RuntimeSession::~RuntimeSession() {
    if (IsActive()) Terminate();
    ResetHandles();
}

bool RuntimeSession::IsActive() const noexcept {
    return state_ == RuntimeState::Launching || state_ == RuntimeState::Running;
}

void RuntimeSession::ResetHandles() {
    if (process_.hThread) {
        CloseHandle(process_.hThread);
        process_.hThread = nullptr;
    }
    if (process_.hProcess) {
        CloseHandle(process_.hProcess);
        process_.hProcess = nullptr;
    }
    process_.dwProcessId = 0;
    process_.dwThreadId = 0;
    if (job_) {
        CloseHandle(job_);
        job_ = nullptr;
    }
}

bool RuntimeSession::PrepareStorage(const GameManifest& game, std::wstring& error) {
    if (!isSafePackageId(game.packageId)) {
        error = L"The game package ID is invalid.";
        return false;
    }

    const auto data = zeroDataRoot();
    const auto packageName = std::filesystem::path(widenUtf8(game.packageId));
    info_.saveRoot = data / "Saves" / packageName;
    info_.cacheRoot = data / "Cache" / packageName;
    info_.tempRoot = data / "Temp" / packageName / widenUtf8(info_.sessionId);

    std::error_code ec;
    std::filesystem::create_directories(info_.saveRoot, ec);
    if (!ec) std::filesystem::create_directories(info_.cacheRoot, ec);
    if (!ec) std::filesystem::create_directories(info_.tempRoot, ec);
    if (ec) {
        error = L"ZERO could not prepare isolated game storage.";
        return false;
    }

    const auto sessionDir = data / "Runtime" / "Sessions";
    std::filesystem::create_directories(sessionDir, ec);
    if (ec) {
        error = L"ZERO could not create the runtime session directory.";
        return false;
    }
    sessionRecordPath_ = sessionDir / (widenUtf8(info_.sessionId) + L".json");
    return true;
}

bool RuntimeSession::CreateContainmentJob(std::wstring& error) {
    job_ = CreateJobObjectW(nullptr, nullptr);
    if (!job_) {
        error = L"ZERO could not create the game containment job. Windows error: " +
                std::to_wstring(GetLastError());
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job_, JobObjectExtendedLimitInformation,
                                 &limits, sizeof(limits))) {
        error = L"ZERO could not configure game process containment. Windows error: " +
                std::to_wstring(GetLastError());
        CloseHandle(job_);
        job_ = nullptr;
        return false;
    }
    return true;
}

void RuntimeSession::WriteSessionRecord(const char* phase) const {
    if (sessionRecordPath_.empty()) return;
    const auto tmp = sessionRecordPath_.wstring() + L".tmp";
    std::ofstream f(std::filesystem::path(tmp), std::ios::binary | std::ios::trunc);
    if (!f) return;

    f << "{\n"
      << "  \"schema\": 2,\n"
      << "  \"phase\": \"" << jsonEscape(phase ? phase : "unknown") << "\",\n"
      << "  \"session_id\": \"" << jsonEscape(info_.sessionId) << "\",\n"
      << "  \"package_id\": \"" << jsonEscape(info_.packageId) << "\",\n"
      << "  \"title\": \"" << jsonEscape(info_.title) << "\",\n"
      << "  \"version\": \"" << jsonEscape(info_.version) << "\",\n"
      << "  \"state\": \"" << stateName(state_) << "\",\n"
      << "  \"pid\": " << info_.processId << ",\n"
      << "  \"exit_code\": " << info_.exitCode << ",\n"
      << "  \"forced_termination\": " << (info_.forcedTermination ? "true" : "false") << ",\n"
      << "  \"started_at\": \"" << isoTime(info_.startedAt) << "\",\n"
      << "  \"ended_at\": \"" << isoTime(info_.endedAt) << "\",\n"
      << "  \"executable\": \"" << jsonEscape(narrowUtf8(info_.executable.wstring())) << "\"\n"
      << "}\n";
    f.close();

    std::error_code ec;
    std::filesystem::rename(std::filesystem::path(tmp), sessionRecordPath_, ec);
    if (ec) {
        std::filesystem::remove(sessionRecordPath_, ec);
        ec.clear();
        std::filesystem::rename(std::filesystem::path(tmp), sessionRecordPath_, ec);
    }
}

bool RuntimeSession::Launch(const GameManifest& game, std::wstring& error) {
    if (IsActive()) {
        error = L"A game session is already active.";
        return false;
    }

    ResetHandles();
    state_ = RuntimeState::Launching;
    exitCode_ = 0;
    packageId_ = game.packageId;
    info_ = {};
    info_.sessionId = makeSessionId();
    info_.packageId = game.packageId;
    info_.title = game.title;
    info_.version = game.version;
    info_.executable = game.executable;
    info_.startedAt = std::chrono::system_clock::now();

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
        error = L"ZERO Runtime V2 currently supports native Windows .exe game payloads only.";
        return false;
    }
    if (!pathInside(game.executable, game.root)) {
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

    auto env = buildEnvironment(game, info_);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    std::wstring command = L"\"" + game.executable.wstring() + L"\"";
    std::wstring working = game.executable.parent_path().wstring();

    const DWORD flags = CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED;
    BOOL ok = CreateProcessW(
        game.executable.c_str(),
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        flags,
        env.data(),
        working.c_str(),
        &si,
        &process_);

    if (!ok) {
        state_ = RuntimeState::Failed;
        error = L"ZERO could not launch the game executable. Windows error: " +
                std::to_wstring(GetLastError());
        WriteSessionRecord("launch_failed");
        ResetHandles();
        return false;
    }

    info_.processId = process_.dwProcessId;
    info_.exitCode = STILL_ACTIVE;

    if (!AssignProcessToJobObject(job_, process_.hProcess)) {
        const DWORD assignmentError = GetLastError();
        TerminateProcess(process_.hProcess, 0xE001);
        WaitForSingleObject(process_.hProcess, 1500);
        state_ = RuntimeState::Failed;
        error = L"ZERO could not contain the game process. Windows error: " +
                std::to_wstring(assignmentError);
        info_.exitCode = 0xE001;
        info_.endedAt = std::chrono::system_clock::now();
        WriteSessionRecord("launch_failed");
        ResetHandles();
        return false;
    }

    if (ResumeThread(process_.hThread) == static_cast<DWORD>(-1)) {
        const DWORD resumeError = GetLastError();
        TerminateJobObject(job_, 0xE002);
        WaitForSingleObject(process_.hProcess, 1500);
        state_ = RuntimeState::Failed;
        error = L"ZERO could not start the contained game process. Windows error: " +
                std::to_wstring(resumeError);
        info_.exitCode = 0xE002;
        info_.endedAt = std::chrono::system_clock::now();
        WriteSessionRecord("launch_failed");
        ResetHandles();
        return false;
    }

    CloseHandle(process_.hThread);
    process_.hThread = nullptr;
    state_ = RuntimeState::Running;
    WriteSessionRecord("running");
    return true;
}

void RuntimeSession::Finalize(RuntimeState finalState, DWORD code, bool forced) {
    exitCode_ = code;
    state_ = finalState;
    info_.exitCode = code;
    info_.forcedTermination = forced;
    info_.endedAt = std::chrono::system_clock::now();
    WriteSessionRecord(finalState == RuntimeState::Crashed ? "crashed" : "ended");
    ResetHandles();
}

void RuntimeSession::Poll() {
    if (state_ != RuntimeState::Running || !process_.hProcess) return;
    const DWORD wait = WaitForSingleObject(process_.hProcess, 0);
    if (wait == WAIT_TIMEOUT) return;
    if (wait != WAIT_OBJECT_0) {
        Finalize(RuntimeState::Crashed, GetLastError(), false);
        return;
    }

    DWORD code = 0;
    if (!GetExitCodeProcess(process_.hProcess, &code)) {
        code = GetLastError();
        Finalize(RuntimeState::Crashed, code, false);
        return;
    }

    const bool clean = (code == 0);
    Finalize(clean ? RuntimeState::Exited : RuntimeState::Crashed, code, false);
}

void RuntimeSession::Terminate() {
    if (!process_.hProcess || !IsActive()) return;

    const DWORD pid = process_.dwProcessId;
    EnumWindows(closeWindowsForProcess, static_cast<LPARAM>(pid));

    if (WaitForSingleObject(process_.hProcess, 3000) == WAIT_OBJECT_0) {
        DWORD code = 0;
        GetExitCodeProcess(process_.hProcess, &code);
        Finalize(RuntimeState::Exited, code, false);
        return;
    }

    const DWORD forcedCode = 0xDEAD;
    if (job_) {
        TerminateJobObject(job_, forcedCode);
    } else {
        TerminateProcess(process_.hProcess, forcedCode);
    }
    WaitForSingleObject(process_.hProcess, 1500);
    Finalize(RuntimeState::Exited, forcedCode, true);
}

} // namespace zero
