#include "MilestoneAcceptance.h"
#include <shlobj.h>
#include <filesystem>
#include <iostream>

namespace {
std::filesystem::path zeroRoot() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path root = std::filesystem::path(p) / "ZERO";
        CoTaskMemFree(p);
        return root;
    }
    return std::filesystem::current_path() / "ZeroData";
}
}

int wmain() {
    zero::MilestoneAcceptance acceptance(zeroRoot());
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
