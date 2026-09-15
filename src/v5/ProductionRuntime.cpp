#include "v5/ProductionRuntime.h"
#include "PackageIntegrityVerifier.h"
#include <bcrypt.h>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace zero {
namespace {

const char* outcomeName(RuntimeOutcome outcome) {
    switch (outcome) {
        case RuntimeOutcome::None: return "none";
        case RuntimeOutcome::Starting: return "starting";
        case RuntimeOutcome::Running: return "running";
        case RuntimeOutcome::CleanExit: return "clean_exit";
        case RuntimeOutcome::Crash: return "crash";
        case RuntimeOutcome::UserTermination: return "user_termination";
        case RuntimeOutcome::LaunchFailure: return "launch_failure";
        case RuntimeOutcome::HandshakeFailure: return "handshake_failure";
        case RuntimeOutcome::ReadyTimeout: return "ready_timeout";
        case RuntimeOutcome::Hung: return "hung";
    }
    return "unknown";
}

std::string nowUtc() {
    const auto tp = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm utc{};
    gmtime_s(&utc, &tt);
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

} // namespace

class ProductionRuntime::LocalLaunchPolicy final : public v5::ILaunchPolicy {
public:
    bool Validate(const v5::RuntimeSessionRequest& request, std::string& error) const override {
        if (request.identity.sessionId.empty() || request.identity.packageId.empty() ||
            request.identity.accountId.empty() || request.identity.packageId != request.launch.packageId) {
            error = "V5 local launch identity is invalid";
            return false;
        }
        if (request.launch.sessionToken.empty() || request.launch.entitlementToken.empty() ||
            request.launch.version.empty() || request.launch.contentRoot.empty() || request.launch.executable.empty()) {
            error = "V5 local launch authority is incomplete";
            return false;
        }
        return true;
    }
};

class ProductionRuntime::LocalCapabilityBroker final : public v5::ICapabilityBroker {
public:
    explicit LocalCapabilityBroker(const ProductionRuntime& owner) : owner_(owner) {}

    std::optional<v5::RuntimeSessionGrant> Grant(const v5::RuntimeSessionRequest& request,
                                                  std::string& error) const override {
        const auto token = owner_.NewOpaqueToken(32);
        if (token.empty()) {
            error = "ZERO could not create the runtime IPC authentication token";
            return std::nullopt;
        }
        return v5::RuntimeSessionGrant{request.identity, request.requestedCapabilities, token};
    }
private:
    const ProductionRuntime& owner_;
};

ProductionRuntime::~ProductionRuntime() = default;

ProductionRuntime::ProductionRuntime()
    : policy_(std::make_unique<LocalLaunchPolicy>()),
      capabilities_(std::make_unique<LocalCapabilityBroker>(*this)),
      host_(),
      authority_(std::make_unique<v5::RuntimeAuthority>(*policy_, *capabilities_, host_)),
      localDatabase_(),
      sessions_(localDatabase_.Database()),
      resumes_(localDatabase_.Database()),
      achievements_(localDatabase_.Database()) {
    RuntimeIpcCallbacks callbacks;
    callbacks.onReady = [this]() {
        if (readyAt_.time_since_epoch().count() == 0) readyAt_ = std::chrono::steady_clock::now();
        if (outcome_ == RuntimeOutcome::Starting) outcome_ = RuntimeOutcome::Running;
    };
    callbacks.onResume = [this](const std::string& activityId,
                                const std::string& displayLabel,
                                const std::string& payload) {
        if (activePackageId_.empty()) return false;
        ResumeMetadata metadata;
        metadata.packageId = activePackageId_;
        metadata.activityId = activityId;
        metadata.displayLabel = displayLabel;
        metadata.payload = payload;
        metadata.updatedAtUtc = nowUtc();
        std::string error;
        return resumes_.Save(metadata, error);
    };
    callbacks.onAchievement = [this](const std::string& achievementId,
                                     const std::string& title) {
        std::wstring error;
        return UnlockAchievement(achievementId, title, error);
    };
    host_.SetCallbacks(std::move(callbacks));
}

std::string ProductionRuntime::NewOpaqueToken(size_t bytes) const {
    if (bytes == 0 || bytes > 256) return {};
    std::vector<unsigned char> random(bytes);
    if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return {};
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto value : random) out << std::setw(2) << static_cast<unsigned>(value);
    return out.str();
}

