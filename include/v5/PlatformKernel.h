#pragma once

#include "ContentModel.h"
#include <optional>
#include <string>
#include <vector>

namespace zero::v5 {

struct AccountSession {
    std::string accountId;
    std::string sessionId;
    std::string displayName;
    bool authenticated{false};
};

enum class LaunchAuthorityRequirement : unsigned char {
    LocalPackage,
    ZeroService
};

struct CatalogItem {
    ContentIdentity identity;
    std::string title;
    std::string version;
    bool cloudReady{false};
    bool locallyInstallable{true};
    LaunchAuthorityRequirement authorityRequirement{LaunchAuthorityRequirement::ZeroService};
};

enum class LaunchFailure : unsigned char {
    None,
    NoAuthenticatedSession,
    ContentNotFound,
    EntitlementUnavailable,
    WrongAccount,
    WrongContent,
    WrongAuthority,
    AuthorizationDenied
};

struct LaunchResult {
    LaunchFailure failure{LaunchFailure::None};
    std::optional<LaunchDescriptor> descriptor;

    bool Allowed() const noexcept { return failure == LaunchFailure::None && descriptor.has_value(); }
};

class IIdentityService {
public:
    virtual ~IIdentityService() = default;
    virtual std::optional<AccountSession> CurrentSession() const = 0;
};

class ICatalogService {
public:
    virtual ~ICatalogService() = default;
    virtual std::vector<CatalogItem> Discover() const = 0;
    virtual std::optional<CatalogItem> FindByContentId(const std::string& contentId) const = 0;
};

class IEntitlementService {
public:
    virtual ~IEntitlementService() = default;
    virtual std::optional<EntitlementGrant> GetLaunchEntitlement(const std::string& accountId,
                                                                  const std::string& contentId) const = 0;
};

class ILaunchAuthority {
public:
    virtual ~ILaunchAuthority() = default;
    virtual std::optional<LaunchDescriptor> Authorize(const AccountSession& session,
                                                       const CatalogItem& item,
                                                       const EntitlementGrant& entitlement) const = 0;
};

class PlatformKernel {
public:
    PlatformKernel(IIdentityService& identity,
                   ICatalogService& catalog,
                   IEntitlementService& entitlements,
                   ILaunchAuthority& launchAuthority)
        : identity_(identity), catalog_(catalog), entitlements_(entitlements), launchAuthority_(launchAuthority) {}

    LaunchResult RequestLaunch(const std::string& contentId) const {
        const auto session = identity_.CurrentSession();
        if (!session || !session->authenticated) return {LaunchFailure::NoAuthenticatedSession, std::nullopt};

        const auto item = catalog_.FindByContentId(contentId);
        if (!item) return {LaunchFailure::ContentNotFound, std::nullopt};

        const auto entitlement = entitlements_.GetLaunchEntitlement(session->accountId, contentId);
        if (!entitlement) return {LaunchFailure::EntitlementUnavailable, std::nullopt};
        if (entitlement->accountId != session->accountId) return {LaunchFailure::WrongAccount, std::nullopt};
        if (entitlement->contentId != contentId) return {LaunchFailure::WrongContent, std::nullopt};

        const auto requiredAuthority = item->authorityRequirement == LaunchAuthorityRequirement::ZeroService
            ? AuthoritySource::ZeroService
            : AuthoritySource::LocalPackage;
        if (entitlement->authority != requiredAuthority) return {LaunchFailure::WrongAuthority, std::nullopt};

        auto descriptor = launchAuthority_.Authorize(*session, *item, *entitlement);
        if (!descriptor) return {LaunchFailure::AuthorizationDenied, std::nullopt};
        return {LaunchFailure::None, std::move(descriptor)};
    }

private:
    IIdentityService& identity_;
    ICatalogService& catalog_;
    IEntitlementService& entitlements_;
    ILaunchAuthority& launchAuthority_;
};

} // namespace zero::v5
