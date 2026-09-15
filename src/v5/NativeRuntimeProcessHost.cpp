#include "v5/NativeRuntimeProcessHost.h"
#include "ZeroTypes.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace zero::v5 {
namespace {

std::filesystem::path pathFromUtf8(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                           value.data(), static_cast<int>(value.size()),
                                           nullptr, 0);
    if (count <= 0) return {};
    std::wstring wide(static_cast<size_t>(count), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                             value.data(), static_cast<int>(value.size()),
                             wide.data(), count)) return {};
    return std::filesystem::path(wide);
}

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                          value.data(), static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                             value.data(), static_cast<int>(value.size()),
                             out.data(), bytes, nullptr, nullptr)) return {};
    return out;
}

bool hasCapability(const std::vector<RuntimeCapability>& values,
                   RuntimeCapability capability) {
    return std::find(values.begin(), values.end(), capability) != values.end();
}

std::string capabilityList(const std::vector<RuntimeCapability>& values) {
    std::ostringstream out;
    bool first = true;
    auto append = [&](const char* name) {
        if (!first) out << ' ';
        first = false;
        out << name;
    };
    for (const auto capability : values) {
        switch (capability) {
            case RuntimeCapability::SaveRead: append("save.read"); break;
            case RuntimeCapability::SaveWrite: append("save.write"); break;
            case RuntimeCapability::Achievements: append("achievements"); break;
            case RuntimeCapability::Overlay: append("overlay"); break;
            case RuntimeCapability::Input: append("input"); break;
            case RuntimeCapability::Capture: append("capture"); break;
            case RuntimeCapability::Presence: append("presence"); break;
            case RuntimeCapability::CloudState: append("cloud.state"); break;
        }
    }
    return out.str();
}

zero::RuntimeIpcPermissions permissionsFrom(const RuntimeSessionGrant& grant) {
    zero::RuntimeIpcPermissions permissions;
    permissions.resumeWrite = hasCapability(grant.grantedCapabilities, RuntimeCapability::SaveWrite);
    permissions.achievements = hasCapability(grant.grantedCapabilities, RuntimeCapability::Achievements);
    permissions.overlay = hasCapability(grant.grantedCapabilities, RuntimeCapability::Overlay);
    return permissions;
}

} // namespace

bool NativeRuntimeProcessHost::ValidateGrant(const LaunchDescriptor& launch,
                                             const RuntimeSessionGrant& grant,
                                             std::string& error) {
    if (grant.identity.sessionId.empty() || grant.identity.packageId.empty() ||
        grant.identity.accountId.empty()) {
        error = "runtime grant is missing authoritative identity";
        return false;
    }
    if (grant.identity.packageId != launch.packageId) {
        error = "runtime grant package does not match launch descriptor";
        return false;
    }
    if (grant.ipcAuthenticationToken.size() < 32) {
        error = "runtime IPC authentication token is missing or too short";
        return false;
    }
    return true;
}

