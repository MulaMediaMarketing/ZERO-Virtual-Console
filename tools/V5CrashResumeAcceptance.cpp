#include "PlatformDatabase.h"
#include "ZeroTypes.h"
#include "v5/CrashSupervisor.h"
#include "v5/ResumeCoordinator.h"
#include <iostream>
#include <string>

using namespace zero::v5;

namespace {
RuntimeSessionGrant grant(std::vector<RuntimeCapability> capabilities) {
    RuntimeSessionGrant value;
    value.identity = {"resume-session", "zero.v5.acceptance.resume", "acct-1"};
    value.grantedCapabilities = std::move(capabilities);
    value.ipcAuthenticationToken = std::string(64, 'x');
    return value;
}
}

int main() {
    // Resume rows are foreign-key bound to a registered package. Seed the same
    // canonical package identity that a production launch registers before the
    // runtime SDK is allowed to persist Resume state.
    zero::PlatformDatabase database;
    zero::GameManifest game;
    game.packageId = "zero.v5.acceptance.resume";
    game.title = "V5 Resume Acceptance";
    game.version = "1.0.0";
    game.executable = L"V5ResumeAcceptance.exe";
    std::wstring databaseError;
    if (!database.UpsertGame(game, databaseError)) return 1;

    ResumeCoordinator resume;
    std::string error;

    ResumeWriteRequest denied;
    denied.grant = grant({RuntimeCapability::SaveRead});
    denied.activityId = "checkpoint-1";
    denied.displayLabel = "Checkpoint One";
    denied.payload = "state";
    if (resume.Save(denied, error)) return 10;

    ResumeWriteRequest write;
    write.grant = grant({RuntimeCapability::SaveRead, RuntimeCapability::SaveWrite});
    write.activityId = "checkpoint-1";
    write.displayLabel = "Checkpoint One";
    write.payload = "state-v5";
    error.clear();
    if (!resume.Save(write, error)) return 11;

    const auto loaded = resume.LoadForLaunch(write.grant, error);
    if (!loaded || loaded->packageId != write.grant.identity.packageId ||
        loaded->activityId != write.activityId || loaded->payload != write.payload) return 12;

    ResumeWriteRequest oversized = write;
    oversized.payload.assign(64u * 1024u + 1u, 'a');
    if (resume.Save(oversized, error)) return 13;

    if (!resume.ClearForPackage(write.grant, error)) return 14;
    if (resume.LoadForLaunch(write.grant, error)) return 15;

    CrashSupervisor crashes;
    RuntimeSnapshot healthy;
    healthy.state = RuntimeLifecycleState::Running;
    healthy.identity = {"crash-session", "pkg.crash", "acct-1"};
    if (crashes.Observe(healthy, [] { return true; })) return 20;

    RuntimeSnapshot failed = healthy;
    failed.state = RuntimeLifecycleState::Failed;
    failed.exitCode = 0xC0000005u;
    int captures = 0;
    const auto event = crashes.Observe(failed, [&captures] { ++captures; return true; });
    if (!event || !event->dumpCaptured || event->exitCode != failed.exitCode || captures != 1) return 21;
    if (crashes.Observe(failed, [&captures] { ++captures; return true; })) return 22;
    if (captures != 1) return 23;

    crashes.Reset(failed.identity.sessionId);
    if (!crashes.Observe(failed, [&captures] { ++captures; return false; })) return 24;
    if (captures != 2) return 25;

    std::cout << "ZERO V5 crash supervisor + resume coordinator acceptance: PASS\n";
    return 0;
}
