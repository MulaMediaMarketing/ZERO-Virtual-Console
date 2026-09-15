#include "v5/CommerceDomain.h"
#include <iostream>

using namespace zero::v5;

namespace {
class Transport final : public ICommerceTransport {
public:
    std::optional<CheckoutQuote> quote;
    PurchaseResult purchase;
    std::optional<CheckoutQuote> Quote(const std::string&, const std::vector<CheckoutLine>&) override { return quote; }
    PurchaseResult Complete(const std::string&, const std::string&) override { return purchase; }
};

StoreOffer offer() {
    return StoreOffer{"offer-1", "content-1", "standard", "USD", 1999, 2999, true, AuthoritySource::ZeroService};
}
}

int main() {
    WishlistDomain wishlist;
    std::string error;
    if (!wishlist.Add({"acct-1", "content-1", 100, 1500}, error)) return 10;
    if (!wishlist.Contains("acct-1", "content-1")) return 11;
    if (!wishlist.Remove("acct-1", "content-1")) return 12;

    Transport transport;
    CheckoutAuthority checkout(transport);
    const auto line = CheckoutLine{offer(), 1};
    CheckoutQuote quote{"quote-1", "acct-1", "USD", {line}, 1999, 160, 2159, 2000, AuthoritySource::ZeroService};
    transport.quote = quote;
    const auto created = checkout.CreateQuote("acct-1", {line}, 1000, error);
    if (!created || checkout.State() != CheckoutState::Ready) return 20;

    auto grant = EntitlementGrant{"ent-1", "acct-1", "content-1", EntitlementType::Purchased,
                                  AuthoritySource::ZeroService, "v1", {}};
    transport.purchase = {true, "order-1", {grant}, {}};
    const auto result = checkout.Complete(*created, "payment-token", 1000);
    if (!result.completed || checkout.State() != CheckoutState::Completed) return 21;

    auto forged = offer();
    forged.authority = AuthoritySource::LocalPackage;
    if (CheckoutAuthority::ValidateOffer(forged, error)) return 22;

    auto expired = quote;
    expired.expiresAtEpochSeconds = 900;
    if (CheckoutAuthority::ValidateQuote(expired, "acct-1", 1000, error)) return 23;

    auto badTotals = quote;
    badTotals.totalMinor = 9999;
    if (CheckoutAuthority::ValidateQuote(badTotals, "acct-1", 1000, error)) return 24;

    DisconnectedCommerceTransport disconnected;
    CheckoutAuthority offline(disconnected);
    if (offline.CreateQuote("acct-1", {line}, 1000, error)) return 25;

    std::cout << "ZERO V5 Store + Wishlist + Checkout acceptance: PASS\n";
    return 0;
}
