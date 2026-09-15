#include "v5/CaptureDeviceDomain.h"
#include <algorithm>

namespace zero::v5 {

bool CaptureDomain::Add(CaptureRecord record, std::string& error) {
    if (record.captureId.empty() || record.accountId.empty() || record.localPath.empty()) {
        error = "capture requires capture, account, and local path identity";
        return false;
    }
    if (record.kind == CaptureKind::Screenshot && record.durationMilliseconds != 0) {
        error = "screenshot cannot have video duration";
        return false;
    }
    if (captures_.contains(record.captureId)) {
        error = "capture identity already exists";
        return false;
    }
    captures_.emplace(record.captureId, std::move(record));
    return true;
}

bool CaptureDomain::SetFavorite(const std::string& captureId, bool favorite) {
    const auto it = captures_.find(captureId);
    if (it == captures_.end()) return false;
    it->second.favorite = favorite;
    return true;
}

bool CaptureDomain::SetSyncState(const std::string& captureId,
                                 CaptureSyncState state,
                                 std::string& error) {
    const auto it = captures_.find(captureId);
    if (it == captures_.end()) {
        error = "capture does not exist";
        return false;
    }
    const auto current = it->second.syncState;
    const bool allowed = current == state ||
        (current == CaptureSyncState::LocalOnly && state == CaptureSyncState::PendingUpload) ||
        (current == CaptureSyncState::PendingUpload && (state == CaptureSyncState::Synced || state == CaptureSyncState::Failed)) ||
        (current == CaptureSyncState::Failed && state == CaptureSyncState::PendingUpload);
    if (!allowed) {
        error = "capture sync transition is not allowed";
        return false;
    }
    it->second.syncState = state;
    return true;
}

std::optional<CaptureRecord> CaptureDomain::Find(const std::string& captureId) const {
    const auto it = captures_.find(captureId);
    return it == captures_.end() ? std::nullopt : std::optional<CaptureRecord>{it->second};
}

std::vector<CaptureRecord> CaptureDomain::ForAccount(const std::string& accountId) const {
    std::vector<CaptureRecord> result;
    for (const auto& [_, capture] : captures_) if (capture.accountId == accountId) result.push_back(capture);
    std::stable_sort(result.begin(), result.end(), [](const CaptureRecord& a, const CaptureRecord& b) {
        return a.createdEpochSeconds > b.createdEpochSeconds;
    });
    return result;
}

bool DeviceAuthority::Apply(DeviceRecord device, std::string& error) {
    if (device.authority != AuthoritySource::ZeroService) {
        error = "device record was not issued by ZERO device authority";
        return false;
    }
    if (device.deviceId.empty() || device.accountId.empty() || device.displayName.empty()) {
        error = "device record is missing identity";
        return false;
    }
    if (device.state == DeviceState::Revoked) device.trusted = false;
    devices_[device.deviceId] = std::move(device);
    return true;
}

std::optional<DeviceRecord> DeviceAuthority::Find(const std::string& deviceId) const {
    const auto it = devices_.find(deviceId);
    return it == devices_.end() ? std::nullopt : std::optional<DeviceRecord>{it->second};
}

std::vector<DeviceRecord> DeviceAuthority::ForAccount(const std::string& accountId) const {
    std::vector<DeviceRecord> result;
    for (const auto& [_, device] : devices_) if (device.accountId == accountId) result.push_back(device);
    return result;
}

bool DeviceAuthority::CanRemoteInstall(const std::string& deviceId,
                                       const std::string& accountId,
                                       std::string& error) const {
    const auto device = Find(deviceId);
    if (!device || device->accountId != accountId) {
        error = "remote-install target does not belong to the account";
        return false;
    }
    if (device->authority != AuthoritySource::ZeroService || !device->trusted || device->state != DeviceState::Online) {
        error = "remote-install target is not trusted and online";
        return false;
    }
    return true;
}

} // namespace zero::v5
