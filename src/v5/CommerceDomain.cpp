#include "v5/CommerceDomain.h"
#include <algorithm>
#include <limits>

namespace zero::v5 {

std::optional<CheckoutQuote> DisconnectedCommerceTransport::Quote(const std::string&,
                                                                  const std::vector<CheckoutLine>&) {
    return std::nullopt;
}

PurchaseResult DisconnectedCommerceTransport::Complete(const std::string&, const std::string&) {
    return {false, {}, {}, "commerce authority is not connected"};
}

std::string WishlistDomain::Key(const std::string& accountId, const std::string& contentId) {
    return accountId + "\n" + contentId;
}

bool WishlistDomain::Add(WishlistEntry entry, std::string& error) {
    if (entry.accountId.empty() || entry.contentId.empty()) {
        error = "wishlist entry requires account and content identity";
        return false;
    }
    entries_[Key(entry.accountId, entry.contentId)] = std::move(entry);
    return true;
}

bool WishlistDomain::Remove(const std::string& accountId, const std::string& contentId) {
    return entries_.erase(Key(accountId, contentId)) > 0;
}

bool WishlistDomain::Contains(const std::string& accountId, const std::string& contentId) const {
    return entries_.contains(Key(accountId, contentId));
}

std::vector<WishlistEntry> WishlistDomain::Entries(const std::string& accountId) const {
    std::vector<WishlistEntry> result;
    for (const auto& [_, entry] : entries_) {
        if (entry.accountId == accountId) result.push_back(entry);
    }
    std::stable_sort(result.begin(), result.end(), [](const WishlistEntry& a, const WishlistEntry& b) {
        return a.addedEpochSeconds > b.addedEpochSeconds;
    });
    return result;
}

bool CheckoutAuthority::ValidateOffer(const StoreOffer& offer, std::string& error) {
    if (offer.authority != AuthoritySource::ZeroService) {
        error = "store offer was not issued by ZERO service";
        return false;
    }
    if (offer.offerId.empty() || offer.contentId.empty() || offer.currency.size() != 3 || !offer.purchasable) {
        error = "store offer is incomplete or not purchasable";
        return false;
    }
    if (offer.originalAmountMinor != 0 && offer.unitAmountMinor > offer.originalAmountMinor) {
        error = "store offer sale amount exceeds original amount";
        return false;
    }
    return true;
}

bool CheckoutAuthority::ValidateQuote(const CheckoutQuote& quote,
                                      const std::string& accountId,
                                      std::uint64_t nowEpochSeconds,
                                      std::string& error) {
    if (quote.authority != AuthoritySource::ZeroService || quote.quoteId.empty() || quote.accountId != accountId) {
        error = "checkout quote is not authoritative for this account";
        return false;
    }
    if (quote.lines.empty() || quote.currency.size() != 3 || quote.expiresAtEpochSeconds <= nowEpochSeconds) {
        error = "checkout quote is empty, malformed, or expired";
        return false;
    }
    std::uint64_t subtotal = 0;
    for (const auto& line : quote.lines) {
        std::string offerError;
        if (!ValidateOffer(line.offer, offerError) || line.quantity == 0 || line.offer.currency != quote.currency) {
            error = offerError.empty() ? "checkout line is invalid" : offerError;
            return false;
        }
        const auto lineTotal = line.offer.unitAmountMinor * static_cast<std::uint64_t>(line.quantity);
        if (line.quantity != 0 && lineTotal / line.quantity != line.offer.unitAmountMinor) {
            error = "checkout amount overflow";
            return false;
        }
        if (lineTotal > std::numeric_limits<std::uint64_t>::max() - subtotal) {
            error = "checkout subtotal overflow";
            return false;
        }
        subtotal += lineTotal;
    }
    if (quote.taxMinor > std::numeric_limits<std::uint64_t>::max() - quote.subtotalMinor) {
        error = "checkout total overflow";
        return false;
    }
    const auto expectedTotal = quote.subtotalMinor + quote.taxMinor;
    if (subtotal != quote.subtotalMinor || quote.totalMinor != expectedTotal) {
        error = "checkout totals do not reconcile";
        return false;
    }
    return true;
}

std::optional<CheckoutQuote> CheckoutAuthority::CreateQuote(const std::string& accountId,
                                                            const std::vector<CheckoutLine>& lines,
                                                            std::uint64_t nowEpochSeconds,
                                                            std::string& error) {
    if (accountId.empty() || lines.empty()) {
        error = "checkout requires account identity and cart lines";
        state_ = CheckoutState::Failed;
        return std::nullopt;
    }
    for (const auto& line : lines) {
        if (!ValidateOffer(line.offer, error) || line.quantity == 0) {
            state_ = CheckoutState::Failed;
            return std::nullopt;
        }
    }
    auto quote = transport_.Quote(accountId, lines);
    if (!quote || !ValidateQuote(*quote, accountId, nowEpochSeconds, error)) {
        if (error.empty()) error = "commerce authority did not return a valid quote";
        state_ = CheckoutState::Failed;
        return std::nullopt;
    }
    state_ = CheckoutState::Ready;
    return quote;
}

PurchaseResult CheckoutAuthority::Complete(const CheckoutQuote& quote,
                                           const std::string& paymentMethodToken,
                                           std::uint64_t nowEpochSeconds) {
    std::string error;
    if (!ValidateQuote(quote, quote.accountId, nowEpochSeconds, error) || paymentMethodToken.empty()) {
        state_ = CheckoutState::Failed;
        return {false, {}, {}, error.empty() ? "payment method token is required" : error};
    }
    state_ = CheckoutState::Submitting;
    auto result = transport_.Complete(quote.quoteId, paymentMethodToken);
    if (!result.completed || result.orderId.empty()) {
        state_ = CheckoutState::Failed;
        if (result.error.empty()) result.error = "commerce authority did not complete the order";
        return result;
    }
    for (const auto& entitlement : result.entitlements) {
        if (entitlement.authority != AuthoritySource::ZeroService || entitlement.accountId != quote.accountId) {
            state_ = CheckoutState::Failed;
            return {false, {}, {}, "order returned a non-authoritative entitlement"};
        }
    }
    state_ = CheckoutState::Completed;
    return result;
}

} // namespace zero::v5
