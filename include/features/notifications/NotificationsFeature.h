#pragma once

#include "CaptureLibrary.h"
#include "v5/ProductionRuntime.h"
#include <string>
#include <vector>

namespace zero::features::notifications {

struct NotificationItemViewModel {
    std::string title;
    std::string body;
};

struct NotificationsViewModel {
    std::vector<NotificationItemViewModel> items;
};

class NotificationsController final {
public:
    static NotificationsViewModel Build(const std::wstring& localStatus,
                                        RuntimeOutcome runtimeOutcome,
                                        const CaptureLibrary& captures);
};

class NotificationsView final {
public:
    static bool IsEmpty(const NotificationsViewModel& model) noexcept { return model.items.empty(); }
};

} // namespace zero::features::notifications
