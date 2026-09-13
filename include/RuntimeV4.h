#pragma once
#include "AchievementStore.h"
#include "PlatformStateStore.h"
#include "RuntimeV3.h"

namespace zero {

class RuntimeV4 {
public:
    bool Launch(const GameManifest& game, std::wstring& error);
    void Poll();
    void Terminate();
    void SetOverlayVisible(bool visible) { v3_.SetOverlayVisible(visible); }

    RuntimeState State() const noexcept { return v3_.State(); }
    DWORD ExitCode() const noexcept { return v3_.ExitCode(); }
    const RuntimeSessionInfo& Info() const noexcept { return v3_.Info(); }
    bool IsActive() const noexcept { return v3_.IsActive(); }
    bool Ready() const noexcept { return v3_.Ready(); }
    uint64_t PlaytimeSeconds() const noexcept { return v3_.PlaytimeSeconds(); }

    GamePlatformState PlatformState(const std::string& packageId) const { return stateStore_.Load(packageId); }
    std::vector<AchievementRecord> Achievements(const std::string& packageId) const { return achievements_.Load(packageId); }

private:
    RuntimeV3 v3_;
    PlatformStateStore stateStore_;
    AchievementStore achievements_;
    std::string activePackageId_;
    std::string activeSessionId_;
    bool sessionRecorded_{true};
};

} // namespace zero
