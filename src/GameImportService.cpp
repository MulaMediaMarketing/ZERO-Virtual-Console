#include "GameImportService.h"
#include "GameRegistry.h"
#include "ManifestValidator.h"
#include "PackageIntegrityVerifier.h"
#include "PackageSecurity.h"
#include <fstream>
#include <sstream>

namespace zero {
namespace {
std::string readAll(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}
std::string jsonString(const std::string& s, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    auto k = s.find(token); if (k == std::string::npos) return {};
    auto c = s.find(':', k + token.size()); if (c == std::string::npos) return {};
    auto q1 = s.find('"', c + 1); if (q1 == std::string::npos) return {};
    auto q2 = s.find('"', q1 + 1); if (q2 == std::string::npos) return {};
    return s.substr(q1 + 1, q2 - q1 - 1);
}

bool stagePackage(const std::filesystem::path& sourceRoot,
                  const std::filesystem::path& libraryRoot,
                  const std::string& expectedPackageId,
                  std::filesystem::path& staging,
                  std::filesystem::path& destination,
                  std::wstring& error) {
    const auto manifest = sourceRoot / "zero.manifest.json";
    if (!std::filesystem::exists(manifest)) {
        error = L"The selected folder does not contain zero.manifest.json.";
        return false;
    }

    PackageInventory sourceInventory;
    if (!PackageSecurity::BuildInventory(sourceRoot, sourceInventory, error)) return false;

    const auto text = readAll(manifest);
    const auto packageId = jsonString(text, "package_id");
    if (!ManifestValidator::IsSafePackageId(packageId)) {
        error = L"The package_id is missing or invalid.";
        return false;
    }
    if (!expectedPackageId.empty() && packageId != expectedPackageId) {
        error = L"The selected package does not match the installed game being repaired.";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(libraryRoot, ec);
    if (ec) { error = L"ZERO could not access the Library directory."; return false; }

    const auto stagingBase = libraryRoot / L".staging";
    staging = stagingBase / std::filesystem::path(packageId.begin(), packageId.end());
    destination = libraryRoot / std::filesystem::path(packageId.begin(), packageId.end());
    std::filesystem::remove_all(staging, ec); ec.clear();
    std::filesystem::create_directories(staging, ec);
    if (ec) { error = L"ZERO could not create the import staging directory."; return false; }

    std::filesystem::copy(sourceRoot, staging,
        std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        error = L"ZERO could not stage the selected game folder.";
        return false;
    }

    PackageInventory stagedInventory;
    if (!PackageSecurity::BuildInventory(staging, stagedInventory, error) ||
        !PackageSecurity::Equivalent(sourceInventory, stagedInventory, error)) {
        std::filesystem::remove_all(staging, ec);
        return false;
    }

    GameRegistry stagedRegistry(stagingBase);
    stagedRegistry.Refresh();
    const auto& games = stagedRegistry.Games();
    if (games.size() != 1 || games.front().packageId != packageId) {
        std::filesystem::remove_all(staging, ec);
        error = L"The staged game failed ZERO manifest validation.";
        return false;
    }

    if (!PackageSecurity::WriteIntegrityManifest(staging, stagedInventory, error)) {
        std::filesystem::remove_all(staging, ec);
        return false;
    }
    return true;
}
}

GameImportService::GameImportService(std::filesystem::path libraryRoot)
    : libraryRoot_(std::move(libraryRoot)) {}

ImportResult GameImportService::ImportFolder(const std::filesystem::path& sourceRoot) const {
    ImportResult result;
    std::filesystem::path staging;
    std::filesystem::path destination;
    if (!stagePackage(sourceRoot, libraryRoot_, {}, staging, destination, result.error)) return result;

    std::error_code ec;
    if (std::filesystem::exists(destination)) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"This ZERO package is already installed. Use Repair/Re-import from Game Details to replace it safely.";
        return result;
    }

    std::filesystem::rename(staging, destination, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"ZERO could not finalize the imported game.";
        return result;
    }

    if (!PackageIntegrityVerifier::Verify(destination, result.error)) {
        std::filesystem::remove_all(destination, ec);
        return result;
    }

    result.success = true;
    result.installedRoot = destination;
    return result;
}

ImportResult GameImportService::RepairFolder(const std::filesystem::path& sourceRoot,
                                             const std::string& expectedPackageId) const {
    ImportResult result;
    if (!ManifestValidator::IsSafePackageId(expectedPackageId)) {
        result.error = L"ZERO cannot repair a package with an invalid package identity.";
        return result;
    }

    std::filesystem::path staging;
    std::filesystem::path destination;
    if (!stagePackage(sourceRoot, libraryRoot_, expectedPackageId, staging, destination, result.error)) return result;

    std::error_code ec;
    if (!std::filesystem::exists(destination)) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"The installed package is no longer present. Import it again instead of repairing it.";
        return result;
    }

    const auto backupBase = libraryRoot_ / L".repair-backup";
    const auto backup = backupBase / std::filesystem::path(expectedPackageId.begin(), expectedPackageId.end());
    std::filesystem::create_directories(backupBase, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"ZERO could not prepare package repair rollback storage.";
        return result;
    }
    std::filesystem::remove_all(backup, ec); ec.clear();

    std::filesystem::rename(destination, backup, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"ZERO could not move the installed package into repair rollback storage.";
        return result;
    }

    std::filesystem::rename(staging, destination, ec);
    if (ec) {
        std::error_code rollback;
        std::filesystem::rename(backup, destination, rollback);
        std::filesystem::remove_all(staging, rollback);
        result.error = rollback
            ? L"ZERO could not activate the repaired package and automatic rollback also failed. The previous package remains in Library/.repair-backup."
            : L"ZERO could not activate the repaired package. The previous installation was restored.";
        return result;
    }

    std::wstring verifyError;
    if (!PackageIntegrityVerifier::Verify(destination, verifyError)) {
        std::filesystem::remove_all(destination, ec); ec.clear();
        std::filesystem::rename(backup, destination, ec);
        result.error = ec
            ? L"The repaired package failed integrity verification and ZERO could not restore the previous installation automatically."
            : (verifyError.empty() ? L"The repaired package failed integrity verification. The previous installation was restored." : verifyError + L" The previous installation was restored.");
        return result;
    }

    std::filesystem::remove_all(backup, ec);
    result.success = true;
    result.installedRoot = destination;
    return result;
}

} // namespace zero
