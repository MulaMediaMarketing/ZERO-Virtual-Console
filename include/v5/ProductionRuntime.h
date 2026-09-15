#pragma once

#include "AchievementStore.h"
#include "CrashReportStore.h"
#include "PackageTrust.h"
#include "PlatformStateStore.h"
#include "ResumeStore.h"
#include "RuntimeSession.h"
#include "v5/CrashSupervisor.h"
#include "v5/DomainRepositories.h"
#include "v5/NativeRuntimeProcessHost.h"
#include "v5/ResumeCoordinator.h"
#include "v5/RuntimeAuthority.h"
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

class ProductionRuntime {
public:
    ProductionRuntime();
    ~ProductionRuntime();

    bool Launch(const GameManifest& game,
                std::wstring& error,
                const std::optional<ResumeMetadata>& launchResume = std::nullopt);
    void Poll();
    void Terminate();
    void SetOverlayVisible(bool visible);

    RuntimeState State() const noexcept { return state_; }
    RuntimeOutcome Outcome() const noexcept { return outcome_; }
    DWORD ExitCode() const noexcept { return exitCode_; }
    const RuntimeSessionInfo& Info() const noexcept { return host_.ProcessInfo(); }
    bool IsActive() const noexcept { return state_ == RuntimeState::Launching || state_ == RuntimeState::Running; }
    bool Ready() const noexcept { return host_.ReadyReceived(); }
    uint64_t PlaytimeSeconds() const noexcept;

    bool UnlockAchievement(const std::string& achievementId,
                           const std::string& title,
                           std::wstring& error);
    GamePlatformState PlatformState(const std::string& packageId) const;
    std::optional<ResumeMetadata> Resume(const std::string& packageId) const;
    std::vector<AchievementRecord> Achievements(const std::string& packageId) const;
    PackageTrustResult PackageTrust(const GameManifest& game) const;

private:
    class LocalLaunchPolicy;
    class LocalCapabilityBroker;

    std::unique_ptr<LocalLaunchPolicy> policy_;
    std::unique_ptr<LocalCapabilityBroker> capabilities_;
    v5::NativeRuntimeProcessHost host_;
    std::unique_ptr<v5::RuntimeAuthority> authority_;
    v5::CrashSupervisor crashSupervisor_;

    v5::LocalDomainDatabase localDatabase_;
    v5::LocalSessionRepository sessions_;
    PlatformStateStore stateStore_;
    AchievementStore achievements_;
    ResumeStore resumeStore_;
    CrashReportStore crashReports_;
    CngPublisherTrustProvider trustProvider_;
    PackageTrustPolicy trustPolicy_{PackageTrustPolicy::AllowLocalUnsigned};

    RuntimeState state_{RuntimeState::Idle};
    RuntimeOutcome outcome_{RuntimeOutcome::None};
    DWORD exitCode_{0};
    std::string activePackageId_;
    v5::RuntimeSessionGrant activeGrant_{};
    bool sessionRecorded_{true};
    bool crashReported_{false};
    bool userTermination_{false};
    std::chrono::steady_clock::time_point launchedAt_{};
    std::chrono::steady_clock::time_point readyAt_{};
    mutable uint64_t finalPlaytimeSeconds_{0};

    static constexpr std::chrono::seconds kReadyTimeout{20};
    static constexpr std::chrono::seconds kHeartbeatTimeout{10};

    void FinalizePersistence();
    void RecordCrashIfNeeded();
    std::string NewOpaqueToken(size_t bytes) const;
    static std::string PathUtf8(const std::filesystem::path& path);
    static std::wstring Widen(const std::string& text);
};

} // namespace zero
