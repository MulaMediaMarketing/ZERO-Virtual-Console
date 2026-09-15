#include "v5/LibraryDownloadAuthority.h"
#include <algorithm>

namespace zero::v5 {

bool LibraryAuthority::Upsert(LibraryEntry entry, std::string& error) {
    if (entry.identity.contentId.empty() || entry.identity.packageId.empty()) {
        error = "library entry requires content and package identity";
        return false;
    }
    if (entry.entitlementAuthority == AuthoritySource::None) {
        error = "library entry requires an entitlement authority";
        return false;
    }
    if ((entry.installState == InstallState::Installed ||
         entry.installState == InstallState::UpdateAvailable ||
         entry.installState == InstallState::RepairRequired) &&
        entry.installedVersion.empty()) {
        error = "installed library state requires an installed version";
        return false;
    }
    entries_[entry.identity.contentId] = std::move(entry);
    return true;
}

std::optional<LibraryEntry> LibraryAuthority::Find(const std::string& contentId) const {
    const auto it = entries_.find(contentId);
    return it == entries_.end() ? std::nullopt : std::optional<LibraryEntry>{it->second};
}

std::vector<LibraryEntry> LibraryAuthority::Entries() const {
    std::vector<LibraryEntry> entries;
    entries.reserve(entries_.size());
    for (const auto& [_, entry] : entries_) entries.push_back(entry);
    std::stable_sort(entries.begin(), entries.end(), [](const LibraryEntry& a, const LibraryEntry& b) {
        if (a.favorite != b.favorite) return a.favorite > b.favorite;
        if (a.lastPlayedEpochSeconds != b.lastPlayedEpochSeconds)
            return a.lastPlayedEpochSeconds > b.lastPlayedEpochSeconds;
        return a.identity.contentId < b.identity.contentId;
    });
    return entries;
}

LibraryAction LibraryAuthority::PrimaryAction(const std::string& contentId) const {
    const auto entry = Find(contentId);
    if (!entry) return LibraryAction::None;
    switch (entry->installState) {
        case InstallState::NotInstalled:
            return entry->cloudReady ? LibraryAction::PlayCloud : LibraryAction::Install;
        case InstallState::Installing:
        case InstallState::Unavailable:
            return LibraryAction::None;
        case InstallState::Installed:
            return LibraryAction::PlayLocal;
        case InstallState::UpdateAvailable:
            return LibraryAction::Update;
        case InstallState::RepairRequired:
            return LibraryAction::Repair;
    }
    return LibraryAction::None;
}

bool DownloadAuthority::Allowed(DownloadState from, DownloadState to) noexcept {
    if (from == to) return true;
    switch (from) {
        case DownloadState::Queued:
            return to == DownloadState::Downloading || to == DownloadState::Cancelled || to == DownloadState::Failed;
        case DownloadState::Downloading:
            return to == DownloadState::Paused || to == DownloadState::Verifying ||
                   to == DownloadState::Cancelled || to == DownloadState::Failed;
        case DownloadState::Paused:
            return to == DownloadState::Downloading || to == DownloadState::Cancelled || to == DownloadState::Failed;
        case DownloadState::Verifying:
            return to == DownloadState::Staging || to == DownloadState::Failed;
        case DownloadState::Staging:
            return to == DownloadState::Ready || to == DownloadState::Failed;
        case DownloadState::Ready:
        case DownloadState::Cancelled:
            return false;
        case DownloadState::Failed:
            return to == DownloadState::Queued;
    }
    return false;
}

bool DownloadAuthority::Enqueue(DownloadJob job, std::string& error) {
    if (job.jobId.empty() || job.contentId.empty() || job.packageId.empty() ||
        job.version.empty() || job.totalBytes == 0) {
        error = "download job is missing required identity or size";
        return false;
    }
    if (jobs_.contains(job.jobId)) {
        error = "download job identity already exists";
        return false;
    }
    job.completedBytes = 0;
    job.state = DownloadState::Queued;
    job.failure.clear();
    jobs_.emplace(job.jobId, std::move(job));
    return true;
}

bool DownloadAuthority::Transition(const std::string& jobId,
                                   DownloadState next,
                                   std::uint64_t completedBytes,
                                   std::string& error) {
    const auto it = jobs_.find(jobId);
    if (it == jobs_.end()) {
        error = "download job does not exist";
        return false;
    }
    auto& job = it->second;
    if (!Allowed(job.state, next)) {
        error = "download state transition is not allowed";
        return false;
    }
    if (completedBytes < job.completedBytes || completedBytes > job.totalBytes) {
        error = "download byte progress is invalid";
        return false;
    }
    if (next == DownloadState::Verifying && completedBytes != job.totalBytes) {
        error = "download cannot verify before all bytes are present";
        return false;
    }
    if (next == DownloadState::Ready && completedBytes != job.totalBytes) {
        error = "download cannot become ready before completion";
        return false;
    }
    job.completedBytes = completedBytes;
    job.state = next;
    if (next != DownloadState::Failed) job.failure.clear();
    return true;
}

std::optional<DownloadJob> DownloadAuthority::Find(const std::string& jobId) const {
    const auto it = jobs_.find(jobId);
    return it == jobs_.end() ? std::nullopt : std::optional<DownloadJob>{it->second};
}

std::vector<DownloadJob> DownloadAuthority::Queue() const {
    std::vector<DownloadJob> jobs;
    jobs.reserve(jobs_.size());
    for (const auto& [_, job] : jobs_) jobs.push_back(job);
    std::stable_sort(jobs.begin(), jobs.end(), [](const DownloadJob& a, const DownloadJob& b) {
        return a.jobId < b.jobId;
    });
    return jobs;
}

bool UpdateAuthority::Validate(const LibraryEntry& entry,
                               const UpdatePlan& plan,
                               std::string& error) {
    if (plan.contentId != entry.identity.contentId ||
        plan.packageId != entry.identity.packageId) {
        error = "update plan identity does not match library entry";
        return false;
    }
    if (entry.entitlementAuthority == AuthoritySource::None ||
        plan.authority == AuthoritySource::None) {
        error = "update requires authoritative library and update metadata";
        return false;
    }
    if (entry.identity.contentClass != ContentClass::Community &&
        plan.authority != AuthoritySource::ZeroService) {
        error = "managed ZERO content update must be authorized by ZERO service";
        return false;
    }
    if (entry.installState != InstallState::UpdateAvailable &&
        entry.installState != InstallState::RepairRequired) {
        error = "library entry is not in an update or repair state";
        return false;
    }
    if (plan.installedVersion.empty() || plan.targetVersion.empty() ||
        plan.installedVersion != entry.installedVersion ||
        plan.targetVersion == plan.installedVersion) {
        error = "update version transition is invalid";
        return false;
    }
    return true;
}

} // namespace zero::v5
