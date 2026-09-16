#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace zero::v5 {

enum class NotificationAuthority : std::uint8_t {
    None,
    LocalSystem,
    ZeroService
};

enum class NotificationKind : std::uint8_t {
    System,
    Download,
    Install,
    Update,
    Achievement,
    Capture,
    FriendRequest,
    Invite,
    CloudSave,
    Store,
    AccountSecurity
};

enum class NotificationAction : std::uint8_t {
    None,
    OpenDownloads,
    OpenAchievements,
    OpenCapture,
    OpenFriends,
    OpenCloudPlay,
    OpenStore,
    OpenSettings
};

struct NotificationRecord {
    std::string id;
    NotificationKind kind{NotificationKind::System};
    std::string title;
    std::string body;
    std::uint64_t createdAtEpochSeconds{0};
    NotificationAuthority authority{NotificationAuthority::None};
    NotificationAction action{NotificationAction::None};
    bool unread{true};
};

class NotificationCenter final {
public:
    bool Upsert(NotificationRecord record, std::string& error);
    bool MarkRead(const std::string& id) noexcept;
    bool Dismiss(const std::string& id) noexcept;
    std::optional<NotificationRecord> Find(const std::string& id) const;
    std::vector<NotificationRecord> Records() const;
    std::size_t UnreadCount() const noexcept;

    static bool ValidateAuthority(const NotificationRecord& record,
                                  std::string& error) noexcept;

private:
    std::unordered_map<std::string, NotificationRecord> records_;
};

} // namespace zero::v5
