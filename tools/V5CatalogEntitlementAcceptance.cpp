#include "v5/CatalogEntitlementAuthority.h"
#include <iostream>

using namespace zero::v5;

namespace {
class CatalogTransport final : public ICatalogTransport {
public:
    CatalogResult result{};
    std::optional<CatalogRecord> record;
    CatalogResult FetchCatalog(const std::string&) override { return result; }
    std::optional<CatalogRecord> FetchContent(const std::string&, const std::string&) override { return record; }
};

class EntitlementTransport final : public IEntitlementTransport {
public:
    EntitlementResult result{};
    EntitlementResult FetchLaunchEntitlement(const std::string&, const std::string&) override { return result; }
};

CatalogRecord catalogRecord() {
    CatalogRecord record;
    record.identity = {"content-1", "pkg.game", "publisher.zero", ContentClass::FreeToPlay};
    record.title = "Game";
    record.version = "1.0.0";
    record.available = true;
    record.authority = AuthoritySource::ZeroService;
    return record;
}

EntitlementGrant entitlement() {
    EntitlementGrant grant;
    grant.entitlementId = "ent-1";
    grant.accountId = "acct-1";
    grant.contentId = "content-1";
    grant.type = EntitlementType::Free;
    grant.authority = AuthoritySource::ZeroService;
    return grant;
}
}

int main() {
    DisconnectedCatalogTransport offlineCatalog;
    CatalogAuthorityClient disconnectedCatalog(offlineCatalog);
    if (disconnectedCatalog.Refresh("acct-1").state != AuthorityClientState::Disconnected) return 10;

    CatalogTransport catalogTransport;
    CatalogAuthorityClient catalog(catalogTransport);
    auto forgedCatalog = catalogRecord();
    forgedCatalog.authority = AuthoritySource::LocalPackage;
    catalogTransport.result = {AuthorityClientState::Ready, {forgedCatalog}, {}};
    if (catalog.Refresh("acct-1").state != AuthorityClientState::Error) return 11;

    const auto validCatalog = catalogRecord();
    catalogTransport.result = {AuthorityClientState::Ready, {validCatalog}, {}};
    if (catalog.Refresh("acct-1").state != AuthorityClientState::Ready) return 12;
    catalogTransport.record = validCatalog;
    std::string error;
    if (!catalog.Resolve("acct-1", "content-1", error)) return 13;

    EntitlementTransport entitlementTransport;
    EntitlementAuthorityClient entitlements(entitlementTransport);
    auto forgedEntitlement = entitlement();
    forgedEntitlement.authority = AuthoritySource::LocalPackage;
    entitlementTransport.result = {AuthorityClientState::Ready, forgedEntitlement, {}};
    if (entitlements.ResolveForLaunch("acct-1", validCatalog).state != AuthorityClientState::Error) return 14;

    auto wrongAccount = entitlement();
    wrongAccount.accountId = "acct-2";
    entitlementTransport.result = {AuthorityClientState::Ready, wrongAccount, {}};
    if (entitlements.ResolveForLaunch("acct-1", validCatalog).state != AuthorityClientState::Error) return 15;

    const auto validEntitlement = entitlement();
    entitlementTransport.result = {AuthorityClientState::Ready, validEntitlement, {}};
    const auto resolved = entitlements.ResolveForLaunch("acct-1", validCatalog);
    if (resolved.state != AuthorityClientState::Ready || !resolved.entitlement ||
        resolved.entitlement->type != EntitlementType::Free) return 16;

    auto unavailable = validCatalog;
    unavailable.available = false;
    if (entitlements.ResolveForLaunch("acct-1", unavailable).state != AuthorityClientState::Error) return 17;

    std::cout << "ZERO V5 Entitlements + Catalog authority acceptance: PASS\n";
    return 0;
}
