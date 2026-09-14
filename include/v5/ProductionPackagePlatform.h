#pragma once

#include "PackagePipeline.h"
#include "PackageTrust.h"
#include <filesystem>
#include <optional>
#include <string>

namespace zero::v5 {

enum class PackageOperation : unsigned char {
    Import,
    Repair
};

struct PackageOperationRequest {
    PackageOperation operation{PackageOperation::Import};
    std::filesystem::path sourceRoot;
    std::string expectedPackageId;
    PackageTrustPolicy trustPolicy{PackageTrustPolicy::AllowLocalUnsigned};
};

struct PackageOperationResult {
    bool success{false};
    PackageStage finalStage{PackageStage::Acquire};
    std::filesystem::path installedRoot;
    ContentIdentity identity;
    std::string version;
    std::wstring error;
};

class ProductionPackagePlatform final {
public:
    ProductionPackagePlatform(std::filesystem::path libraryRoot,
                              const IPublisherTrustProvider& trustProvider,
                              std::filesystem::path databasePath = {});

    PackageOperationResult Execute(const PackageOperationRequest& request) const;
    const std::filesystem::path& LibraryRoot() const noexcept { return libraryRoot_; }

private:
    std::filesystem::path libraryRoot_;
    const IPublisherTrustProvider& trustProvider_;
    std::filesystem::path databasePath_;
};

} // namespace zero::v5
