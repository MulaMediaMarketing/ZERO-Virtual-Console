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

} // namespace

std::string RuntimeV3::CreateAuthToken() const {
    unsigned char bytes[32]{};
    if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0) {
        return hex(bytes, sizeof(bytes));
    }
    const auto fallback = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return "fallback-" + std::to_string(fallback);
}

bool RuntimeV3::Launch(const GameManifest& game, std::wstring& error) {
    if (IsActive()) {
        error = L"A game session is already active.";
        return false;
    }

    ready_ = false;
    overlayVisible_ = false;
    playtimeFinalized_ = false;
    playtimeSeconds_ = 0;
    activeGame_ = game;

    if (!v2_.Launch(game, error)) {
        playtimeFinalized_ = true;
        return false;
    }

    const auto token = CreateAuthToken();
    RuntimeIpcCallbacks callbacks;
    callbacks.onReady = [this]() {
        if (!ready_) {
            ready_ = true;
            readyAt_ = std::chrono::steady_clock::now();
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

    if (!ipc_.Start(v2_.Info().sessionId, game.packageId, token, std::move(callbacks), error)) {
        v2_.Terminate();
        playtimeFinalized_ = true;
        return false;
    }

    // Runtime V2 currently builds its environment before V3 IPC exists. Publish the
    // bootstrap contract in the session temp directory so SDK clients can discover it
    // without putting secrets on the command line. Environment injection moves into the
    // launcher itself in the next hardening pass.
    const auto bootstrap = v2_.Info().tempRoot / "runtime-v3.bootstrap";
    std::ofstream f(bootstrap, std::ios::binary | std::ios::trunc);
    if (!f) {
        error = L"ZERO Runtime V3 could not create the SDK bootstrap contract.";
        ipc_.Stop();
        v2_.Terminate();
        playtimeFinalized_ = true;
        return false;
    }
    std::string pipe(ipc_.PipeName().begin(), ipc_.PipeName().end());
    f << "3\n" << v2_.Info().sessionId << "\n" << game.packageId << "\n" << pipe << "\n" << token << "\n";
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
    const auto previous = v2_.State();
    v2_.Poll();

    if (ready_) {
        playtimeSeconds_ = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - readyAt_).count());
    }

    const auto current = v2_.State();
    const bool ended = current == RuntimeState::Exited || current == RuntimeState::Crashed || current == RuntimeState::Failed;
    if (ended && !playtimeFinalized_) {
        PersistPlaytime();
        ipc_.Stop();
        playtimeFinalized_ = true;
    }

    (void)previous;
}

void RuntimeV3::Terminate() {
    if (ready_) {
        playtimeSeconds_ = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - readyAt_).count());
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
