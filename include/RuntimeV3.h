#pragma once
#include "ResumeStore.h"
#include "RuntimeIpcServer.h"
#include "RuntimeSession.h"
#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace zero {

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
    bool Ready() const noexcept { return ready_; }
    uint64_t PlaytimeSeconds() const noexcept { return playtimeSeconds_; }

    void SetOverlayVisible(bool visible);
    void SetAchievementCallback(std::function<void(const std::string&, const std::string&)> callback) {
        achievementCallback_ = std::move(callback);
    }

private:
    RuntimeSession v2_;
    RuntimeIpcServer ipc_;
    ResumeStore resumeStore_;
    GameManifest activeGame_{};
    bool ready_{false};
    bool overlayVisible_{false};
    bool playtimeFinalized_{true};
    std::chrono::steady_clock::time_point readyAt_{};
    uint64_t playtimeSeconds_{0};
    std::function<void(const std::string&, const std::string&)> achievementCallback_;

    std::string CreateAuthToken() const;
    void PersistPlaytime() const;
};

} // namespace zero
