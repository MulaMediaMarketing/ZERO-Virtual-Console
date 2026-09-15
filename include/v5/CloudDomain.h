#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace zero::v5 {

enum class SaveConflictPolicy : std::uint8_t { RequireChoice, PreferLocal, PreferCloud };

enum class SaveSyncDecision : std::uint8_t { NoOp, UploadLocal, DownloadCloud, Conflict };

struct SaveVersion {
    std::string accountId;
    std::string contentId;
    std::string saveNamespace;
    std::string versionId;
    std::string sha256;
    std::uint64_t modifiedEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

class CloudSaveCoordinator {
public:
    static bool ValidateCloudVersion(const SaveVersion& cloud, std::string& error);
    static SaveSyncDecision Decide(const std::optional<SaveVersion>& local,
                                   const std::optional<SaveVersion>& cloud,
                                   SaveConflictPolicy policy,
                                   std::string& error);
};

enum class CloudClientState : std::uint8_t { Disconnected, Ready, Error };

struct CloudRegion {
    std::string regionId;
    std::uint32_t latencyMs{0};
    bool capacityAvailable{false};
    AuthoritySource authority{AuthoritySource::None};
};

struct CloudSessionAllocation {
    std::string allocationId;
    std::string accountId;
    std::string contentId;
    std::string regionId;
    std::string streamToken;
    std::uint64_t expiresAtEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

class IZeroCloudTransport {
public:
    virtual ~IZeroCloudTransport() = default;
    virtual std::vector<CloudRegion> Regions(const std::string& accountId,
                                             const std::string& contentId) = 0;
    virtual std::optional<CloudSessionAllocation> Allocate(const std::string& accountId,
                                                            const std::string& contentId,
                                                            const std::string& regionId) = 0;
    virtual bool Release(const std::string& allocationId) = 0;
};

class DisconnectedZeroCloudTransport final : public IZeroCloudTransport {
public:
    std::vector<CloudRegion> Regions(const std::string&, const std::string&) override { return {}; }
    std::optional<CloudSessionAllocation> Allocate(const std::string&, const std::string&, const std::string&) override { return std::nullopt; }
    bool Release(const std::string&) override { return false; }
};

class ZeroCloudClient {
public:
    explicit ZeroCloudClient(IZeroCloudTransport& transport) : transport_(transport) {}

    std::optional<CloudRegion> BestRegion(const std::string& accountId,
                                          const std::string& contentId,
                                          std::string& error);
    std::optional<CloudSessionAllocation> Start(const std::string& accountId,
                                                const std::string& contentId,
                                                std::uint64_t nowEpochSeconds,
                                                std::string& error);
    bool Stop(const CloudSessionAllocation& allocation, std::string& error);

    static bool ValidateAllocation(const CloudSessionAllocation& allocation,
                                   const std::string& accountId,
                                   const std::string& contentId,
                                   std::uint64_t nowEpochSeconds,
                                   std::string& error);

private:
    IZeroCloudTransport& transport_;
};

} // namespace zero::v5
