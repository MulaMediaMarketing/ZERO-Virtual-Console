#include "MilestoneAcceptance.h"
#include "PlatformPaths.h"
#include <iostream>

int wmain() {
    zero::MilestoneAcceptance acceptance(zero::PlatformPaths::DataRoot());
    const auto checks = acceptance.Run();
    std::cout << "ZERO Console Experience Milestone 1 Acceptance\n";
    std::cout << "================================================\n";
    for (const auto& check : checks) {
        std::cout << (check.passed ? "[PASS] " : "[FAIL] ") << check.name << " - " << check.detail << "\n";
    }
    const bool passed = acceptance.AllPassed(checks);
    std::cout << "\nResult: " << (passed ? "PASS" : "INCOMPLETE") << "\n";
    return passed ? 0 : 2;
}
