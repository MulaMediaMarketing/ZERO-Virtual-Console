#pragma once
#include "ResumeStore.h"
#include "RuntimeIpcServer.h"
#include "RuntimeSession.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace zero {

enum class RuntimeOutcome {
    None,
    Starting,
    Running,
    CleanExit,
    Crash,
    UserTermination,
    LaunchFailure,
    HandshakeFailure,
    ReadyTimeout,
    Hung
};

class RuntimeV3 {
public:
    bool Launch(const GameManifest& game, std::wstring& error,
                const std::optional<ResumeMetadata>& launchResume = std::nullopt);
    void Poll();
    void Terminate();

    RuntimeState State() const noexcept { return v2_.State(); }
    DWORD ExitCode() const noexcept { return v2_.ExitCode(); }
    const RuntimeSessionInfo& Info() const noexcept { return v2_.Info(); }
    bool IsActive() const noexcept { return v2_.IsActive(); }
    bool Ready() const noexcept { return ready_.load(); }
    uint64_t PlaytimeSeconds() const noexcept { return playtimeSeconds_; }
    RuntimeOutcome Outcome() const noexcept { return outcome_.load(); }

    void SetOverlayVisible(bool visible);
    void SetAchievementCallback(std::function<bool(const std::string&, const std::string&)> callback) {
        achievementCallback_ = std::move(callback);
    }

private:
    RuntimeSession v2_;
    RuntimeIpcServer ipc_;
    ResumeStore resumeStore_;
    GameManifest activeGame_{};
    std::atomic<bool> ready_{false};
    std::atomic<RuntimeOutcome> outcome_{RuntimeOutcome::None};
    bool overlayVisible_{false};
    bool playtimeFinalized_{true};
    std::chrono::steady_clock::time_point readyAt_{};
    std::chrono::steady_clock::time_point readyDeadline_{};
    uint64_t playtimeSeconds_{0};
    std::function<bool(const std::string&, const std::string&)> achievementCallback_;

    static constexpr std::chrono::seconds kReadyTimeout{20};
    static constexpr std::chrono::seconds kHeartbeatTimeout{10};

    std::string CreateAuthToken() const;
    void PersistPlaytime() const;
};

} // namespace zero
