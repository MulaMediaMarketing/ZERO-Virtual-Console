#pragma once
#include "MiniDumpWriter.h"
#include "ZeroTypes.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <string>

namespace zero {

struct RuntimeSessionInfo {
    std::string sessionId;
    std::string packageId;
    std::string title;
    std::string version;
    std::filesystem::path executable;
    std::filesystem::path saveRoot;
    std::filesystem::path cacheRoot;
    std::filesystem::path tempRoot;
    std::filesystem::path miniDumpPath;
    std::string miniDumpNote;
    std::chrono::system_clock::time_point startedAt{};
    std::chrono::system_clock::time_point endedAt{};
    DWORD processId{0};
    DWORD exitCode{STILL_ACTIVE};
    DWORD miniDumpError{0};
    bool forcedTermination{false};
    bool miniDumpWritten{false};
};

class RuntimeSession {
public:
    RuntimeSession();
    ~RuntimeSession();
    RuntimeSession(const RuntimeSession&) = delete;
    RuntimeSession& operator=(const RuntimeSession&) = delete;

    bool Launch(const GameManifest& game, std::wstring& error);
    bool PrepareLaunch(const GameManifest& game, std::wstring& error);
    bool ResumePrepared(std::wstring& error);
    void FailPrepared(DWORD code, const char* phase = "launch_failed");

    void Poll();
    void Terminate();
    bool CaptureDiagnosticDump();
    bool HasAbnormalExit() const noexcept;

    RuntimeState State() const noexcept { return state_; }
    DWORD ExitCode() const noexcept { return exitCode_; }
    const std::string& ActivePackageId() const noexcept { return packageId_; }
    const RuntimeSessionInfo& Info() const noexcept { return info_; }
    bool IsActive() const noexcept;
    bool IsPrepared() const noexcept { return state_ == RuntimeState::Launching && process_.hThread != nullptr; }

private:
    PROCESS_INFORMATION process_{};
    HANDLE job_{nullptr};
    RuntimeState state_{RuntimeState::Idle};
    DWORD exitCode_{0};
    std::string packageId_;
    RuntimeSessionInfo info_{};
    std::filesystem::path sessionRecordPath_;
    MiniDumpWriter miniDumpWriter_;

    void ResetHandles();
    void Finalize(RuntimeState finalState, DWORD code, bool forced);
    bool PrepareStorage(const GameManifest& game, std::wstring& error);
    bool CreateContainmentJob(std::wstring& error);
    void WriteSessionRecord(const char* phase) const;
};
}
