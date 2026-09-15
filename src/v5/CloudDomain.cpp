#include "v5/CloudDomain.h"
#include <algorithm>

namespace zero::v5 {

bool CloudSaveCoordinator::ValidateCloudVersion(const SaveVersion& cloud, std::string& error) {
    if (cloud.authority != AuthoritySource::ZeroService) {
        error = "cloud save version was not issued by ZERO service";
        return false;
    }
    if (cloud.accountId.empty() || cloud.contentId.empty() || cloud.saveNamespace.empty() ||
        cloud.versionId.empty() || cloud.sha256.size() != 64) {
        error = "cloud save version is missing identity or digest";
        return false;
    }
    return true;
}

SaveSyncDecision CloudSaveCoordinator::Decide(const std::optional<SaveVersion>& local,
                                              const std::optional<SaveVersion>& cloud,
                                              SaveConflictPolicy policy,
                                              std::string& error) {
    if (!local && !cloud) return SaveSyncDecision::NoOp;
    if (cloud && !ValidateCloudVersion(*cloud, error)) return SaveSyncDecision::Conflict;
    if (local && !cloud) return SaveSyncDecision::UploadLocal;
    if (!local && cloud) return SaveSyncDecision::DownloadCloud;

    if (local->accountId != cloud->accountId || local->contentId != cloud->contentId ||
        local->saveNamespace != cloud->saveNamespace) {
        error = "local and cloud save identities do not match";
        return SaveSyncDecision::Conflict;
    }
    if (local->sha256 == cloud->sha256) return SaveSyncDecision::NoOp;
    if (local->versionId == cloud->versionId) {
        error = "same save version identity has divergent content";
        return SaveSyncDecision::Conflict;
    }
    if (policy == SaveConflictPolicy::PreferLocal) return SaveSyncDecision::UploadLocal;
    if (policy == SaveConflictPolicy::PreferCloud) return SaveSyncDecision::DownloadCloud;
    error = "local and cloud saves diverged and require player choice";
    return SaveSyncDecision::Conflict;
}

std::optional<CloudRegion> ZeroCloudClient::BestRegion(const std::string& accountId,
                                                       const std::string& contentId,
                                                       std::string& error) {
    if (accountId.empty() || contentId.empty()) {
        error = "cloud region lookup requires account and content identity";
        return std::nullopt;
    }
    auto regions = transport_.Regions(accountId, contentId);
    regions.erase(std::remove_if(regions.begin(), regions.end(), [](const CloudRegion& region) {
        return region.authority != AuthoritySource::ZeroService || region.regionId.empty() || !region.capacityAvailable;
    }), regions.end());
    if (regions.empty()) {
        error = "no authoritative Zero Cloud region has available capacity";
        return std::nullopt;
    }
    return *std::min_element(regions.begin(), regions.end(), [](const CloudRegion& a, const CloudRegion& b) {
        return a.latencyMs < b.latencyMs;
    });
}

bool ZeroCloudClient::ValidateAllocation(const CloudSessionAllocation& allocation,
                                         const std::string& accountId,
                                         const std::string& contentId,
                                         std::uint64_t nowEpochSeconds,
                                         std::string& error) {
    if (allocation.authority != AuthoritySource::ZeroService) {
        error = "cloud session allocation was not issued by ZERO service";
        return false;
    }
    if (allocation.allocationId.empty() || allocation.accountId != accountId ||
        allocation.contentId != contentId || allocation.regionId.empty() || allocation.streamToken.empty()) {
        error = "cloud session allocation identity is invalid";
        return false;
    }
    if (allocation.expiresAtEpochSeconds <= nowEpochSeconds) {
        error = "cloud session allocation expired";
        return false;
    }
    return true;
}

std::optional<CloudSessionAllocation> ZeroCloudClient::Start(const std::string& accountId,
                                                             const std::string& contentId,
                                                             std::uint64_t nowEpochSeconds,
                                                             std::string& error) {
    const auto region = BestRegion(accountId, contentId, error);
    if (!region) return std::nullopt;
    const auto allocation = transport_.Allocate(accountId, contentId, region->regionId);
    if (!allocation || !ValidateAllocation(*allocation, accountId, contentId, nowEpochSeconds, error)) {
        if (error.empty()) error = "Zero Cloud did not return a valid session allocation";
        return std::nullopt;
    }
    if (allocation->regionId != region->regionId) {
        error = "cloud allocation region differs from the authorized selection";
        return std::nullopt;
    }
    return allocation;
}

bool ZeroCloudClient::Stop(const CloudSessionAllocation& allocation, std::string& error) {
    if (allocation.allocationId.empty()) {
        error = "cloud allocation identity is required";
        return false;
    }
    if (!transport_.Release(allocation.allocationId)) {
        error = "Zero Cloud could not release the allocation";
        return false;
    }
    return true;
}

} // namespace zero::v5
