#include "v5/NotificationDomain.h"
#include <algorithm>

namespace zero::v5 {
namespace {

bool RequiresZeroService(NotificationKind kind) noexcept {
    switch (kind) {
        case NotificationKind::FriendRequest:
        case NotificationKind::Invite:
        case NotificationKind::CloudSave:
        case NotificationKind::Store:
        case NotificationKind::AccountSecurity:
            return true;
        default:
            return false;
    }
}

} // namespace

bool NotificationCenter::ValidateAuthority(const NotificationRecord& record,
                                           std::string& error) noexcept {
    if (record.id.empty()) {
        error = "Notification ID is required.";
        return false;
    }
    if (record.title.empty()) {
        error = "Notification title is required.";
        return false;
    }
    if (record.createdAtEpochSeconds == 0) {
        error = "Notification timestamp is required.";
        return false;
    }
    if (record.authority == NotificationAuthority::None) {
        error = "Notification authority is required.";
        return false;
    }
    if (RequiresZeroService(record.kind) && record.authority != NotificationAuthority::ZeroService) {
        error = "Online notification type requires ZERO service authority.";
        return false;
    }
    error.clear();
    return true;
}

bool NotificationCenter::Upsert(NotificationRecord record, std::string& error) {
    if (!ValidateAuthority(record, error)) return false;
    records_[record.id] = std::move(record);
    return true;
}

bool NotificationCenter::MarkRead(const std::string& id) noexcept {
    const auto it = records_.find(id);
    if (it == records_.end()) return false;
    it->second.unread = false;
    return true;
}

bool NotificationCenter::Dismiss(const std::string& id) noexcept {
    return records_.erase(id) > 0;
}

std::optional<NotificationRecord> NotificationCenter::Find(const std::string& id) const {
    const auto it = records_.find(id);
    if (it == records_.end()) return std::nullopt;
    return it->second;
}

std::vector<NotificationRecord> NotificationCenter::Records() const {
    std::vector<NotificationRecord> out;
    out.reserve(records_.size());
    for (const auto& [_, record] : records_) out.push_back(record);
    std::stable_sort(out.begin(), out.end(), [](const NotificationRecord& a, const NotificationRecord& b) {
        if (a.createdAtEpochSeconds != b.createdAtEpochSeconds)
            return a.createdAtEpochSeconds > b.createdAtEpochSeconds;
        return a.id < b.id;
    });
    return out;
}

std::size_t NotificationCenter::UnreadCount() const noexcept {
    std::size_t count = 0;
    for (const auto& [_, record] : records_) {
        if (record.unread) ++count;
    }
    return count;
}

} // namespace zero::v5
