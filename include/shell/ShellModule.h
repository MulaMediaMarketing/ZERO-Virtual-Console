#pragma once

#include "CaptureLibrary.h"
#include "FriendsProvider.h"
#include "GameRegistry.h"
#include "InputCore.h"
#include "ProductionUxContract.h"
#include "StoreProvider.h"
#include "v5/ProductionShellIntegration.h"
#include <optional>
#include <string>

namespace zero::shell {

enum class ShellIntentType {
    None,
    MoveTopLevel,
    Back,
    ToggleOverlay,
    Refresh
};

struct ShellIntent {
    ShellIntentType type{ShellIntentType::None};
    int direction{0};
};

class ShellModule final {
public:
    ShellModule(v5::ProductionShellIntegration& navigation,
                GameRegistry& registry,
                CaptureLibrary& captures,
                IFriendsProvider& friends,
                IStoreProvider& store) noexcept;

    ShellIntent RouteGlobalInput(const InputSnapshot& input,
                                 bool runtimeActive,
                                 bool overlayVisible) const noexcept;

    bool MoveTopLevel(int direction, ProductionUxPage& page, std::string& error);
    bool Back(ProductionUxPage& page, std::string& error);

    void RefreshAll();
    void RefreshForPage(ProductionUxPage page);

private:
    v5::ProductionShellIntegration& navigation_;
    GameRegistry& registry_;
    CaptureLibrary& captures_;
    IFriendsProvider& friends_;
    IStoreProvider& store_;
};

} // namespace zero::shell
