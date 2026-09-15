#include "v5/ProductionRuntime.h"
#include <iostream>

int main() {
    zero::ProductionRuntime runtime;

    if (runtime.State() != zero::RuntimeState::Idle) return 10;
    if (runtime.Outcome() != zero::RuntimeOutcome::None) return 11;
    if (runtime.IsActive()) return 12;
    if (runtime.Ready()) return 13;
    if (runtime.PlaytimeSeconds() != 0) return 14;

    // No package is active, so a managed SDK-facing achievement operation must
    // fail closed rather than creating local authority implicitly.
    std::wstring error;
    if (runtime.UnlockAchievement("cutover.test", "Cutover Test", error)) return 20;
    if (error.empty()) return 21;

    std::cout << "ZERO V5 final production runtime acceptance: PASS\n";
    return 0;
}
