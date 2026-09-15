#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace zero::v5 {

enum class AuthorityClientState : std::uint8_t { Disconnected, Ready, Error };

struct CatalogRecord {
    ContentIdentity identity;
    std::string title;
    std::string version;
    bool available{false};
    bool cloudReady{false};
    AuthoritySource authority{AuthoritySource::None};
};

struct CatalogResult {
    AuthorityClientState state{AuthorityClientState::Disconnected};
    std::vector<CatalogRecord> records;
    std::string error;
};

struct EntitlementResult {
    AuthorityClientState state{AuthorityClientState::Disconnected};
    std::optional<EntitlementGrant> entitlement;
    std::string error;
};

class ICatalogTransport {
public:
    virtual ~ICatalogTransport() = default;
    virtual CatalogResult FetchCatalog(const std::string& accountId) = 0;
    virtual std::optional<CatalogRecord> FetchContent(const std::string& accountId,
                                                       const std::string& contentId) = 0;
};

class IEntitlementTransport {
public:
    virtual ~IEntitlementTransport() = default;
    virtual EntitlementResult FetchLaunchEntitlement(const std::string& accountId,
                                                      const std::string& contentId) = 0;
};

class DisconnectedCatalogTransport final : public ICatalogTransport {
public:
    CatalogResult FetchCatalog(const std::string&) override;
    std::optional<CatalogRecord> FetchContent(const std::string&, const std::string&) override;
};

class DisconnectedEntitlementTransport final : public IEntitlementTransport {
public:
    EntitlementResult FetchLaunchEntitlement(const std::string&, const std::string&) override;
};

class CatalogAuthorityClient {
public:
    explicit CatalogAuthorityClient(ICatalogTransport& transport) : transport_(transport) {}
    CatalogResult Refresh(const std::string& accountId);
    std::optional<CatalogRecord> Resolve(const std::string& accountId,
                                         const std::string& contentId,
                                         std::string& error);
    static bool Validate(const CatalogRecord& record, std::string& error);
private:
    ICatalogTransport& transport_;
};

class EntitlementAuthorityClient {
public:
    explicit EntitlementAuthorityClient(IEntitlementTransport& transport) : transport_(transport) {}
    EntitlementResult ResolveForLaunch(const std::string& accountId,
                                       const CatalogRecord& record);
    static bool Validate(const EntitlementGrant& grant,
                         const std::string& accountId,
                         const std::string& contentId,
                         std::string& error);
private:
    IEntitlementTransport& transport_;
};

} // namespace zero::v5
