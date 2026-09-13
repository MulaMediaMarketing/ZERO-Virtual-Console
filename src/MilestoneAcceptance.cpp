#include "MilestoneAcceptance.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace zero {
namespace {

std::string readAll(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool contains(const std::filesystem::path& path, const std::string& token) {
    const auto text = readAll(path);
    return !text.empty() && text.find(token) != std::string::npos;
}

bool hasRegularFile(const std::filesystem::path& root) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return false;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec)) return true;
    }
    return false;
}

bool hasNonEmptyExtension(const std::filesystem::path& root, const wchar_t* extension) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return false;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (_wcsicmp(it->path().extension().c_str(), extension) != 0) continue;
        const auto bytes = it->file_size(ec);
        if (!ec && bytes > 0) return true;
        ec.clear();
    }
    return false;
}

bool anyJsonContains(const std::filesystem::path& root, const std::string& token) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return false;
    for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || _wcsicmp(it->path().extension().c_str(), L".json") != 0) continue;
        if (contains(it->path(), token)) return true;
    }
    return false;
}

} // namespace

MilestoneAcceptance::MilestoneAcceptance(std::filesystem::path zeroRoot) : root_(std::move(zeroRoot)) {}

std::vector<AcceptanceCheck> MilestoneAcceptance::Run() const {
    constexpr const char* packageId = "zero.system.reference";
    std::vector<AcceptanceCheck> checks;

    const auto firstBoot = root_ / "first_boot.ini";
    const auto libraryPackage = root_ / "Library" / packageId;
    const auto saveRoot = root_ / "Saves" / packageId;
    const auto resumeFile = root_ / "Resume" / (std::string(packageId) + ".json");
    const auto achievementFile = root_ / "Achievements" / (std::string(packageId) + ".jsonl");
    const auto sessions = root_ / "Runtime" / "Sessions";
    const auto crashPackage = root_ / "CrashReports" / packageId;

    const bool firstBootPassed = contains(firstBoot, "completed=1") &&
                                 contains(firstBoot, "controller=1") &&
                                 contains(firstBoot, "display=1") &&
                                 contains(firstBoot, "audio=1");
    checks.push_back({"first_boot_completed", firstBootPassed,
        firstBootPassed ? "First Boot completed with controller, display, and audio confirmation."
                        : "Complete the native First Boot flow before acceptance."});

    const bool imported = std::filesystem::exists(libraryPackage / "zero.manifest.json") &&
                          std::filesystem::exists(libraryPackage / "ZeroReferenceGame.exe");
    checks.push_back({"reference_package_imported", imported,
        imported ? "Dedicated ZERO reference package is installed in the managed Library."
                 : "Import the generated ZERO Reference Experience package."});

    const bool launched = contains(saveRoot / "launch.ok", "reference_game_launched=1") &&
                          contains(saveRoot / "ready.ok", "sdk_ready=1") &&
                          hasRegularFile(sessions);
    checks.push_back({"runtime_ready_session", launched,
        launched ? "Reference game launched, SDK reported READY, and Runtime persisted a session."
                 : "Launch the reference package through ZERO and complete the READY handshake."});

    const bool sdkContract = contains(saveRoot / "sdk_contract.ok", "resume_set=1") &&
                             contains(saveRoot / "sdk_contract.ok", "achievement_set=1") &&
                             contains(achievementFile, "m1-reference-connected");
    checks.push_back({"sdk_achievement_contract", sdkContract,
        sdkContract ? "Resume registration and achievement persistence are confirmed."
                    : "Run the reference game until Resume and achievement calls persist successfully."});

    const bool resumeStored = contains(resumeFile, "m1-reference-checkpoint") &&
                              contains(resumeFile, "checkpoint=m1_reference_validated");
    checks.push_back({"resume_metadata_persisted", resumeStored,
        resumeStored ? "Expected logical Resume metadata is persisted."
                     : "Create the reference Resume activity through ZERO SDK."});

    const bool resumeRoundTrip = contains(saveRoot / "resume_roundtrip.ok", "resume_valid=1") &&
                                 contains(saveRoot / "resume_roundtrip.ok", "checkpoint=m1_reference_validated");
    checks.push_back({"resume_roundtrip", resumeRoundTrip,
        resumeRoundTrip ? "ZERO delivered the persisted Resume payload back into the game."
                        : "Exit to ZERO, choose Resume, and verify the reference checkpoint is restored."});

    const bool crashTriggered = contains(saveRoot / "intentional_crash.triggered", "exception_code=0xE0000073") &&
                                hasNonEmptyExtension(crashPackage, L".dmp") &&
                                anyJsonContains(crashPackage, "\"minidump_written\": true");
    checks.push_back({"crash_containment", crashTriggered,
        crashTriggered ? "Intentional unhandled exception produced a non-empty minidump and crash metadata while ZERO remained the host."
                       : "Trigger the reference exception and verify both .dmp and crash JSON diagnostics are produced."});

    return checks;
}

bool MilestoneAcceptance::AllPassed(const std::vector<AcceptanceCheck>& checks) const {
    if (checks.empty()) return false;
    for (const auto& check : checks) if (!check.passed) return false;
    return true;
}

} // namespace zero
