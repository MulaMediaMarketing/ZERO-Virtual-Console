#include "v5/ContentDeliveryDomain.h"
#include <algorithm>
#include <cctype>

namespace zero::v5 {
namespace {

bool IsSha256Hex(const std::string& value) noexcept {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

bool IsHttpsUrl(const std::string& value) noexcept {
    static constexpr char prefix[] = "https://";
    return value.size() > sizeof(prefix) - 1 &&
           value.compare(0, sizeof(prefix) - 1, prefix) == 0;
}

ContentTransferResult Invalid(std::string error) {
    ContentTransferResult result;
    result.state = ContentDeliveryClientState::Error;
    result.error = std::move(error);
    return result;
}

} // namespace

ContentTransferResult DisconnectedContentDeliveryTransport::RequestTransfer(const std::string&,
                                                                            const std::string&) {
    ContentTransferResult result;
    result.state = ContentDeliveryClientState::Disconnected;
    result.error = "ZERO content delivery is not connected.";
    return result;
}

ContentTransferResult ContentDeliveryClient::Begin(const std::string& contentId,
                                                   const std::string& version,
                                                   std::uint64_t nowEpochSeconds) const {
    if (contentId.empty() || version.empty())
        return Invalid("Content ID and version are required before requesting delivery.");

    auto result = transport_.RequestTransfer(contentId, version);
    if (result.state != ContentDeliveryClientState::Ready) {
        result.ticket.reset();
        if (result.error.empty()) {
            result.error = result.state == ContentDeliveryClientState::Disconnected
                ? "ZERO content delivery is not connected."
                : "ZERO content delivery returned an error.";
        }
        return result;
    }

    if (!result.ticket)
        return Invalid("Content delivery reported Ready without an authoritative transfer ticket.");

    std::string validationError;
    if (!ValidateAuthoritativeTicket(*result.ticket, contentId, version, nowEpochSeconds, validationError))
        return Invalid(validationError);

    result.error.clear();
    return result;
}

bool ContentDeliveryClient::ValidateAuthoritativeTicket(const ContentTransferTicket& ticket,
                                                        const std::string& requestedContentId,
                                                        const std::string& requestedVersion,
                                                        std::uint64_t nowEpochSeconds,
                                                        std::string& error) noexcept {
    if (ticket.authority != AuthoritySource::ZeroService) {
        error = "Managed content delivery requires ZERO service authority.";
        return false;
    }
    if (ticket.contentId.empty() || ticket.contentId != requestedContentId) {
        error = "Transfer ticket content identity does not match the request.";
        return false;
    }
    if (ticket.packageId.empty()) {
        error = "Transfer ticket is missing a package identity.";
        return false;
    }
    if (ticket.version.empty() || ticket.version != requestedVersion) {
        error = "Transfer ticket version does not match the request.";
        return false;
    }
    if (!IsHttpsUrl(ticket.httpsUrl)) {
        error = "Transfer ticket must use HTTPS.";
        return false;
    }
    if (!IsSha256Hex(ticket.sha256Hex)) {
        error = "Transfer ticket must provide a valid SHA-256 digest.";
        return false;
    }
    if (ticket.expectedBytes == 0) {
        error = "Transfer ticket must provide a non-zero payload size.";
        return false;
    }
    if (ticket.expiresAtEpochSeconds <= nowEpochSeconds) {
        error = "Transfer ticket is expired.";
        return false;
    }
    error.clear();
    return true;
}

} // namespace zero::v5
