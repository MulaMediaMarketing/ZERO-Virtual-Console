#include "v5/CloudDomain.h"
#include <iostream>

using namespace zero::v5;

namespace {
class Transport final : public IZeroCloudTransport {
public:
    std::vector<CloudRegion> regions;
    std::optional<CloudSessionAllocation> allocation;
    bool released{false};
    std::vector<CloudRegion> Regions(const std::string&, const std::string&) override { return regions; }
    std::optional<CloudSessionAllocation> Allocate(const std::string&, const std::string&, const std::string&) override { return allocation; }
    bool Release(const std::string&) override { released = true; return true; }
};

SaveVersion localSave() {
    return SaveVersion{"acct-1", "content-1", "main", "local-v2", std::string(64, 'a'), 1200, AuthoritySource::LocalPackage};
}

SaveVersion cloudSave() {
    return SaveVersion{"acct-1", "content-1", "main", "cloud-v1", std::string(64, 'b'), 1000, AuthoritySource::ZeroService};
}
}

int main() {
    std::string error;
    const auto local = localSave();
    const auto cloud = cloudSave();
    if (CloudSaveCoordinator::Decide(local, cloud, SaveConflictPolicy::RequireChoice, error) != SaveSyncDecision::Conflict) return 10;
    if (CloudSaveCoordinator::Decide(local, cloud, SaveConflictPolicy::PreferLocal, error) != SaveSyncDecision::UploadLocal) return 11;
    if (CloudSaveCoordinator::Decide(std::nullopt, cloud, SaveConflictPolicy::RequireChoice, error) != SaveSyncDecision::DownloadCloud) return 12;

    auto forgedCloud = cloud;
    forgedCloud.authority = AuthoritySource::LocalPackage;
    if (CloudSaveCoordinator::ValidateCloudVersion(forgedCloud, error)) return 13;

    Transport transport;
    transport.regions = {{"us-east", 42, true, AuthoritySource::ZeroService},
                         {"us-west", 18, true, AuthoritySource::ZeroService},
                         {"forged", 1, true, AuthoritySource::LocalPackage}};
    transport.allocation = CloudSessionAllocation{"alloc-1", "acct-1", "content-1", "us-west",
                                                   "stream-token", 2000, AuthoritySource::ZeroService};
    ZeroCloudClient cloudClient(transport);
    const auto region = cloudClient.BestRegion("acct-1", "content-1", error);
    if (!region || region->regionId != "us-west") return 20;
    const auto allocation = cloudClient.Start("acct-1", "content-1", 1000, error);
    if (!allocation || allocation->regionId != "us-west") return 21;
    if (!cloudClient.Stop(*allocation, error) || !transport.released) return 22;

    auto forgedAllocation = *transport.allocation;
    forgedAllocation.authority = AuthoritySource::LocalPackage;
    transport.allocation = forgedAllocation;
    if (cloudClient.Start("acct-1", "content-1", 1000, error)) return 23;

    DisconnectedZeroCloudTransport disconnected;
    ZeroCloudClient offline(disconnected);
    if (offline.Start("acct-1", "content-1", 1000, error)) return 24;

    std::cout << "ZERO V5 Cloud Saves + Zero Cloud client acceptance: PASS\n";
    return 0;
}
