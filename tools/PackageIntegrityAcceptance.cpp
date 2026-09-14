#include "PackageIntegrityVerifier.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void printCheck(bool passed, const char* name, const std::wstring& detail = {}) {
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << name;
    if (!detail.empty()) std::wcout << L" - " << detail;
    std::cout << "\n";
}

std::filesystem::path tempRoot() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD count = GetTempPathW(MAX_PATH, buffer);
    if (!count || count >= MAX_PATH) return {};
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(buffer) / (L"zero-integrity-acceptance-" + std::to_wstring(tick));
}

bool clonePackage(const std::filesystem::path& source, const std::filesystem::path& destination, std::wstring& error) {
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    if (ec) { error = L"Could not create the temporary package root."; return false; }
    std::filesystem::copy(source, destination,
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::overwrite_existing,
                          ec);
    if (ec) { error = L"Could not copy the reference package for integrity acceptance."; return false; }
    return true;
}

std::filesystem::path firstPayloadFile(const std::filesystem::path& root) {
    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        if (_wcsicmp(it->path().filename().c_str(), L"zero.integrity.sha256") == 0) continue;
        return it->path();
    }
    return {};
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::wcerr << L"Usage: ZeroPackageIntegrityAcceptance.exe <package-root>\n";
        return 2;
    }

    const std::filesystem::path source = argv[1];
    bool allPassed = true;
    std::wstring error;

    const bool cleanPassed = zero::PackageIntegrityVerifier::Verify(source, error);
    printCheck(cleanPassed, "clean_package_verifies", error);
    allPassed &= cleanPassed;

    const auto root = tempRoot();
    const auto tampered = root / L"tampered";
    error.clear();
    bool copied = clonePackage(source, tampered, error);
    printCheck(copied, "tamper_package_cloned", error);
    allPassed &= copied;
    if (copied) {
        const auto payload = firstPayloadFile(tampered);
        bool changed = !payload.empty();
        if (changed) {
            std::ofstream f(payload, std::ios::binary | std::ios::app);
            changed = static_cast<bool>(f);
            if (changed) {
                f << "ZERO_INTEGRITY_TAMPER";
                f.flush();
                changed = f.good();
            }
        }
        printCheck(changed, "tamper_payload_changed");
        allPassed &= changed;

        error.clear();
        const bool rejected = changed && !zero::PackageIntegrityVerifier::Verify(tampered, error) &&
                              error.find(L"changed after import") != std::wstring::npos;
        printCheck(rejected, "tampered_package_blocked", error);
        allPassed &= rejected;
    }

    const auto missingManifest = root / L"missing-manifest";
    error.clear();
    copied = clonePackage(source, missingManifest, error);
    printCheck(copied, "missing_manifest_package_cloned", error);
    allPassed &= copied;
    if (copied) {
        std::error_code ec;
        const bool removed = std::filesystem::remove(missingManifest / L"zero.integrity.sha256", ec) && !ec;
        printCheck(removed, "integrity_manifest_removed");
        allPassed &= removed;

        error.clear();
        const bool rejected = removed && !zero::PackageIntegrityVerifier::Verify(missingManifest, error) &&
                              error.find(L"manifest is missing") != std::wstring::npos;
        printCheck(rejected, "missing_integrity_manifest_blocks_launch", error);
        allPassed &= rejected;
    }

    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);
    std::cout << "Result: " << (allPassed ? "PASS" : "FAIL") << "\n";
    return allPassed ? 0 : 2;
}