std::string ProductionRuntime::PathUtf8(const std::filesystem::path& path) {
    const auto wide = path.wstring();
    if (wide.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                          wide.data(), static_cast<int>(wide.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                             wide.data(), static_cast<int>(wide.size()),
                             out.data(), bytes, nullptr, nullptr)) return {};
    return out;
}

std::wstring ProductionRuntime::Widen(const std::string& text) {
    if (text.empty()) return {};
    const int chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                           text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (chars <= 0) return {};
    std::wstring out(static_cast<size_t>(chars), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(), chars);
    return out;
}

bool ProductionRuntime::Launch(const GameManifest& game,
                               std::wstring& error,
                               const std::optional<ResumeMetadata>& launchResume) {
    if (IsActive()) {
        error = L"A ZERO runtime session is already active.";
        return false;
    }
    if (!PackageIntegrityVerifier::Verify(game.root, error)) return false;
    const auto trust = PackageTrust(game);
    if (!trust.launchAllowed) {
        error = trust.detail.empty() ? L"ZERO blocked launch because package trust validation failed." : trust.detail;
        return false;
    }

    std::string repositoryError;
    if (!localDatabase_.Initialize(repositoryError)) {
        error = Widen(repositoryError);
        return false;
    }
    std::wstring databaseError;
    if (!localDatabase_.Database()->UpsertGame(game, databaseError)) {
        error = databaseError;
        return false;
    }

    const auto sessionId = NewOpaqueToken(16);
    const auto sessionToken = NewOpaqueToken(32);
    const auto entitlementToken = NewOpaqueToken(32);
    if (sessionId.empty() || sessionToken.empty() || entitlementToken.empty()) {
        error = L"ZERO could not create local-package launch authority.";
        return false;
    }

    v5::LaunchDescriptor descriptor;
    descriptor.packageId = game.packageId;
    descriptor.version = game.version;
    descriptor.runtimeType = v5::RuntimeType::NativeWin32;
    descriptor.contentRoot = PathUtf8(game.root);
    descriptor.executable = PathUtf8(game.executable);
    descriptor.saveNamespace = game.packageId;
    descriptor.entitlementToken = "local-entitlement-" + entitlementToken;
    descriptor.sessionToken = "local-session-" + sessionToken;

    v5::RuntimeSessionRequest request;
    request.launch = descriptor;
    request.identity = {sessionId, game.packageId, "local-package-user"};
    if (game.zeroResume) {
        request.requestedCapabilities.push_back(v5::RuntimeCapability::SaveRead);
        request.requestedCapabilities.push_back(v5::RuntimeCapability::SaveWrite);
    }
    if (game.zeroAchievements) request.requestedCapabilities.push_back(v5::RuntimeCapability::Achievements);
    if (game.zeroOverlay) request.requestedCapabilities.push_back(v5::RuntimeCapability::Overlay);
    if (game.zeroInput) request.requestedCapabilities.push_back(v5::RuntimeCapability::Input);

    host_.SetLaunchResume(launchResume);
    const auto started = authority_->Start(request);
    if (!started.Started()) {
        outcome_ = RuntimeOutcome::LaunchFailure;
        state_ = RuntimeState::Failed;
        error = Widen(started.detail.empty() ? "V5 runtime rejected the launch" : started.detail);
        return false;
    }

    activeGrant_ = *started.grant;
    activePackageId_ = game.packageId;
    state_ = RuntimeState::Launching;
    outcome_ = RuntimeOutcome::Starting;
    exitCode_ = STILL_ACTIVE;
    sessionRecorded_ = false;
    crashReported_ = false;
    userTermination_ = false;
    playtimeFinalized_ = false;
    finalPlaytimeSeconds_ = 0;
    launchedAt_ = std::chrono::steady_clock::now();
    readyAt_ = {};
    return true;
}

