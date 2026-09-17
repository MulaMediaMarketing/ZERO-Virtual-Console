#include "features/downloads/DownloadsFeature.h"

namespace zero::features::downloads {

std::string DownloadsView::StateLabel(v5::DownloadState state) {
    switch (state) {
        case v5::DownloadState::Queued: return "Queued";
        case v5::DownloadState::Downloading: return "Downloading";
        case v5::DownloadState::Paused: return "Paused";
        case v5::DownloadState::Verifying: return "Verifying";
        case v5::DownloadState::Staging: return "Installing";
        case v5::DownloadState::Ready: return "Ready";
        case v5::DownloadState::Failed: return "Failed";
        case v5::DownloadState::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

DownloadsViewModel DownloadsController::Build(const v5::DownloadAuthority& downloads,
                                              const GameRegistry& registry) {
    DownloadsViewModel model;
    for (const auto& job : downloads.Queue()) {
        DownloadJobViewModel item;
        item.jobId = job.jobId;
        item.packageId = job.packageId;
        item.version = job.version;
        item.stateLabel = DownloadsView::StateLabel(job.state);
        item.failure = job.failure;
        item.completedBytes = job.completedBytes;
        item.totalBytes = job.totalBytes;
        item.percent = job.totalBytes == 0 ? 0u : static_cast<unsigned>((job.completedBytes * 100ull) / job.totalBytes);
        model.jobs.push_back(std::move(item));
    }

    for (const auto& game : registry.Games()) {
        model.installed.push_back(InstalledPackageViewModel{game.packageId, game.title, game.version});
    }
    return model;
}

} // namespace zero::features::downloads
