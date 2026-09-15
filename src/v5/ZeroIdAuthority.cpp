#include "v5/ZeroIdAuthority.h"

namespace zero::v5 {

ZeroIdResult DisconnectedZeroIdTransport::ExchangeAuthorizationCode(const std::string&) {
    return {IdentityClientState::Disconnected, std::nullopt, "Zero ID authority is not connected"};
}

ZeroIdResult DisconnectedZeroIdTransport::Refresh(const std::string&) {
    return {IdentityClientState::Disconnected, std::nullopt, "Zero ID authority is not connected"};
}

bool DisconnectedZeroIdTransport::Revoke(const std::string&, std::string& error) {
    error = "Zero ID authority is not connected";
    return false;
}

bool ZeroIdAuthorityClient::ValidAuthoritativeSession(const ZeroIdSession& session,
                                                      std::uint64_t nowEpochSeconds,
                                                      std::string& error) {
    if (session.authority != AuthoritySource::ZeroService) {
        error = "identity session was not issued by ZERO service";
        return false;
    }
    if (session.accountId.empty() || session.sessionId.empty() ||
        session.accessToken.empty() || session.refreshHandle.empty()) {
        error = "identity session is missing authoritative fields";
        return false;
    }
    if (session.expiresAtEpochSeconds <= nowEpochSeconds) {
        error = "identity session is expired";
        return false;
    }
    return true;
}

ZeroIdResult ZeroIdAuthorityClient::SignIn(const std::string& authorizationCode,
                                           std::uint64_t nowEpochSeconds) {
    if (authorizationCode.empty())
        return {IdentityClientState::Error, std::nullopt, "authorization code is required"};
    auto result = transport_.ExchangeAuthorizationCode(authorizationCode);
    if (result.state != IdentityClientState::Ready || !result.session) return result;
    std::string error;
    if (!ValidAuthoritativeSession(*result.session, nowEpochSeconds, error))
        return {IdentityClientState::Error, std::nullopt, error};
    return result;
}

ZeroIdResult ZeroIdAuthorityClient::RefreshSession(const ZeroIdSession& current,
                                                   std::uint64_t nowEpochSeconds) {
    if (current.refreshHandle.empty())
        return {IdentityClientState::Error, std::nullopt, "refresh handle is required"};
    auto result = transport_.Refresh(current.refreshHandle);
    if (result.state != IdentityClientState::Ready || !result.session) return result;
    std::string error;
    if (!ValidAuthoritativeSession(*result.session, nowEpochSeconds, error))
        return {IdentityClientState::Error, std::nullopt, error};
    if (result.session->accountId != current.accountId) {
        return {IdentityClientState::Error, std::nullopt, "refresh changed account identity"};
    }
    return result;
}

bool ZeroIdAuthorityClient::SignOut(const ZeroIdSession& current, std::string& error) {
    if (current.sessionId.empty()) {
        error = "session identity is required";
        return false;
    }
    return transport_.Revoke(current.sessionId, error);
}

} // namespace zero::v5
