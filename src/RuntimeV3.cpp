#include "RuntimeV3.h"
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <shlobj.h>
#include <sstream>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

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

std::string hex(const unsigned char* data, size_t n) {
    static constexpr char lut[] = "0123456789abcdef";
    std::string out;
    out.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        out.push_back(lut[(data[i] >> 4) & 0xF]);
        out.push_back(lut[data[i] & 0xF]);
    }
    return out;
}

std::string hexString(const std::string& value) {
    return hex(reinterpret_cast<const unsigned char*>(value.data()), value.size());
}

} // namespace

std::string RuntimeV3::CreateAuthToken() const {
    unsigned char bytes[32]{};
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        return {};
    }
    return hex(bytes, sizeof(bytes));
}

bool RuntimeV3::Launch(const GameManifest& game, std::wstring& error,
                       const std::optional<ResumeMetadata>& launchResume) {
    if (IsActive()) {
        error = L"A game session is already active.";
        return false;
    }

    ready_.store(false);
    outcome_.store(RuntimeOutcome::Starting);
    overlayVisible_ = false;
    playtimeFinalized_ = false;
    playtimeSeconds_ = 0;
    activeGame_ = game;

    if (!v2_.PrepareLaunch(game, error)) {
        outcome_.store(RuntimeOutcome::LaunchFailure);
        playtimeFinalized_ = true;
        return false;
    }

    const auto token = CreateAuthToken();
    if (token.empty()) {
        error = L"ZERO Runtime could not generate a cryptographically secure IPC authentication token.";
        outcome_.store(RuntimeOutcome::LaunchFailure);
        v2_.FailPrepared(0xE110, "auth_token_failed");
        playtimeFinalized_ = true;
        return false;
    }

    RuntimeIpcCallbacks callbacks;
    callbacks.onReady = [this]() {
        bool expected = false;
        if (ready_.compare_exchange_strong(expected, true)) {
            readyAt_ = std::chrono::steady_clock::now();
            outcome_.store(RuntimeOutcome::Running);
        }
    };
    callbacks.onResume = [this](const std::string& activityId,
                                const std::string& displayLabel,
                                const std::string& payload) {
        ResumeMetadata metadata;
        metadata.packageId = activeGame_.packageId;
        metadata.activityId = activityId;
        metadata.displayLabel = displayLabel;
        metadata.payload = payload;
        std::wstring ignored;
        resumeStore_.Save(metadata, ignored);
    };
    callbacks.onAchievement = [this](const std::string& id, const std::string& title) {
        if (achievementCallback_) achievementCallback_(id, title);
    };

    if (!ipc_.Start(v2_.Info().sessionId, game.packageId, token, std::move(callbacks), error)) {
        outcome_.store(RuntimeOutcome::HandshakeFailure);
        v2_.FailPrepared(0xE111, "ipc_start_failed");
        playtimeFinalized_ = true;
        return false;
    }

    const auto bootstrap = v2_.Info().tempRoot / "runtime-v4.bootstrap";
    std::ofstream f(bootstrap, std::ios::binary | std::ios::trunc);
    if (!f) {
        error = L"ZERO Runtime V4 could not create the SDK bootstrap contract.";
        outcome_.store(RuntimeOutcome::HandshakeFailure);
        ipc_.Stop();
        v2_.FailPrepared(0xE112, "bootstrap_failed");
        playtimeFinalized_ = true;
        return false;
    }

    std::string pipe(ipc_.PipeName().begin(), ipc_.PipeName().end());
    f << "4\n" << v2_.Info().sessionId << "\n" << game.packageId << "\n" << pipe << "\n" << token << "\n";
    if (launchResume) {
        f << "1\n"
          << hexString(launchResume->activityId) << "\n"
          << hexString(launchResume->displayLabel) << "\n"
          << hexString(launchResume->payload) << "\n";
    } else {
        f << "0\n\n\n\n";
    }
    f.flush();
    if (!f.good()) {
        error = L"ZERO Runtime V4 could not finalize the SDK bootstrap contract.";
        outcome_.store(RuntimeOutcome::HandshakeFailure);
        f.close();
        ipc_.Stop();
        v2_.FailPrepared(0xE113, "bootstrap_commit_failed");
        playtimeFinalized_ = true;
        return false;
    }
    f.close();

    if (!v2_.ResumePrepared(error)) {
        outcome_.store(RuntimeOutcome::LaunchFailure);
        ipc_.Stop();
        playtimeFinalized_ = true;
        return false;
    }

    readyDeadline_ = std::chrono::steady_clock::now() + kReadyTimeout;
    return true;
}

void RuntimeV3::PersistPlaytime() const {
    if (activeGame_.packageId.empty()) return;
    std::error_code ec;
    const auto dir = zeroDataRoot() / "Playtime";
    std::filesystem::create_directories(dir, ec);
    if (ec) return;
    std::ofstream f(dir / (std::filesystem::path(activeGame_.packageId) += L".txt"), std::ios::trunc);
    if (f) f << playtimeSeconds_ << "\n";
}

void RuntimeV3::Poll() {
    v2_.Poll();

    const bool isReady = ready_.load();
    if (isReady) {
        playtimeSeconds_ = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - readyAt_).count());

        if (v2_.IsActive() && ipc_.ClientSilence() >= kHeartbeatTimeout) {
            outcome_.store(RuntimeOutcome::Hung);
            PersistPlaytime();
            ipc_.Stop();
            v2_.Terminate();
            playtimeFinalized_ = true;
            return;
        }
    } else if (v2_.IsActive() && std::chrono::steady_clock::now() >= readyDeadline_) {
        outcome_.store(RuntimeOutcome::ReadyTimeout);
        ipc_.Stop();
        v2_.Terminate();
        playtimeFinalized_ = true;
        return;
    }

    const auto current = v2_.State();
    const bool ended = current == RuntimeState::Exited || current == RuntimeState::Crashed || current == RuntimeState::Failed;
    if (ended && !playtimeFinalized_) {
        if (current == RuntimeState::Crashed) {
            outcome_.store(RuntimeOutcome::Crash);
        } else if (current == RuntimeState::Exited && outcome_.load() != RuntimeOutcome::UserTermination &&
                   outcome_.load() != RuntimeOutcome::Hung) {
            outcome_.store(RuntimeOutcome::CleanExit);
        } else if (current == RuntimeState::Failed && outcome_.load() == RuntimeOutcome::Starting) {
            outcome_.store(RuntimeOutcome::LaunchFailure);
        }
        PersistPlaytime();
        ipc_.Stop();
        playtimeFinalized_ = true;
    }
}

void RuntimeV3::Terminate() {
    if (ready_.load()) {
        playtimeSeconds_ = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - readyAt_).count());
    }
    if (v2_.IsActive() && outcome_.load() != RuntimeOutcome::ReadyTimeout && outcome_.load() != RuntimeOutcome::Hung) {
        outcome_.store(RuntimeOutcome::UserTermination);
    }
    PersistPlaytime();
    ipc_.Stop();
    v2_.Terminate();
    playtimeFinalized_ = true;
}

void RuntimeV3::SetOverlayVisible(bool visible) {
    overlayVisible_ = visible;
    ipc_.SendOverlayFocus(visible);
}

} // namespace zero
