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

struct CatalogItem {
    ContentIdentity identity;
    std::string title;
    std::string version;
    bool cloudReady{false};
    bool locallyInstallable{true};
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

    std::optional<LaunchDescriptor> RequestLaunch(const std::string& contentId) const {
        const auto session = identity_.CurrentSession();
        if (!session || !session->authenticated) return std::nullopt;
        const auto item = catalog_.FindByContentId(contentId);
        if (!item) return std::nullopt;
        const auto entitlement = entitlements_.GetLaunchEntitlement(session->accountId, contentId);
        if (!entitlement || !entitlement->authoritative) return std::nullopt;
        if (entitlement->accountId != session->accountId || entitlement->contentId != contentId) return std::nullopt;
        return launchAuthority_.Authorize(*session, *item, *entitlement);
    }

private:
    IIdentityService& identity_;
    ICatalogService& catalog_;
    IEntitlementService& entitlements_;
    ILaunchAuthority& launchAuthority_;
};

} // namespace zero::v5
