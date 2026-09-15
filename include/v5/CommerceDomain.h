#pragma once

#include "CatalogEntitlementAuthority.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace zero::v5 {

struct StoreOffer {
    std::string offerId;
    std::string contentId;
    std::string editionId;
    std::string currency;
    std::uint64_t unitAmountMinor{0};
    std::uint64_t originalAmountMinor{0};
    bool purchasable{false};
    AuthoritySource authority{AuthoritySource::None};
};

struct WishlistEntry {
    std::string accountId;
    std::string contentId;
    std::uint64_t addedEpochSeconds{0};
    std::optional<std::uint64_t> targetPriceMinor;
};

struct CheckoutLine {
    StoreOffer offer;
    std::uint32_t quantity{1};
};

struct CheckoutQuote {
    std::string quoteId;
    std::string accountId;
    std::string currency;
    std::vector<CheckoutLine> lines;
    std::uint64_t subtotalMinor{0};
    std::uint64_t taxMinor{0};
    std::uint64_t totalMinor{0};
    std::uint64_t expiresAtEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

enum class CheckoutState : std::uint8_t {
    Empty,
    Ready,
    Submitting,
    Completed,
    Failed
};

struct PurchaseResult {
    bool completed{false};
    std::string orderId;
    std::vector<EntitlementGrant> entitlements;
    std::string error;
};

class ICommerceTransport {
public:
    virtual ~ICommerceTransport() = default;
    virtual std::optional<CheckoutQuote> Quote(const std::string& accountId,
                                               const std::vector<CheckoutLine>& lines) = 0;
    virtual PurchaseResult Complete(const std::string& quoteId,
                                    const std::string& paymentMethodToken) = 0;
};

class DisconnectedCommerceTransport final : public ICommerceTransport {
public:
    std::optional<CheckoutQuote> Quote(const std::string&, const std::vector<CheckoutLine>&) override;
    PurchaseResult Complete(const std::string&, const std::string&) override;
};

class WishlistDomain {
public:
    bool Add(WishlistEntry entry, std::string& error);
    bool Remove(const std::string& accountId, const std::string& contentId);
    bool Contains(const std::string& accountId, const std::string& contentId) const;
    std::vector<WishlistEntry> Entries(const std::string& accountId) const;
private:
    std::unordered_map<std::string, WishlistEntry> entries_;
    static std::string Key(const std::string& accountId, const std::string& contentId);
};

class CheckoutAuthority {
public:
    explicit CheckoutAuthority(ICommerceTransport& transport) : transport_(transport) {}

    std::optional<CheckoutQuote> CreateQuote(const std::string& accountId,
                                             const std::vector<CheckoutLine>& lines,
                                             std::uint64_t nowEpochSeconds,
                                             std::string& error);
    PurchaseResult Complete(const CheckoutQuote& quote,
                            const std::string& paymentMethodToken,
                            std::uint64_t nowEpochSeconds);
    CheckoutState State() const noexcept { return state_; }

    static bool ValidateOffer(const StoreOffer& offer, std::string& error);
    static bool ValidateQuote(const CheckoutQuote& quote,
                              const std::string& accountId,
                              std::uint64_t nowEpochSeconds,
                              std::string& error);

private:
    ICommerceTransport& transport_;
    CheckoutState state_{CheckoutState::Empty};
};

} // namespace zero::v5
