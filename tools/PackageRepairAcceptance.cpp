#include "GameImportService.h"
#include "PackageIntegrityVerifier.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
std::filesystem::path makeRoot() {
    wchar_t temp[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, temp)) return {};
    return std::filesystem::path(temp) / (L"zero-package-repair-" + std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count()));
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << text;
    f.flush();
    return f.good();
}

std::string readText(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

void check(bool value, const char* name, bool& all) {
    std::cout << (value ? "[PASS] " : "[FAIL] ") << name << "\n";
    all &= value;
}

bool makePackage(const std::filesystem::path& root, const std::string& packageId, const std::string& payload) {
    const std::string manifest =
        "{\n"
        "  \"schema\": 1,\n"
        "  \"minimum_runtime_major\": 4,\n"
        "  \"package_id\": \"" + packageId + "\",\n"
        "  \"title\": \"Repair Acceptance Game\",\n"
        "  \"version\": \"1.0.0\",\n"
        "  \"executable\": \"Game.exe\",\n"
        "  \"zero_resume\": true,\n"
        "  \"zero_achievements\": true,\n"
        "  \"zero_overlay\": true,\n"
        "  \"zero_input\": true\n"
        "}\n";
    return writeText(root / L"zero.manifest.json", manifest) &&
           writeText(root / L"Game.exe", payload) &&
           writeText(root / L"Content" / L"data.bin", "content-v1");
}
}

int wmain() {
    const auto root = makeRoot();
    const auto library = root / L"Library";
    const auto source = root / L"Source";
    const auto wrongSource = root / L"WrongSource";
    const auto saveRoot = root / L"Saves" / L"zero.test.repair";
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) return 2;

    bool all = true;
    check(makePackage(source, "zero.test.repair", "clean-executable-v1"), "clean_source_created", all);
    check(makePackage(wrongSource, "zero.test.other", "other-executable"), "wrong_source_created", all);
    check(writeText(saveRoot / L"slot1.sav", "player-save-sentinel"), "external_save_created", all);

    zero::GameImportService service(library);
    auto imported = service.ImportFolder(source);
    check(imported.success, "initial_import_succeeds", all);
    const auto installed = library / L"zero.test.repair";

    std::wstring verifyError;
    check(zero::PackageIntegrityVerifier::Verify(installed, verifyError), "initial_integrity_valid", all);

    check(writeText(installed / L"Game.exe", "tampered-executable"), "installed_package_corrupted", all);
    verifyError.clear();
    check(!zero::PackageIntegrityVerifier::Verify(installed, verifyError), "corruption_detected", all);

    auto wrongRepair = service.RepairFolder(wrongSource, "zero.test.repair");
    check(!wrongRepair.success, "mismatched_package_repair_rejected", all);
    check(readText(installed / L"Game.exe") == "tampered-executable", "failed_repair_leaves_installation_untouched", all);

    auto repaired = service.RepairFolder(source, "zero.test.repair");
    check(repaired.success, "repair_succeeds", all);
    verifyError.clear();
    check(zero::PackageIntegrityVerifier::Verify(installed, verifyError), "repaired_integrity_valid", all);
    check(readText(installed / L"Game.exe") == "clean-executable-v1", "installed_files_replaced_from_clean_source", all);
    check(readText(saveRoot / L"slot1.sav") == "player-save-sentinel", "external_save_preserved", all);
    check(!std::filesystem::exists(library / L".repair-backup" / L"zero.test.repair"), "repair_backup_cleaned_after_success", all);

    std::filesystem::remove_all(root, ec);
    std::cout << "Result: " << (all ? "PASS" : "FAIL") << "\n";
    return all ? 0 : 2;
}
