#include "v5/ProductionPackagePlatform.h"

#include "ManifestValidator.h"
#include "PackageIntegrityVerifier.h"
#include "PackageManifestParser.h"
#include "PackageSecurity.h"
#include "PlatformDatabase.h"
#include "Utf8Path.h"
#include <filesystem>

namespace zero::v5 {
namespace {

std::filesystem::path packagePath(const std::filesystem::path& root, const std::string& packageId) {
    const auto relative = PathFromUtf8(packageId);
    return relative ? root / *relative : std::filesystem::path{};
}

void removeBestEffort(const std::filesystem::path& path) noexcept {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
}

ContentIdentity identityFrom(const GameManifest& manifest, const PackageTrustResult& trust) {
    ContentIdentity identity;
    identity.contentId = manifest.packageId;
    identity.packageId = manifest.packageId;
    identity.publisherId = trust.publisherId.empty() ? "local" : trust.publisherId;
    identity.contentClass = ContentClass::Experience;
    return identity;
}

bool stageSource(const std::filesystem::path& source,
                 const std::filesystem::path& staging,
                 PackageInventory& sourceInventory,
                 std::wstring& error) {
    if (!PackageSecurity::BuildInventory(source, sourceInventory, error)) return false;
    removeBestEffort(staging);
    std::error_code ec;
    std::filesystem::create_directories(staging.parent_path(), ec);
    if (ec) {
        error = L"ZERO V5 could not create package staging storage.";
        return false;
    }
    std::filesystem::copy(source, staging,
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        removeBestEffort(staging);
        error = L"ZERO V5 could not stage the package.";
        return false;
    }
    PackageInventory stagedInventory;
    if (!PackageSecurity::BuildInventory(staging, stagedInventory, error) ||
        !PackageSecurity::Equivalent(sourceInventory, stagedInventory, error)) {
        removeBestEffort(staging);
        return false;
    }
    if (!PackageSecurity::WriteIntegrityManifest(staging, stagedInventory, error)) {
        removeBestEffort(staging);
        return false;
    }
    return true;
}

bool activate(const std::filesystem::path& staging,
              const std::filesystem::path& destination,
              const std::filesystem::path& rollback,
              bool destinationMustExist,
              std::wstring& error) {
    std::error_code ec;
    const bool exists = std::filesystem::exists(destination, ec) && !ec;
    if (destinationMustExist && !exists) {
        error = L"ZERO V5 repair requires an existing installed package.";
        removeBestEffort(staging);
        return false;
    }

    removeBestEffort(rollback);
    if (exists) {
        std::filesystem::create_directories(rollback.parent_path(), ec);
        if (ec) {
            error = L"ZERO V5 could not create rollback storage.";
            removeBestEffort(staging);
            return false;
        }
        std::filesystem::rename(destination, rollback, ec);
        if (ec) {
            error = L"ZERO V5 could not move the installed package into rollback storage.";
            removeBestEffort(staging);
            return false;
        }
    }

    ec.clear();
    std::filesystem::rename(staging, destination, ec);
    if (ec) {
        if (exists) {
            std::error_code restore;
            std::filesystem::rename(rollback, destination, restore);
            error = restore ? L"ZERO V5 activation and rollback both failed."
                            : L"ZERO V5 activation failed; the previous package was restored.";
        } else {
            error = L"ZERO V5 could not commit the staged package.";
        }
        removeBestEffort(staging);
        return false;
    }

    std::wstring verifyError;
    if (!PackageIntegrityVerifier::Verify(destination, verifyError)) {
        removeBestEffort(destination);
        if (exists) {
            std::error_code restore;
            std::filesystem::rename(rollback, destination, restore);
            error = restore ? L"ZERO V5 integrity verification failed and rollback failed."
                            : verifyError + L" The previous package was restored.";
        } else {
            error = verifyError.empty() ? L"ZERO V5 integrity verification failed after commit." : verifyError;
        }
        return false;
    }

    removeBestEffort(rollback);
    return true;
}

} // namespace

ProductionPackagePlatform::ProductionPackagePlatform(std::filesystem::path libraryRoot,
                                                     const IPublisherTrustProvider& trustProvider,
                                                     std::filesystem::path databasePath)
    : libraryRoot_(std::move(libraryRoot)),
      trustProvider_(trustProvider),
      databasePath_(std::move(databasePath)) {}

PackageOperationResult ProductionPackagePlatform::Execute(const PackageOperationRequest& request) const {
    PackageOperationResult result;
    if (request.sourceRoot.empty() || !std::filesystem::exists(request.sourceRoot)) {
        result.error = L"ZERO V5 package source is missing.";
        return result;
    }

    const auto parsed = PackageManifestParser::ParseFile(request.sourceRoot / L"zero.manifest.json", request.sourceRoot);
    result.finalStage = PackageStage::Validate;
    if (!parsed.valid || !ManifestValidator::IsSafePackageId(parsed.manifest.packageId)) {
        result.error = parsed.error.empty() ? L"ZERO V5 rejected the package manifest." : parsed.error;
        return result;
    }
    if (request.operation == PackageOperation::Repair &&
        (request.expectedPackageId.empty() || request.expectedPackageId != parsed.manifest.packageId)) {
        result.error = L"ZERO V5 repair package identity does not match the installed package.";
        return result;
    }

    result.finalStage = PackageStage::Trust;
    const auto trust = PackageTrustService::Evaluate(request.sourceRoot, request.trustPolicy, trustProvider_);
    if (!trust.launchAllowed) {
        result.error = trust.detail.empty() ? L"ZERO V5 package trust policy rejected the package." : trust.detail;
        return result;
    }

    std::error_code ec;
    std::filesystem::create_directories(libraryRoot_, ec);
    if (ec) {
        result.error = L"ZERO V5 could not access the Library directory.";
        return result;
    }

    const auto destination = packagePath(libraryRoot_, parsed.manifest.packageId);
    const auto staging = packagePath(libraryRoot_ / L".v5-staging", parsed.manifest.packageId);
    const auto rollback = packagePath(libraryRoot_ / L".v5-rollback", parsed.manifest.packageId);
    if (destination.empty() || staging.empty() || rollback.empty()) {
        result.error = L"ZERO V5 could not resolve a safe package path.";
        return result;
    }

    PackageInventory sourceInventory;
    result.finalStage = PackageStage::Stage;
    if (!stageSource(request.sourceRoot, staging, sourceInventory, result.error)) return result;

    const auto staged = PackageManifestParser::ParseFile(staging / L"zero.manifest.json", staging);
    if (!staged.valid || staged.manifest.packageId != parsed.manifest.packageId ||
        staged.manifest.version != parsed.manifest.version) {
        removeBestEffort(staging);
        result.error = L"ZERO V5 rejected the staged package because its manifest identity changed.";
        return result;
    }

    result.finalStage = PackageStage::Commit;
    if (!activate(staging, destination, rollback,
                  request.operation == PackageOperation::Repair, result.error)) return result;

    result.finalStage = PackageStage::Register;
    const auto installed = PackageManifestParser::ParseFile(destination / L"zero.manifest.json", destination);
    if (!installed.valid) {
        result.error = L"ZERO V5 could not register the committed package because its installed manifest is invalid.";
        return result;
    }
    PlatformDatabase database(databasePath_);
    if (!database.Initialize(result.error) || !database.UpsertGame(installed.manifest, result.error)) return result;

    result.success = true;
    result.finalStage = PackageStage::Ready;
    result.installedRoot = destination;
    result.identity = identityFrom(installed.manifest, trust);
    result.version = installed.manifest.version;
    return result;
}

} // namespace zero::v5
