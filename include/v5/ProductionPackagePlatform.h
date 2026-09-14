#pragma once

#include "PackageTrust.h"
#include "v5/ContentModel.h"
#include <filesystem>
#include <string>

namespace zero::v5 {

enum class PackageOperation : unsigned char {
    Install,
    Repair
};

enum class PackageOperationStage : unsigned char {
    ValidateRequest,
    ParseManifest,
    VerifyTrust,
    StageAndCommit,
    VerifyInstalledPackage,
    Ready
};

struct PackageOperationRequest {
    PackageOperation operation{PackageOperation::Install};
    std::filesystem::path sourceRoot;
    std::filesystem::path libraryRoot;
    std::string expectedPackageId;
    PackageTrustPolicy trustPolicy{PackageTrustPolicy::RequireTrustedPublisher};
    ContentClass contentClass{ContentClass::Premium};
};

struct PackageOperationResult {
    bool success{false};
    PackageOperationStage stage{PackageOperationStage::ValidateRequest};
    std::filesystem::path installedRoot;
    ContentIdentity identity;
    std::string version;
    PackageTrustResult trust;
    std::wstring error;
};

// Production entry point for package installation/repair in V5. All callers go
// through the same parse -> trust -> transactional stage/commit -> installed
// integrity verification flow. The legacy GameImportService is contained as a
// migration implementation detail until its filesystem transaction logic is
// moved behind the V5 package transaction interface.
class ProductionPackagePlatform {
public:
    explicit ProductionPackagePlatform(const IPublisherTrustProvider& publisherTrust);

    PackageOperationResult Execute(const PackageOperationRequest& request) const;

private:
    const IPublisherTrustProvider& publisherTrust_;
};

} // namespace zero::v5
