#include "MilestoneAcceptance.h"
#include <filesystem>

namespace zero {

MilestoneAcceptance::MilestoneAcceptance(std::filesystem::path zeroRoot) : root_(std::move(zeroRoot)) {}

std::vector<AcceptanceCheck> MilestoneAcceptance::Run() const {
    std::vector<AcceptanceCheck> checks;
    const auto library = root_ / "Library";
    const auto saves = root_ / "Saves";
    const auto resume = root_ / "Resume";
    const auto runtime = root_ / "Runtime";
    const auto crash = root_ / "CrashReports";
    const auto firstBoot = root_ / "first_boot.ini";

    checks.push_back({"first_boot_state", std::filesystem::exists(firstBoot),
        std::filesystem::exists(firstBoot) ? "First-boot state exists." : "Complete first boot once."});
    checks.push_back({"library_root", std::filesystem::exists(library),
        std::filesystem::exists(library) ? "Library root exists." : "Import a compatible game."});
    checks.push_back({"save_root", std::filesystem::exists(saves),
        std::filesystem::exists(saves) ? "Managed save root exists." : "Launch a game and write a save."});
    checks.push_back({"resume_root", std::filesystem::exists(resume),
        std::filesystem::exists(resume) ? "Resume metadata exists." : "Set a Resume activity in a game."});
    checks.push_back({"runtime_sessions", std::filesystem::exists(runtime / "Sessions"),
        std::filesystem::exists(runtime / "Sessions") ? "Runtime session records exist." : "Launch a game through ZERO."});
    checks.push_back({"crash_diagnostics", std::filesystem::exists(crash),
        std::filesystem::exists(crash) ? "Crash diagnostics root exists." : "Run the intentional crash acceptance test."});
    return checks;
}

bool MilestoneAcceptance::AllPassed(const std::vector<AcceptanceCheck>& checks) const {
    if (checks.empty()) return false;
    for (const auto& check : checks) if (!check.passed) return false;
    return true;
}

} // namespace zero
