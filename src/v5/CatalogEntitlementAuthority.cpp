#include "v5/CatalogEntitlementAuthority.h"

namespace zero::v5 {

CatalogResult DisconnectedCatalogTransport::FetchCatalog(const std::string&) {
    return {AuthorityClientState::Disconnected, {}, "catalog authority is not connected"};
}

std::optional<CatalogRecord> DisconnectedCatalogTransport::FetchContent(const std::string&, const std::string&) {
    return std::nullopt;
}

EntitlementResult DisconnectedEntitlementTransport::FetchLaunchEntitlement(const std::string&, const std::string&) {
    return {AuthorityClientState::Disconnected, std::nullopt, "entitlement authority is not connected"};
}

bool CatalogAuthorityClient::Validate(const CatalogRecord& record, std::string& error) {
    if (record.authority != AuthoritySource::ZeroService) {
        error = "managed catalog record was not issued by ZERO service";
        return false;
    }
    if (record.identity.contentId.empty() || record.identity.packageId.empty() ||
        record.identity.publisherId.empty() || record.title.empty() || record.version.empty()) {
        error = "catalog record is missing required identity or version fields";
        return false;
    }
    return true;
}

CatalogResult CatalogAuthorityClient::Refresh(const std::string& accountId) {
    if (accountId.empty()) return {AuthorityClientState::Error, {}, "account identity is required"};
    auto result = transport_.FetchCatalog(accountId);
    if (result.state != AuthorityClientState::Ready) return result;
    for (const auto& record : result.records) {
        std::string error;
        if (!Validate(record, error)) return {AuthorityClientState::Error, {}, error};
    }
    return result;
}

std::optional<CatalogRecord> CatalogAuthorityClient::Resolve(const std::string& accountId,
                                                             const std::string& contentId,
                                                             std::string& error) {
    if (accountId.empty() || contentId.empty()) {
        error = "account and content identity are required";
        return std::nullopt;
    }
    const auto record = transport_.FetchContent(accountId, contentId);
    if (!record) {
        error = "catalog content is unavailable";
        return std::nullopt;
    }
    if (record->identity.contentId != contentId || !Validate(*record, error)) return std::nullopt;
    return record;
}

bool EntitlementAuthorityClient::Validate(const EntitlementGrant& grant,
                                          const std::string& accountId,
                                          const std::string& contentId,
                                          std::string& error) {
    if (grant.authority != AuthoritySource::ZeroService) {
        error = "launch entitlement was not issued by ZERO service";
        return false;
    }
    if (grant.entitlementId.empty() || grant.accountId != accountId || grant.contentId != contentId) {
        error = "launch entitlement identity does not match the request";
        return false;
    }
    return true;
}

EntitlementResult EntitlementAuthorityClient::ResolveForLaunch(const std::string& accountId,
                                                               const CatalogRecord& record) {
    std::string catalogError;
    if (!CatalogAuthorityClient::Validate(record, catalogError) || !record.available)
        return {AuthorityClientState::Error, std::nullopt,
                catalogError.empty() ? "catalog content is unavailable" : catalogError};
    auto result = transport_.FetchLaunchEntitlement(accountId, record.identity.contentId);
    if (result.state != AuthorityClientState::Ready || !result.entitlement) return result;
    std::string error;
    if (!Validate(*result.entitlement, accountId, record.identity.contentId, error))
        return {AuthorityClientState::Error, std::nullopt, error};
    return result;
}

} // namespace zero::v5
