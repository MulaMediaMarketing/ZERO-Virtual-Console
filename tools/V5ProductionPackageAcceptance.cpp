#include "PackageIntegrityVerifier.h"
#include "PackageManifestParser.h"
#include "PackageTrust.h"
#include "v5/ProductionPackagePlatform.h"
#include "v5/ProductionPlatformKernel.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <windows.h>

namespace {

struct TempTree {
    std::filesystem::path root;
    explicit TempTree(const wchar_t* name) {
        root = std::filesystem::temp_directory_path() /
               (std::wstring(L"zero-v5-") + name + L"-" + std::to_wstring(GetCurrentProcessId()));
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
        std::filesystem::create_directories(root, ec);
    }
    ~TempTree() {
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }
};

struct BoundManagedAuthority final : zero::v5::IManagedLaunchAuthorityClient {
    bool misbind{false};
    std::optional<zero::v5::LaunchDescriptor> Authorize(const std::string& accountId,
                                                         const std::string& contentId,
                                                         std::wstring&) const override {
        zero::v5::LaunchDescriptor descriptor;
        descriptor.packageId = "zero.managed.game";
        descriptor.version = "2.0.0";
        descriptor.runtimeType = zero::v5::RuntimeType::Streaming;
        descriptor.entitlementToken = "entitlement-token";
        descriptor.sessionToken = "session-token";
        descriptor.authorityKind = zero::v5::LaunchAuthorityKind::ServerManaged;
        descriptor.contentId = contentId;
        descriptor.accountId = misbind ? "wrong-account" : accountId;
        return descriptor;
    }
};

bool writeText(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
    return static_cast<bool>(out);
}

bool buildPackage(const std::filesystem::path& root,
                  const std::string& packageId,
                  const std::string& version,
                  const std::string& payload) {
    const std::string manifest =
        "{\n"
        "  \"schema\": 1,\n"
        "  \"minimum_runtime_major\": 4,\n"
        "  \"package_id\": \"" + packageId + "\",\n"
        "  \"title\": \"V5 Acceptance Game\",\n"
        "  \"version\": \"" + version + "\",\n"
        "  \"executable\": \"game.exe\",\n"
        "  \"zero_resume\": true,\n"
        "  \"zero_achievements\": true,\n"
        "  \"zero_overlay\": true,\n"
        "  \"zero_input\": true\n"
        "}\n";
    return writeText(root / L"zero.manifest.json", manifest) &&
           writeText(root / L"game.exe", payload);
}

} // namespace

int main() {
    using namespace zero;
    using namespace zero::v5;

    TempTree temp(L"package-platform");
    const auto sourceV1 = temp.root / L"source-v1";
    const auto sourceV2 = temp.root / L"source-v2";
    const auto wrongSource = temp.root / L"source-wrong";
    const auto library = temp.root / L"library";
    const auto database = temp.root / L"state" / L"zero.db";

    if (!buildPackage(sourceV1, "zero.acceptance.game", "1.0.0", "version-one")) return 10;
    if (!buildPackage(sourceV2, "zero.acceptance.game", "1.1.0", "version-two")) return 11;
    if (!buildPackage(wrongSource, "zero.other.game", "1.1.0", "wrong-game")) return 12;

    DisconnectedPublisherTrustProvider trust;
    ProductionPackagePlatform packages(library, trust, database);
    DisconnectedManagedLaunchAuthorityClient disconnectedManaged;
    ProductionPlatformKernel kernel(library, trust, disconnectedManaged);

    PackageOperationRequest import;
    import.operation = PackageOperation::Import;
    import.sourceRoot = sourceV1;
    import.trustPolicy = PackageTrustPolicy::AllowLocalUnsigned;
    const auto imported = packages.Execute(import);
    if (!imported.success || imported.finalStage != PackageStage::Ready) return 20;
    if (imported.identity.packageId != "zero.acceptance.game" || imported.version != "1.0.0") return 21;

    std::wstring integrityError;
    if (!PackageIntegrityVerifier::Verify(imported.installedRoot, integrityError)) return 22;
    const auto parsedV1 = PackageManifestParser::ParseFile(imported.installedRoot / L"zero.manifest.json",
                                                           imported.installedRoot);
    if (!parsedV1.valid || parsedV1.manifest.version != "1.0.0") return 23;

    ProductionLaunchRequest localRequest;
    localRequest.mode = LaunchRequestMode::LocalInstalled;
    localRequest.contentId = "zero.acceptance.game";
    const auto localLaunch = kernel.RequestLaunch(localRequest);
    if (!localLaunch.authorized) return 24;
    if (localLaunch.descriptor.authorityKind != LaunchAuthorityKind::LocalPackage) return 25;
    if (!localLaunch.descriptor.entitlementToken.empty()) return 26;

    ProductionLaunchRequest managedRequest;
    managedRequest.mode = LaunchRequestMode::Managed;
    managedRequest.contentId = "zero.managed.content";
    managedRequest.accountId = "acct-1";
    if (kernel.RequestLaunch(managedRequest).authorized) return 27;

    BoundManagedAuthority boundManaged;
    ProductionPlatformKernel managedKernel(library, trust, boundManaged);
    const auto managedLaunch = managedKernel.RequestLaunch(managedRequest);
    if (!managedLaunch.authorized || managedLaunch.descriptor.accountId != "acct-1") return 28;
    boundManaged.misbind = true;
    if (managedKernel.RequestLaunch(managedRequest).authorized) return 29;

    if (!writeText(imported.installedRoot / L"game.exe", "tampered")) return 30;
    if (kernel.RequestLaunch(localRequest).authorized) return 31;

    PackageOperationRequest mismatch;
    mismatch.operation = PackageOperation::Repair;
    mismatch.sourceRoot = wrongSource;
    mismatch.expectedPackageId = "zero.acceptance.game";
    mismatch.trustPolicy = PackageTrustPolicy::AllowLocalUnsigned;
    if (packages.Execute(mismatch).success) return 32;

    PackageOperationRequest repair;
    repair.operation = PackageOperation::Repair;
    repair.sourceRoot = sourceV2;
    repair.expectedPackageId = "zero.acceptance.game";
    repair.trustPolicy = PackageTrustPolicy::AllowLocalUnsigned;
    const auto repaired = packages.Execute(repair);
    if (!repaired.success || repaired.finalStage != PackageStage::Ready) return 40;
    if (repaired.version != "1.1.0") return 41;
    if (!PackageIntegrityVerifier::Verify(repaired.installedRoot, integrityError)) return 42;

    const auto parsedV2 = PackageManifestParser::ParseFile(repaired.installedRoot / L"zero.manifest.json",
                                                           repaired.installedRoot);
    if (!parsedV2.valid || parsedV2.manifest.version != "1.1.0") return 43;
    if (!std::filesystem::exists(database)) return 44;

    const auto repairedLaunch = kernel.RequestLaunch(localRequest);
    if (!repairedLaunch.authorized || repairedLaunch.descriptor.version != "1.1.0") return 45;

    std::cout << "ZERO V5 production package + platform kernel acceptance: PASS\n";
    return 0;
}
