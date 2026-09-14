#pragma once

#include "ContentModel.h"
#include <filesystem>
#include <optional>
#include <string>

namespace zero::v5 {

enum class PackageStage : unsigned char {
    Acquire,
    Validate,
    Trust,
    Stage,
    Commit,
    Register,
    Ready
};

struct PackageCandidate {
    std::filesystem::path source;
    std::filesystem::path stagingRoot;
    ContentIdentity identity;
    std::string version;
};

struct PackageCommit {
    std::filesystem::path installedRoot;
    ContentIdentity identity;
    std::string version;
};

class IPackageValidator {
public:
    virtual ~IPackageValidator() = default;
    virtual bool Validate(const PackageCandidate& candidate, std::string& error) const = 0;
};

class IPackageTrustVerifier {
public:
    virtual ~IPackageTrustVerifier() = default;
    virtual bool Verify(const PackageCandidate& candidate, std::string& error) const = 0;
};

class IPackageInstaller {
public:
    virtual ~IPackageInstaller() = default;
    virtual std::optional<PackageCommit> Install(const PackageCandidate& candidate, std::string& error) = 0;
};

class IPackageRegistry {
public:
    virtual ~IPackageRegistry() = default;
    virtual bool Register(const PackageCommit& commit, std::string& error) = 0;
};

class PackagePipeline {
public:
    PackagePipeline(IPackageValidator& validator,
                    IPackageTrustVerifier& trust,
                    IPackageInstaller& installer,
                    IPackageRegistry& registry)
        : validator_(validator), trust_(trust), installer_(installer), registry_(registry) {}

    std::optional<PackageCommit> Execute(const PackageCandidate& candidate, std::string& error) {
        if (!validator_.Validate(candidate, error)) return std::nullopt;
        if (!trust_.Verify(candidate, error)) return std::nullopt;
        auto commit = installer_.Install(candidate, error);
        if (!commit) return std::nullopt;
        if (!registry_.Register(*commit, error)) return std::nullopt;
        return commit;
    }

private:
    IPackageValidator& validator_;
    IPackageTrustVerifier& trust_;
    IPackageInstaller& installer_;
    IPackageRegistry& registry_;
};

} // namespace zero::v5
