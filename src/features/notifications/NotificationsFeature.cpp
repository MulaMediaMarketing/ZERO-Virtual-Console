#include "features/notifications/NotificationsFeature.h"

namespace zero::features::notifications {
namespace {
std::string Narrow(const std::wstring& value) {
    return std::string(value.begin(), value.end());
}
}

NotificationsViewModel NotificationsController::Build(const std::wstring& localStatus,
                                                       RuntimeOutcome runtimeOutcome,
                                                       const CaptureLibrary& captures) {
    NotificationsViewModel model;
    if (!localStatus.empty()) {
        model.items.push_back({"ZERO Player", Narrow(localStatus)});
    }

    if (runtimeOutcome == RuntimeOutcome::Crash || runtimeOutcome == RuntimeOutcome::LaunchFailure ||
        runtimeOutcome == RuntimeOutcome::HandshakeFailure || runtimeOutcome == RuntimeOutcome::ReadyTimeout) {
        model.items.push_back({
            "Game session needs attention",
            "ZERO recorded a runtime failure. Open the affected game or Settings diagnostics for recovery details."
        });
    }

    if (!captures.Items().empty()) {
        model.items.push_back({"Latest local capture", captures.Items().front().path.filename().string()});
    }
    return model;
}

} // namespace zero::features::notifications
