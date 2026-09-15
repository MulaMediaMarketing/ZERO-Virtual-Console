#include "v5/LibraryDownloadAuthority.h"
#include <iostream>

using namespace zero::v5;

int main() {
    LibraryAuthority library;
    std::string error;

    LibraryEntry local;
    local.identity = {"content.local", "pkg.local", "publisher.zero", ContentClass::Premium};
    local.entitlementType = EntitlementType::Purchased;
    local.entitlementAuthority = AuthoritySource::ZeroService;
    local.installState = InstallState::Installed;
    local.installedVersion = "1.0.0";
    local.availableVersion = "1.0.0";
    local.favorite = true;
    if (!library.Upsert(local, error)) return 10;
    if (library.PrimaryAction(local.identity.contentId) != LibraryAction::PlayLocal) return 11;

    LibraryEntry cloud;
    cloud.identity = {"content.cloud", "pkg.cloud", "publisher.zero", ContentClass::FreeToPlay};
    cloud.entitlementType = EntitlementType::Free;
    cloud.entitlementAuthority = AuthoritySource::ZeroService;
    cloud.installState = InstallState::NotInstalled;
    cloud.cloudReady = true;
    if (!library.Upsert(cloud, error)) return 12;
    if (library.PrimaryAction(cloud.identity.contentId) != LibraryAction::PlayCloud) return 13;

    LibraryEntry update = local;
    update.installState = InstallState::UpdateAvailable;
    update.availableVersion = "1.1.0";
    if (!library.Upsert(update, error)) return 14;
    if (library.PrimaryAction(update.identity.contentId) != LibraryAction::Update) return 15;

    DownloadAuthority downloads;
    DownloadJob job{"job-1", update.identity.contentId, update.identity.packageId,
                    "1.1.0", 1000, 0, DownloadState::Queued, {}};
    if (!downloads.Enqueue(job, error)) return 20;
    if (!downloads.Transition("job-1", DownloadState::Downloading, 100, error)) return 21;
    if (!downloads.Transition("job-1", DownloadState::Paused, 250, error)) return 22;
    if (!downloads.Transition("job-1", DownloadState::Downloading, 250, error)) return 23;
    if (downloads.Transition("job-1", DownloadState::Verifying, 900, error)) return 24;
    if (!downloads.Transition("job-1", DownloadState::Verifying, 1000, error)) return 25;
    if (!downloads.Transition("job-1", DownloadState::Staging, 1000, error)) return 26;
    if (!downloads.Transition("job-1", DownloadState::Ready, 1000, error)) return 27;

    UpdatePlan plan{update.identity.contentId, update.identity.packageId,
                    "1.0.0", "1.1.0", AuthoritySource::ZeroService, false};
    if (!UpdateAuthority::Validate(update, plan, error)) return 30;

    auto forged = plan;
    forged.authority = AuthoritySource::LocalPackage;
    if (UpdateAuthority::Validate(update, forged, error)) return 31;

    auto wrongPackage = plan;
    wrongPackage.packageId = "pkg.other";
    if (UpdateAuthority::Validate(update, wrongPackage, error)) return 32;

    LibraryEntry invalid = cloud;
    invalid.entitlementAuthority = AuthoritySource::None;
    if (library.Upsert(invalid, error)) return 33;

    std::cout << "ZERO V5 Library + Downloads + Update authority acceptance: PASS\n";
    return 0;
}
