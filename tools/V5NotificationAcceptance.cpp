#include "v5/NotificationDomain.h"
#include <iostream>
#include <string_view>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}
}

int main() {
    using namespace zero::v5;
    bool ok = true;
    NotificationCenter center;
    std::string error;

    NotificationRecord local;
    local.id = "local.download.1";
    local.kind = NotificationKind::Download;
    local.title = "Download ready";
    local.body = "A local transfer completed.";
    local.createdAtEpochSeconds = 100;
    local.authority = NotificationAuthority::LocalSystem;
    local.action = NotificationAction::OpenDownloads;
    ok &= expect(center.Upsert(local, error), "local system events must be accepted for local notification kinds");
    ok &= expect(center.UnreadCount() == 1, "accepted notification must start unread");

    NotificationRecord forgedInvite;
    forgedInvite.id = "invite.fake";
    forgedInvite.kind = NotificationKind::Invite;
    forgedInvite.title = "Join game";
    forgedInvite.createdAtEpochSeconds = 101;
    forgedInvite.authority = NotificationAuthority::LocalSystem;
    forgedInvite.action = NotificationAction::OpenFriends;
    ok &= expect(!center.Upsert(forgedInvite, error),
        "local code must not synthesize server-authoritative invites");
    ok &= expect(!center.Find("invite.fake").has_value(),
        "rejected online notification must never enter the notification center");

    NotificationRecord serviceInvite = forgedInvite;
    serviceInvite.id = "invite.real";
    serviceInvite.authority = NotificationAuthority::ZeroService;
    ok &= expect(center.Upsert(serviceInvite, error), "ZERO service-authoritative invite must be accepted");
    ok &= expect(center.UnreadCount() == 2, "unread count must include authoritative online events");
    ok &= expect(center.MarkRead("invite.real"), "mark-read must update an existing record");
    ok &= expect(center.UnreadCount() == 1, "mark-read must reduce unread count");

    auto records = center.Records();
    ok &= expect(records.size() == 2, "notification listing must contain only accepted records");
    ok &= expect(records.front().id == "invite.real", "notifications must sort newest first");
    ok &= expect(center.Dismiss("local.download.1"), "dismiss must remove a real record");
    ok &= expect(center.Records().size() == 1, "dismissed record must not remain visible");

    if (!ok) return 1;
    std::cout << "ZERO V5 notification authority acceptance: PASS\n";
    return 0;
}