uint64_t ProductionRuntime::PlaytimeSeconds() const noexcept {
    if (playtimeFinalized_ || readyAt_.time_since_epoch().count() == 0) return finalPlaytimeSeconds_;
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - readyAt_).count());
}

void ProductionRuntime::RecordCrashIfNeeded() {
    if (crashReported_ || (state_ != RuntimeState::Crashed && outcome_ != RuntimeOutcome::Hung &&
                           outcome_ != RuntimeOutcome::ReadyTimeout)) return;

    v5::RuntimeSnapshot snapshot = authority_->Snapshot();
    snapshot.state = v5::RuntimeLifecycleState::Failed;
    snapshot.exitCode = exitCode_;
    const auto event = crashSupervisor_.Observe(snapshot, [this]() {
        const auto& info = host_.ProcessInfo();
        return info.miniDumpWritten || host_.CaptureDiagnosticDump();
    });
    if (!event) return;

    const auto& info = host_.ProcessInfo();
    CrashReport report;
    report.sessionId = info.sessionId;
    report.packageId = info.packageId;
    report.title = info.title;
    report.version = info.version;
    report.executable = info.executable.string();
    report.processId = info.processId;
    report.exitCode = exitCode_;
    report.playtimeSeconds = PlaytimeSeconds();
    report.forcedTermination = info.forcedTermination;
    report.outcome = outcomeName(outcome_);
    report.miniDumpWritten = info.miniDumpWritten || event->dumpCaptured;
    report.miniDumpPath = info.miniDumpPath.string();
    report.miniDumpError = info.miniDumpError;
    report.miniDumpNote = info.miniDumpNote;
    std::wstring ignored;
    if (crashReports_.Save(report, ignored)) crashReported_ = true;
}

void ProductionRuntime::FinalizePersistence() {
    if (sessionRecorded_ || activePackageId_.empty() || activeGrant_.identity.sessionId.empty()) return;
    finalPlaytimeSeconds_ = PlaytimeSeconds();
    playtimeFinalized_ = true;
    v5::SessionRecord record;
    record.packageId = activePackageId_;
    record.sessionId = activeGrant_.identity.sessionId;
    record.playtimeSeconds = finalPlaytimeSeconds_;
    record.exitCode = exitCode_;
    record.outcome = outcomeName(outcome_);
    record.crashed = state_ == RuntimeState::Crashed || outcome_ == RuntimeOutcome::Hung ||
                     outcome_ == RuntimeOutcome::ReadyTimeout;
    std::string repositoryError;
    if (!sessions_.Record(record, repositoryError)) return;
    sessionRecorded_ = true;
}

void ProductionRuntime::Poll() {
    if (!IsActive()) return;

    const auto snapshot = authority_->Poll();
    const auto now = std::chrono::steady_clock::now();
    if (host_.ReadyReceived() && readyAt_.time_since_epoch().count() == 0) {
        readyAt_ = now;
        state_ = RuntimeState::Running;
        outcome_ = RuntimeOutcome::Running;
    }

    if (!host_.ReadyReceived() && now - launchedAt_ > kReadyTimeout) {
        host_.CaptureDiagnosticDump();
        authority_->Terminate();
        state_ = RuntimeState::Failed;
        outcome_ = RuntimeOutcome::ReadyTimeout;
        exitCode_ = 0xE301;
        RecordCrashIfNeeded();
        FinalizePersistence();
        return;
    }

    if (host_.ReadyReceived() && host_.ClientSilence() > kHeartbeatTimeout) {
        host_.CaptureDiagnosticDump();
        authority_->Terminate();
        state_ = RuntimeState::Crashed;
        outcome_ = RuntimeOutcome::Hung;
        exitCode_ = 0xE302;
        RecordCrashIfNeeded();
        FinalizePersistence();
        return;
    }

    if (snapshot.state == v5::RuntimeLifecycleState::Failed) {
        state_ = RuntimeState::Crashed;
        outcome_ = RuntimeOutcome::Crash;
        exitCode_ = snapshot.exitCode;
        RecordCrashIfNeeded();
        FinalizePersistence();
    } else if (snapshot.state == v5::RuntimeLifecycleState::Exited) {
        state_ = RuntimeState::Exited;
        exitCode_ = snapshot.exitCode;
        outcome_ = userTermination_ ? RuntimeOutcome::UserTermination : RuntimeOutcome::CleanExit;
        FinalizePersistence();
    }
}

