#include "MilestoneAcceptance.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <winsqlite/winsqlite3.h>

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

bool anyFileContains(const std::filesystem::path& root,
                     const wchar_t* extension,
                     const std::string& token) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return false;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (extension && _wcsicmp(it->path().extension().c_str(), extension) != 0) continue;
        if (contains(it->path(), token)) return true;
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

bool queryExists(sqlite3* db, const char* sql, const char* value) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    const bool bound = sqlite3_bind_text(stmt, 1, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
    const bool found = bound && sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

bool canonicalDatabaseReady(const std::filesystem::path& dbPath, const char* packageId) {
    sqlite3* db = nullptr;
    if (sqlite3_open16(dbPath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }
    sqlite3_busy_timeout(db, 3000);
    const bool ok =
        queryExists(db, "SELECT 1 FROM games WHERE package_id=? LIMIT 1;", packageId) &&
        queryExists(db, "SELECT 1 FROM sessions WHERE package_id=? LIMIT 1;", packageId) &&
        queryExists(db, "SELECT 1 FROM game_stats WHERE package_id=? AND launch_count>0 LIMIT 1;", packageId) &&
        queryExists(db, "SELECT 1 FROM resume_activities WHERE package_id=? LIMIT 1;", packageId) &&
        queryExists(db, "SELECT 1 FROM achievements WHERE package_id=? AND achievement_id='m1-reference-connected' LIMIT 1;", packageId);
    sqlite3_close(db);
    return ok;
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
    const auto database = root_ / "Data" / "zero.db";

    const bool firstBootPassed = contains(firstBoot, "completed=1") &&
                                 contains(firstBoot, "controller=1") &&
                                 contains(firstBoot, "display=1") &&
                                 contains(firstBoot, "audio=1");
    checks.push_back({"first_boot_completed", firstBootPassed,
        firstBootPassed ? "First Boot completed with controller, display, and audio confirmation."
                        : "Complete the native First Boot flow before acceptance."});

    const bool imported = std::filesystem::exists(libraryPackage / "zero.manifest.json") &&
                          std::filesystem::exists(libraryPackage / "ZeroReferenceGame.exe") &&
                          contains(libraryPackage / "zero.integrity.sha256", "# ZERO package integrity v1");
    checks.push_back({"reference_package_integrity", imported,
        imported ? "Reference package is installed through the hardened importer with a committed SHA-256 inventory."
                 : "Import the generated reference package through ZERO and verify zero.integrity.sha256 is created."});

    const bool launched = contains(saveRoot / "launch.ok", "reference_game_launched=1") &&
                          contains(saveRoot / "ready.ok", "sdk_ready=1") &&
                          anyFileContains(sessions, L".json", "\"package_id\": \"zero.system.reference\"");
    checks.push_back({"runtime_ready_session", launched,
        launched ? "Reference game launched, SDK reported READY, and a package-specific Runtime session exists."
                 : "Launch the reference package and complete the READY handshake."});

    const bool sdkContract = contains(saveRoot / "sdk_contract.ok", "resume_set=1") &&
                             contains(saveRoot / "sdk_contract.ok", "achievement_set=1") &&
                             contains(achievementFile, "m1-reference-connected");
    checks.push_back({"sdk_contract", sdkContract,
        sdkContract ? "Resume registration and achievement persistence are confirmed through the SDK."
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

    const bool databasePassed = canonicalDatabaseReady(database, packageId);
    checks.push_back({"canonical_sqlite_state", databasePassed,
        databasePassed ? "zero.db contains canonical game, session, stats, Resume, and achievement state."
                       : "Run the reference acceptance journey until all canonical SQLite rows are present."});

    const bool crashTriggered = contains(saveRoot / "intentional_crash.triggered", "exception_code=0xE0000073") &&
                                hasNonEmptyExtension(crashPackage, L".dmp") &&
                                anyFileContains(crashPackage, L".json", "\"minidump_written\": true") &&
                                anyFileContains(crashPackage, L".json", "\"package_id\": \"zero.system.reference\"");
    checks.push_back({"crash_containment", crashTriggered,
        crashTriggered ? "Intentional unhandled exception produced a non-empty minidump and package-specific crash metadata while ZERO remained alive."
                       : "Trigger the reference exception and verify both .dmp and package-specific crash JSON diagnostics are produced."});

    return checks;
}

bool MilestoneAcceptance::AllPassed(const std::vector<AcceptanceCheck>& checks) const {
    if (checks.empty()) return false;
    for (const auto& check : checks) if (!check.passed) return false;
    return true;
}

} // namespace zero
