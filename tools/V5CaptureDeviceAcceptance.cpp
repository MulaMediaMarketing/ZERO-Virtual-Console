#include "v5/CaptureDeviceDomain.h"
#include <iostream>

using namespace zero::v5;

int main() {
    CaptureDomain captures;
    std::string error;
    CaptureRecord screenshot{"cap-1", "acct-1", "content-1", CaptureKind::Screenshot,
                             "C:/captures/cap-1.png", 1000, 0, CaptureSyncState::LocalOnly, false};
    if (!captures.Add(screenshot, error)) return 10;
    if (!captures.SetFavorite("cap-1", true)) return 11;
    if (!captures.SetSyncState("cap-1", CaptureSyncState::PendingUpload, error)) return 12;
    if (!captures.SetSyncState("cap-1", CaptureSyncState::Synced, error)) return 13;
    if (captures.SetSyncState("cap-1", CaptureSyncState::PendingUpload, error)) return 14;

    auto badScreenshot = screenshot;
    badScreenshot.captureId = "cap-2";
    badScreenshot.durationMilliseconds = 500;
    if (captures.Add(badScreenshot, error)) return 15;

    DeviceAuthority devices;
    DeviceRecord pc{"device-1", "acct-1", "ZERO PLAYER PC", DeviceType::Pc, DeviceState::Online,
                    "5.0.0", true, AuthoritySource::ZeroService};
    if (!devices.Apply(pc, error)) return 20;
    if (!devices.CanRemoteInstall("device-1", "acct-1", error)) return 21;
    if (devices.CanRemoteInstall("device-1", "acct-2", error)) return 22;

    auto forged = pc;
    forged.deviceId = "device-2";
    forged.authority = AuthoritySource::LocalPackage;
    if (devices.Apply(forged, error)) return 23;

    auto revoked = pc;
    revoked.state = DeviceState::Revoked;
    if (!devices.Apply(revoked, error)) return 24;
    if (devices.CanRemoteInstall("device-1", "acct-1", error)) return 25;

    std::cout << "ZERO V5 Capture + Devices acceptance: PASS\n";
    return 0;
}
