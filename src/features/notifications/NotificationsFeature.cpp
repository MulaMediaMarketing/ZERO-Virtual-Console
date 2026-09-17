#include "features/notifications/NotificationsFeature.h"
#include <windows.h>

namespace zero::features::notifications {
namespace {
std::string Narrow(const std::wstring& value) {
    if (value.empty()) return {};

    const int count = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (count <= 0) return {};

    std::string result(static_cast<size_t>(count), '\0');
    const int written = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        count,
        nullptr,
        nullptr);
    if (written != count) return {};
    return result;
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
