#pragma once

#include "InputCore.h"
#include "ProductionUxContract.h"
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
    explicit ShellModule(v5::ProductionShellIntegration& navigation) noexcept;

    ShellIntent RouteGlobalInput(const InputSnapshot& input,
                                 bool runtimeActive,
                                 bool overlayVisible) const noexcept;

    bool MoveTopLevel(int direction, ProductionUxPage& page, std::string& error);
    bool Back(ProductionUxPage& page, std::string& error);


private:
    v5::ProductionShellIntegration& navigation_;
};

} // namespace zero::shell
