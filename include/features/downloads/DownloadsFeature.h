#pragma once

#include "GameRegistry.h"
#include "v5/LibraryDownloadAuthority.h"
#include <cstdint>
#include <string>
#include <vector>

namespace zero::features::downloads {

struct DownloadJobViewModel {
    std::string jobId;
    std::string packageId;
    std::string version;
    std::string stateLabel;
    std::string failure;
    std::uint64_t completedBytes{0};
    std::uint64_t totalBytes{0};
    unsigned percent{0};
};

struct InstalledPackageViewModel {
    std::string packageId;
    std::string title;
    std::string version;
};

struct DownloadsViewModel {
    std::vector<DownloadJobViewModel> jobs;
    std::vector<InstalledPackageViewModel> installed;
};

class DownloadsController final {
public:
    static DownloadsViewModel Build(const v5::DownloadAuthority& downloads,
                                    const GameRegistry& registry);
};

class DownloadsView final {
public:
    static std::string StateLabel(v5::DownloadState state);
};

} // namespace zero::features::downloads
