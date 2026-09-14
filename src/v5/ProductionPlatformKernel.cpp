#include "v5/ProductionPlatformKernel.h"

#include "ManifestValidator.h"
#include "PackageIntegrityVerifier.h"
#include "PackageManifestParser.h"
#include "Utf8Path.h"

namespace zero::v5 {

std::optional<LaunchDescriptor> DisconnectedManagedLaunchAuthorityClient::Authorize(
    const std::string& accountId,
    const std::string& contentId,
    std::wstring& error) const {
    (void)accountId;
    (void)contentId;
    error = L"ZERO managed launch authority is unavailable. Managed content remains disconnected.";
    return std::nullopt;
}

ProductionPlatformKernel::ProductionPlatformKernel(
    std::filesystem::path libraryRoot,
    const IPublisherTrustProvider& trustProvider,
    const IManagedLaunchAuthorityClient& managedLaunchAuthority)
    : libraryRoot_(std::move(libraryRoot)),
      trustProvider_(trustProvider),
      managedLaunchAuthority_(managedLaunchAuthority) {}

ProductionLaunchResult ProductionPlatformKernel::RequestLaunch(const ProductionLaunchRequest& request) const {
    ProductionLaunchResult result;
    if (request.contentId.empty()) {
        result.error = L"ZERO V5 launch request is missing content identity.";
        return result;
    }

    if (request.mode == LaunchRequestMode::LocalInstalled) {
        return RequestLocalInstalled(request.contentId);
    }

    if (request.accountId.empty()) {
        result.error = L"ZERO managed launch requires an authenticated account identity.";
        return result;
    }

    const auto managed = managedLaunchAuthority_.Authorize(request.accountId, request.contentId, result.error);
    if (!managed || managed->authorityKind != LaunchAuthorityKind::ServerManaged ||
        managed->packageId.empty() || managed->sessionToken.empty() || managed->entitlementToken.empty()) {
        if (result.error.empty()) result.error = L"ZERO managed launch authority rejected the request.";
        return result;
    }

    result.authorized = true;
    result.descriptor = *managed;
    return result;
}

ProductionLaunchResult ProductionPlatformKernel::RequestLocalInstalled(const std::string& contentId) const {
    ProductionLaunchResult result;
    if (!ManifestValidator::IsSafePackageId(contentId)) {
        result.error = L"ZERO V5 rejected an unsafe local package identity.";
        return result;
    }

    const auto relative = PathFromUtf8(contentId);
    if (!relative || relative->empty()) {
        result.error = L"ZERO V5 could not resolve the local package identity.";
        return result;
    }

    const auto packageRoot = libraryRoot_ / *relative;
    const auto parsed = PackageManifestParser::ParseFile(packageRoot / L"zero.manifest.json", packageRoot);
    if (!parsed.valid || parsed.manifest.packageId != contentId) {
        result.error = parsed.error.empty() ? L"ZERO V5 local package manifest is invalid." : parsed.error;
        return result;
    }

    if (!PackageIntegrityVerifier::Verify(packageRoot, result.error)) return result;

    const auto trust = PackageTrustService::Evaluate(packageRoot,
                                                     PackageTrustPolicy::AllowLocalUnsigned,
                                                     trustProvider_);
    if (!trust.launchAllowed) {
        result.error = trust.detail.empty() ? L"ZERO V5 local package trust check failed." : trust.detail;
        return result;
    }

    LaunchDescriptor descriptor;
    descriptor.packageId = parsed.manifest.packageId;
    descriptor.version = parsed.manifest.version;
    descriptor.runtimeType = RuntimeType::NativeWin32;
    descriptor.contentRoot = packageRoot.u8string();
    descriptor.executable = parsed.manifest.executable.u8string();
    descriptor.saveNamespace = parsed.manifest.packageId;
    descriptor.authorityKind = LaunchAuthorityKind::LocalPackage;

    if (parsed.manifest.zeroResume) descriptor.capabilityScope += "resume ";
    if (parsed.manifest.zeroAchievements) descriptor.capabilityScope += "achievements ";
    if (parsed.manifest.zeroOverlay) descriptor.capabilityScope += "overlay ";
    if (parsed.manifest.zeroInput) descriptor.capabilityScope += "input ";
    descriptor.capabilityScope += "save.read save.write";

    result.authorized = true;
    result.descriptor = std::move(descriptor);
    return result;
}

} // namespace zero::v5
