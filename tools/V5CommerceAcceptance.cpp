#include "v5/CommerceDomain.h"
#include <cstdint>
#include <iostream>
#include <limits>

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

    auto overflowingTotal = quote;
    overflowingTotal.subtotalMinor = std::numeric_limits<std::uint64_t>::max() - 5;
    overflowingTotal.taxMinor = 10;
    overflowingTotal.totalMinor = 4;
    if (CheckoutAuthority::ValidateQuote(overflowingTotal, "acct-1", 1000, error)) return 25;
    if (error != "checkout total overflow") return 26;

    auto overflowingSubtotal = quote;
    StoreOffer huge = offer();
    huge.unitAmountMinor = std::numeric_limits<std::uint64_t>::max();
    huge.originalAmountMinor = 0;
    overflowingSubtotal.lines = {CheckoutLine{huge, 1}, CheckoutLine{offer(), 1}};
    overflowingSubtotal.subtotalMinor = 0;
    overflowingSubtotal.taxMinor = 0;
    overflowingSubtotal.totalMinor = 0;
    if (CheckoutAuthority::ValidateQuote(overflowingSubtotal, "acct-1", 1000, error)) return 27;
    if (error != "checkout subtotal overflow") return 28;

    DisconnectedCommerceTransport disconnected;
    CheckoutAuthority offline(disconnected);
    if (offline.CreateQuote("acct-1", {line}, 1000, error)) return 29;

    std::cout << "ZERO V5 Store + Wishlist + Checkout acceptance: PASS\n";
    return 0;
}
