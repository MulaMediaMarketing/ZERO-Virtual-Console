#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace zero::v5 {

enum class InstallState : std::uint8_t {
    NotInstalled,
    Installing,
    Installed,
    UpdateAvailable,
    RepairRequired,
    Unavailable
};

enum class LibraryAction : std::uint8_t {
    None,
    Install,
    PlayLocal,
    PlayCloud,
    Update,
    Repair
};

struct LibraryEntry {
    ContentIdentity identity;
    EntitlementType entitlementType{EntitlementType::Purchased};
    AuthoritySource entitlementAuthority{AuthoritySource::None};
    InstallState installState{InstallState::NotInstalled};
    std::string installedVersion;
    std::string availableVersion;
    bool cloudReady{false};
    bool favorite{false};
    std::uint64_t lastPlayedEpochSeconds{0};
};

class LibraryAuthority {
public:
    bool Upsert(LibraryEntry entry, std::string& error);
    std::optional<LibraryEntry> Find(const std::string& contentId) const;
    std::vector<LibraryEntry> Entries() const;
    LibraryAction PrimaryAction(const std::string& contentId) const;

private:
    std::unordered_map<std::string, LibraryEntry> entries_;
};

enum class DownloadState : std::uint8_t {
    Queued,
    Downloading,
    Paused,
    Verifying,
    Staging,
    Ready,
    Failed,
    Cancelled
};

struct DownloadJob {
    std::string jobId;
    std::string contentId;
    std::string packageId;
    std::string version;
    std::uint64_t totalBytes{0};
    std::uint64_t completedBytes{0};
    DownloadState state{DownloadState::Queued};
    std::string failure;
};

class DownloadAuthority {
public:
    bool Enqueue(DownloadJob job, std::string& error);
    bool Transition(const std::string& jobId,
                    DownloadState next,
                    std::uint64_t completedBytes,
                    std::string& error);
    std::optional<DownloadJob> Find(const std::string& jobId) const;
    std::vector<DownloadJob> Queue() const;

private:
    std::unordered_map<std::string, DownloadJob> jobs_;
    static bool Allowed(DownloadState from, DownloadState to) noexcept;
};

struct UpdatePlan {
    std::string contentId;
    std::string packageId;
    std::string installedVersion;
    std::string targetVersion;
    AuthoritySource authority{AuthoritySource::None};
    bool mandatory{false};
};

class UpdateAuthority {
public:
    static bool Validate(const LibraryEntry& entry,
                         const UpdatePlan& plan,
                         std::string& error);
};

} // namespace zero::v5
