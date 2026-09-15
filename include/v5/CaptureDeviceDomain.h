#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace zero::v5 {

enum class CaptureKind : std::uint8_t { Screenshot, VideoClip };
enum class CaptureSyncState : std::uint8_t { LocalOnly, PendingUpload, Synced, Failed };

struct CaptureRecord {
    std::string captureId;
    std::string accountId;
    std::string contentId;
    CaptureKind kind{CaptureKind::Screenshot};
    std::string localPath;
    std::uint64_t createdEpochSeconds{0};
    std::uint64_t durationMilliseconds{0};
    CaptureSyncState syncState{CaptureSyncState::LocalOnly};
    bool favorite{false};
};

class CaptureDomain {
public:
    bool Add(CaptureRecord record, std::string& error);
    bool SetFavorite(const std::string& captureId, bool favorite);
    bool SetSyncState(const std::string& captureId, CaptureSyncState state, std::string& error);
    std::optional<CaptureRecord> Find(const std::string& captureId) const;
    std::vector<CaptureRecord> ForAccount(const std::string& accountId) const;
private:
    std::unordered_map<std::string, CaptureRecord> captures_;
};

enum class DeviceType : std::uint8_t { Pc, Controller, Mobile, ZeroStick, Tv, FutureHardware };
enum class DeviceState : std::uint8_t { Offline, Online, Revoked };

struct DeviceRecord {
    std::string deviceId;
    std::string accountId;
    std::string displayName;
    DeviceType type{DeviceType::Pc};
    DeviceState state{DeviceState::Offline};
    std::string softwareVersion;
    bool trusted{false};
    AuthoritySource authority{AuthoritySource::None};
};

class DeviceAuthority {
public:
    bool Apply(DeviceRecord device, std::string& error);
    std::optional<DeviceRecord> Find(const std::string& deviceId) const;
    std::vector<DeviceRecord> ForAccount(const std::string& accountId) const;
    bool CanRemoteInstall(const std::string& deviceId,
                          const std::string& accountId,
                          std::string& error) const;
private:
    std::unordered_map<std::string, DeviceRecord> devices_;
};

} // namespace zero::v5
