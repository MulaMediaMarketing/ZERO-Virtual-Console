#include "v5/ContentModel.h"
#include "v5/PackagePipeline.h"
#include "v5/PlatformKernel.h"
#include "v5/RuntimeCore.h"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {
using namespace zero::v5;

struct Identity final : IIdentityService {
    bool authenticated{true};
    std::optional<AccountSession> CurrentSession() const override {
        if (!authenticated) return std::nullopt;
        return AccountSession{"acct-1", "session-1", "ZERO_KING", true};
    }
};

struct Catalog final : ICatalogService {
    CatalogItem item{{"content-1", "pkg.game", "publisher.zero", ContentClass::FreeToPlay},
                     "Test Experience", "1.0.0", true, true,
                     LaunchAuthorityRequirement::ZeroService};
    std::vector<CatalogItem> Discover() const override { return {item}; }
    std::optional<CatalogItem> FindByContentId(const std::string& id) const override {
        return id == item.identity.contentId ? std::optional<CatalogItem>{item} : std::nullopt;
    }
};

struct Entitlements final : IEntitlementService {
    AuthoritySource authority{AuthoritySource::ZeroService};
    std::string accountOverride;
    std::string contentOverride;
    bool available{true};

    std::optional<EntitlementGrant> GetLaunchEntitlement(const std::string& accountId,
                                                          const std::string& contentId) const override {
        if (!available) return std::nullopt;
        return EntitlementGrant{"ent-1",
                                accountOverride.empty() ? accountId : accountOverride,
                                contentOverride.empty() ? contentId : contentOverride,
                                EntitlementType::Free,
                                authority,
                                "authority-v1",
                                {}};
    }
};

struct LaunchAuthority final : ILaunchAuthority {
    bool allow{true};
    std::optional<LaunchDescriptor> Authorize(const AccountSession& session,
                                               const CatalogItem& item,
                                               const EntitlementGrant& entitlement) const override {
        if (!allow || !session.authenticated || entitlement.authority == AuthoritySource::None) return std::nullopt;
        LaunchDescriptor descriptor;
        descriptor.packageId = item.identity.packageId;
        descriptor.version = item.version;
        descriptor.runtimeType = RuntimeType::NativeWin32;
        descriptor.executable = "game.exe";
        descriptor.capabilityScope = "save.read save.write achievements input";
        descriptor.entitlementToken = "server-entitlement-token";
        descriptor.sessionToken = "server-session-token";
        return descriptor;
    }
};

struct Validator final : IPackageValidator {
    bool Validate(const PackageCandidate& candidate, std::string& error) const override {
        if (candidate.identity.packageId.empty()) { error = "missing package id"; return false; }
        return true;
    }
};

struct Trust final : IPackageTrustVerifier {
    bool Verify(const PackageCandidate& candidate, std::string& error) const override {
        if (candidate.identity.publisherId.empty()) { error = "missing publisher"; return false; }
        return true;
    }
};

struct Installer final : IPackageInstaller {
    std::optional<PackageCommit> Install(const PackageCandidate& candidate, std::string&) override {
        return PackageCommit{candidate.stagingRoot / "installed", candidate.identity, candidate.version};
    }
};

struct Registry final : IPackageRegistry {
    bool Register(const PackageCommit& commit, std::string& error) override {
        if (commit.identity.packageId.empty()) { error = "invalid commit"; return false; }
        return true;
    }
};

struct LaunchPolicy final : ILaunchPolicy {
    bool Validate(const RuntimeSessionRequest& request, std::string& error) const override {
        if (request.identity.sessionId.empty() || request.launch.sessionToken.empty() ||
            request.launch.entitlementToken.empty()) {
            error = "missing authoritative launch identity";
            return false;
        }
        return true;
    }
};

struct Capabilities final : ICapabilityBroker {
    std::optional<RuntimeSessionGrant> Grant(const RuntimeSessionRequest& request,
                                             std::string&) const override {
        return RuntimeSessionGrant{request.identity, request.requestedCapabilities, "ipc-secret"};
    }
};

struct Process final : IProcessSupervisor {
    bool launched{false};
    bool Launch(const LaunchDescriptor& descriptor, const RuntimeSessionGrant& grant,
                std::string& error) override {
        if (descriptor.executable.empty() || grant.ipcAuthenticationToken.empty()) {
            error = "invalid runtime launch";
            return false;
        }
        launched = true;
        return true;
    }
};
}

int main() {
    using namespace zero::v5;

    Identity identity;
    Catalog catalog;
    Entitlements entitlements;
    LaunchAuthority authority;
    PlatformKernel kernel(identity, catalog, entitlements, authority);

    const auto launch = kernel.RequestLaunch("content-1");
    if (!launch.Allowed() || !launch.descriptor) return 10;
    if (launch.descriptor->packageId != "pkg.game") return 11;

    entitlements.authority = AuthoritySource::LocalPackage;
    if (kernel.RequestLaunch("content-1").failure != LaunchFailure::WrongAuthority) return 12;
    entitlements.authority = AuthoritySource::ZeroService;

    entitlements.accountOverride = "other-account";
    if (kernel.RequestLaunch("content-1").failure != LaunchFailure::WrongAccount) return 13;
    entitlements.accountOverride.clear();

    entitlements.contentOverride = "other-content";
    if (kernel.RequestLaunch("content-1").failure != LaunchFailure::WrongContent) return 14;
    entitlements.contentOverride.clear();

    entitlements.available = false;
    if (kernel.RequestLaunch("content-1").failure != LaunchFailure::EntitlementUnavailable) return 15;
    entitlements.available = true;

    identity.authenticated = false;
    if (kernel.RequestLaunch("content-1").failure != LaunchFailure::NoAuthenticatedSession) return 16;
    identity.authenticated = true;

    Validator validator;
    Trust trust;
    Installer installer;
    Registry registry;
    PackagePipeline packages(validator, trust, installer, registry);
    PackageCandidate candidate;
    candidate.source = "source";
    candidate.stagingRoot = "stage";
    candidate.identity = catalog.item.identity;
    candidate.version = "1.0.0";
    std::string error;
    const auto committed = packages.Execute(candidate, error);
    if (!committed) return 20;

    LaunchPolicy policy;
    Capabilities capabilities;
    Process process;
    RuntimeCore runtime(policy, capabilities, process);
    RuntimeSessionRequest request;
    request.launch = *launch.descriptor;
    request.identity = {"runtime-session-1", launch.descriptor->packageId, "acct-1"};
    request.requestedCapabilities = {RuntimeCapability::SaveRead,
                                     RuntimeCapability::SaveWrite,
                                     RuntimeCapability::Achievements,
                                     RuntimeCapability::Input};
    const auto grant = runtime.Start(request, error);
    if (!grant || !process.launched) return 30;

    std::cout << "ZERO Core Architecture V5 acceptance: PASS\n";
    return 0;
}
