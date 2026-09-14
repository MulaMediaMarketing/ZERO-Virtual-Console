#include "v5/ProductionPackagePlatform.h"

#include "GameImportService.h"
#include "ManifestValidator.h"
#include "PackageIntegrityVerifier.h"
#include "PackageManifestParser.h"

namespace zero::v5 {

ProductionPackagePlatform::ProductionPackagePlatform(const IPublisherTrustProvider& publisherTrust)
    : publisherTrust_(publisherTrust) {}

PackageOperationResult ProductionPackagePlatform::Execute(const PackageOperationRequest& request) const {
    PackageOperationResult result;

    result.stage = PackageOperationStage::ValidateRequest;
    if (request.sourceRoot.empty() || request.libraryRoot.empty()) {
        result.error = L"ZERO V5 requires both a package source and Library root.";
        return result;
    }
    if (request.operation == PackageOperation::Repair &&
        !ManifestValidator::IsSafePackageId(request.expectedPackageId)) {
        result.error = L"ZERO V5 cannot repair a package with an invalid expected package identity.";
        return result;
    }

    result.stage = PackageOperationStage::ParseManifest;
    const auto parsed = PackageManifestParser::ParseFile(request.sourceRoot / L"zero.manifest.json",
                                                          request.sourceRoot);
    if (!parsed.valid) {
        result.error = parsed.error.empty() ? L"ZERO V5 rejected the package manifest." : parsed.error;
        return result;
    }
    if (request.operation == PackageOperation::Repair &&
        parsed.manifest.packageId != request.expectedPackageId) {
        result.error = L"ZERO V5 rejected repair media for a different package identity.";
        return result;
    }

    result.identity.contentId = parsed.manifest.packageId;
    result.identity.packageId = parsed.manifest.packageId;
    result.identity.contentClass = request.contentClass;
    result.version = parsed.manifest.version;

    result.stage = PackageOperationStage::VerifyTrust;
    result.trust = PackageTrustService::Evaluate(request.sourceRoot,
                                                 request.trustPolicy,
                                                 publisherTrust_);
    if (!result.trust.launchAllowed) {
        result.error = result.trust.detail.empty()
            ? L"ZERO V5 rejected the package because publisher trust could not be established."
            : result.trust.detail;
        return result;
    }
    result.identity.publisherId = result.trust.publisherId.empty()
        ? std::string{"local.package"}
        : result.trust.publisherId;

    result.stage = PackageOperationStage::StageAndCommit;
    const GameImportService importer(request.libraryRoot);
    const auto imported = request.operation == PackageOperation::Repair
        ? importer.RepairFolder(request.sourceRoot, request.expectedPackageId)
        : importer.ImportFolder(request.sourceRoot);
    if (!imported.success) {
        result.error = imported.error.empty() ? L"ZERO V5 could not commit the package transaction." : imported.error;
        return result;
    }
    result.installedRoot = imported.installedRoot;

    result.stage = PackageOperationStage::VerifyInstalledPackage;
    std::wstring integrityError;
    if (!PackageIntegrityVerifier::Verify(result.installedRoot, integrityError)) {
        result.error = integrityError.empty()
            ? L"ZERO V5 rejected the committed package because installed integrity verification failed."
            : integrityError;
        return result;
    }

    const auto installedManifest = PackageManifestParser::ParseFile(
        result.installedRoot / L"zero.manifest.json", result.installedRoot);
    if (!installedManifest.valid ||
        installedManifest.manifest.packageId != result.identity.packageId ||
        installedManifest.manifest.version != result.version) {
        result.error = L"ZERO V5 rejected the committed package because installed identity changed after commit.";
        return result;
    }

    result.stage = PackageOperationStage::Ready;
    result.success = true;
    return result;
}

} // namespace zero::v5
