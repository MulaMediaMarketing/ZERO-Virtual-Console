#include "GameImportService.h"
#include "ManifestValidator.h"
#include "PackageIntegrityVerifier.h"
#include "PackageManifestParser.h"
#include "PackageSecurity.h"

namespace zero {
namespace {

bool stagePackage(const std::filesystem::path& sourceRoot,
                  const std::filesystem::path& libraryRoot,
                  const std::string& expectedPackageId,
                  std::filesystem::path& staging,
                  std::filesystem::path& destination,
                  std::string& packageId,
                  std::wstring& error) {
    const auto manifestPath = sourceRoot / L"zero.manifest.json";
    if (!std::filesystem::exists(manifestPath)) {
        error = L"The selected folder does not contain zero.manifest.json.";
        return false;
    }

    auto parsedSource = PackageManifestParser::ParseFile(manifestPath, sourceRoot);
    if (!parsedSource.valid) {
        error = parsedSource.error.empty() ? L"The selected package manifest is invalid." : parsedSource.error;
        return false;
    }
    packageId = parsedSource.manifest.packageId;
    if (!expectedPackageId.empty() && packageId != expectedPackageId) {
        error = L"The selected package does not match the installed game being repaired.";
        return false;
    }

    PackageInventory sourceInventory;
    if (!PackageSecurity::BuildInventory(sourceRoot, sourceInventory, error)) return false;

    std::error_code ec;
    std::filesystem::create_directories(libraryRoot, ec);
    if (ec) { error = L"ZERO could not access the Library directory."; return false; }

    const auto stagingBase = libraryRoot / L".staging";
    staging = stagingBase / std::filesystem::u8path(packageId);
    destination = libraryRoot / std::filesystem::u8path(packageId);
    std::filesystem::remove_all(staging, ec);
    ec.clear();
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

    auto parsedStaged = PackageManifestParser::ParseFile(staging / L"zero.manifest.json", staging);
    if (!parsedStaged.valid || parsedStaged.manifest.packageId != packageId) {
        std::filesystem::remove_all(staging, ec);
        error = parsedStaged.error.empty() ? L"The staged game failed ZERO manifest validation." : parsedStaged.error;
        return false;
    }

    if (!PackageSecurity::WriteIntegrityManifest(staging, stagedInventory, error)) {
        std::filesystem::remove_all(staging, ec);
        return false;
    }
    return true;
}

ImportResult activateReplacement(const std::filesystem::path& libraryRoot,
                                 const std::filesystem::path& staging,
                                 const std::filesystem::path& destination,
                                 const std::string& packageId) {
    ImportResult result;
    std::error_code ec;
    const auto backupBase = libraryRoot / L".repair-backup";
    const auto backup = backupBase / std::filesystem::u8path(packageId);
    std::filesystem::create_directories(backupBase, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"ZERO could not prepare package repair rollback storage.";
        return result;
    }
    std::filesystem::remove_all(backup, ec);
    ec.clear();

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
        std::filesystem::remove_all(destination, ec);
        ec.clear();
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

} // namespace

GameImportService::GameImportService(std::filesystem::path libraryRoot)
    : libraryRoot_(std::move(libraryRoot)) {}

ImportResult GameImportService::ImportFolder(const std::filesystem::path& sourceRoot) const {
    ImportResult result;
    std::filesystem::path staging;
    std::filesystem::path destination;
    std::string packageId;
    if (!stagePackage(sourceRoot, libraryRoot_, {}, staging, destination, packageId, result.error)) return result;

    std::error_code ec;
    if (std::filesystem::exists(destination)) {
        return activateReplacement(libraryRoot_, staging, destination, packageId);
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
    std::string packageId;
    if (!stagePackage(sourceRoot, libraryRoot_, expectedPackageId, staging, destination, packageId, result.error)) return result;

    std::error_code ec;
    if (!std::filesystem::exists(destination)) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"The installed package is no longer present. Import it again instead of repairing it.";
        return result;
    }

    return activateReplacement(libraryRoot_, staging, destination, packageId);
}

} // namespace zero