bool NativeRuntimeProcessHost::Start(const LaunchDescriptor& launch,
                                     const RuntimeSessionGrant& grant,
                                     std::string& error) {
    if (started_ || process_.IsActive()) {
        error = "native runtime process is already active";
        return false;
    }
    if (!ValidateGrant(launch, grant, error)) return false;
    if (launch.runtimeType != RuntimeType::NativeWin32 &&
        launch.runtimeType != RuntimeType::Unreal &&
        launch.runtimeType != RuntimeType::Godot) {
        error = "native runtime host cannot execute this runtime type";
        return false;
    }
    if (!launch.arguments.empty()) {
        error = "native runtime arguments are not yet admitted by the V5 launch policy";
        return false;
    }

    auto root = pathFromUtf8(launch.contentRoot);
    auto executable = pathFromUtf8(launch.executable);
    if (root.empty() || executable.empty()) {
        error = "native runtime launch paths are missing or invalid UTF-8";
        return false;
    }
    if (executable.is_relative()) executable = root / executable;

    zero::GameManifest game;
    game.schemaVersion = 1;
    game.minimumRuntimeMajor = 5;
    game.packageId = launch.packageId;
    game.title = launch.packageId;
    game.version = launch.version;
    game.root = std::move(root);
    game.executable = std::move(executable);
    game.zeroResume = hasCapability(grant.grantedCapabilities, RuntimeCapability::SaveWrite);
    game.zeroAchievements = hasCapability(grant.grantedCapabilities, RuntimeCapability::Achievements);
    game.zeroOverlay = hasCapability(grant.grantedCapabilities, RuntimeCapability::Overlay);
    game.zeroInput = hasCapability(grant.grantedCapabilities, RuntimeCapability::Input);

    std::wstring processError;
    if (!process_.PrepareLaunchWithSession(game, grant.identity.sessionId, processError)) {
        error = utf8(processError);
        if (error.empty()) error = "native process supervisor rejected launch";
        return false;
    }

    if (process_.Info().sessionId != grant.identity.sessionId ||
        process_.Info().packageId != grant.identity.packageId) {
        process_.FailPrepared(0xE201, "v5_identity_mismatch");
        error = "native process identity diverged from authoritative runtime grant";
        return false;
    }

    const auto permissions = permissionsFrom(grant);
    std::wstring ipcError;
    if (!ipc_.StartSecure(grant.identity.sessionId,
                          grant.identity.packageId,
                          grant.ipcAuthenticationToken,
                          callbacks_, permissions, ipcError)) {
        process_.FailPrepared(0xE202, "v5_ipc_start_failed");
        error = utf8(ipcError);
        if (error.empty()) error = "secure IPC server failed to start";
        return false;
    }

    const auto bootstrap = process_.Info().tempRoot / L"runtime-v4.bootstrap";
    std::ofstream file(bootstrap, std::ios::binary | std::ios::trunc);
    if (!file) {
        ipc_.Stop();
        process_.FailPrepared(0xE203, "v5_bootstrap_open_failed");
        error = "V5 runtime could not create the SDK bootstrap contract";
        return false;
    }

    const auto pipe = utf8(ipc_.PipeName());
    if (pipe.empty()) {
        file.close();
        ipc_.Stop();
        process_.FailPrepared(0xE204, "v5_pipe_encoding_failed");
        error = "V5 runtime could not encode the secure IPC identity";
        return false;
    }

    // Protocol 4 is retained during the V5 migration so existing SDK games can
    // launch while authority moves to V5. The final line is ignored by the V4
    // SDK and records the V5 capability scope for forward-compatible SDK work.
    file << "4\n"
         << grant.identity.sessionId << "\n"
         << grant.identity.packageId << "\n"
         << pipe << "\n"
         << grant.ipcAuthenticationToken << "\n"
         << "0\n\n\n\n"
         << capabilityList(grant.grantedCapabilities) << "\n";
    file.flush();
    if (!file.good()) {
        file.close();
        ipc_.Stop();
        process_.FailPrepared(0xE205, "v5_bootstrap_commit_failed");
        error = "V5 runtime could not finalize the SDK bootstrap contract";
        return false;
    }
    file.close();

    if (!process_.ResumePrepared(processError)) {
        ipc_.Stop();
        error = utf8(processError);
        if (error.empty()) error = "native process supervisor could not resume the prepared process";
        return false;
    }

    activeGrant_ = grant;
    started_ = true;
    return true;
}

RuntimeProcessStatus NativeRuntimeProcessHost::Poll() {
    if (!started_) return {RuntimeProcessState::Exited, 0};
    process_.Poll();
    switch (process_.State()) {
        case zero::RuntimeState::Launching:
            return {RuntimeProcessState::Starting, process_.ExitCode()};
        case zero::RuntimeState::Running:
            return {RuntimeProcessState::Running, process_.ExitCode()};
        case zero::RuntimeState::Exited:
            ipc_.Stop();
            started_ = false;
            return {RuntimeProcessState::Exited, process_.ExitCode()};
        case zero::RuntimeState::Crashed:
        case zero::RuntimeState::Failed:
            ipc_.Stop();
            started_ = false;
            return {RuntimeProcessState::Crashed, process_.ExitCode()};
        case zero::RuntimeState::Idle:
            return {RuntimeProcessState::Exited, process_.ExitCode()};
    }
    return {RuntimeProcessState::Crashed, process_.ExitCode()};
}

void NativeRuntimeProcessHost::Terminate() {
    ipc_.Stop();
    process_.Terminate();
    started_ = false;
    activeGrant_ = {};
}

} // namespace zero::v5