void ProductionRuntime::Terminate() {
    if (!IsActive()) return;
    userTermination_ = true;
    authority_->Terminate();
    state_ = RuntimeState::Exited;
    outcome_ = RuntimeOutcome::UserTermination;
    exitCode_ = host_.ProcessInfo().exitCode;
    FinalizePersistence();
}

void ProductionRuntime::SetOverlayVisible(bool visible) {
    if (!IsActive()) return;
    host_.SendOverlayFocus(visible);
}

bool ProductionRuntime::UnlockAchievement(const std::string& achievementId,
                                          const std::string& title,
                                          std::wstring& error) {
    if (activePackageId_.empty()) {
        error = L"ZERO cannot unlock an achievement without an active game package.";
        return false;
    }
    if (achievementId.empty() || achievementId.size() > 160 || title.empty() || title.size() > 256) {
        error = L"The achievement identity is invalid.";
        return false;
    }
    std::string repositoryError;
    if (!achievements_.Save(activePackageId_, achievementId, title, nowUtc(), repositoryError)) {
        error = Widen(repositoryError.empty() ? "achievement repository rejected the unlock" : repositoryError);
        return false;
    }
    return true;
}

bool ProductionRuntime::ApplyAuthoritativeAchievementDefinition(v5::AchievementDefinition definition,
                                                                 std::string& error) {
    return authoritativeAchievements_.ApplyDefinition(std::move(definition), error);
}

bool ProductionRuntime::ApplyAuthoritativeAchievementUnlock(v5::AchievementUnlock unlock,
                                                             std::string& error) {
    return authoritativeAchievements_.ApplyUnlock(std::move(unlock), error);
}

std::vector<v5::AchievementDefinition> ProductionRuntime::AchievementDefinitions(const std::string& packageId) const {
    return authoritativeAchievements_.DefinitionsFor(packageId);
}

std::vector<v5::AchievementUnlock> ProductionRuntime::AuthoritativeAchievementUnlocks(const std::string& accountId,
                                                                                       const std::string& packageId) const {
    return authoritativeAchievements_.UnlocksFor(accountId, packageId);
}

std::uint64_t ProductionRuntime::AuthoritativeAchievementScore(const std::string& accountId) const {
    return authoritativeAchievements_.ScoreFor(accountId);
}

GamePlatformState ProductionRuntime::PlatformState(const std::string& packageId) const {
    std::string repositoryError;
    if (!localDatabase_.Initialize(repositoryError)) return {};
    std::wstring error;
    const auto state = localDatabase_.Database()->LoadGameState(packageId, error);
    return state.value_or(GamePlatformState{});
}

std::optional<ResumeMetadata> ProductionRuntime::Resume(const std::string& packageId) const {
    std::string repositoryError;
    if (!localDatabase_.Initialize(repositoryError)) return std::nullopt;
    return resumes_.Load(packageId, repositoryError);
}

std::vector<AchievementRecord> ProductionRuntime::Achievements(const std::string& packageId) const {
    std::string repositoryError;
    if (!localDatabase_.Initialize(repositoryError)) return {};
    return achievements_.Load(packageId, repositoryError);
}

PackageTrustResult ProductionRuntime::PackageTrust(const GameManifest& game) const {
    return PackageTrustService::Evaluate(game.root, trustPolicy_, trustProvider_);
}

} // namespace zero
