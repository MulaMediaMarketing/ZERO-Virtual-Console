#include "v5/ZeroIdAuthority.h"
#include <iostream>
#include <string>

using namespace zero::v5;

namespace {
class Transport final : public IZeroIdTransport {
public:
    ZeroIdResult exchangeResult{};
    ZeroIdResult refreshResult{};
    bool revokeResult{true};
    std::string revokedSession;

    ZeroIdResult ExchangeAuthorizationCode(const std::string&) override { return exchangeResult; }
    ZeroIdResult Refresh(const std::string&) override { return refreshResult; }
    bool Revoke(const std::string& sessionId, std::string& error) override {
        revokedSession = sessionId;
        if (!revokeResult) error = "revoke failed";
        return revokeResult;
    }
};

ZeroIdSession validSession(const char* account = "acct-1") {
    return ZeroIdSession{account, "session-1", "ZERO_KING", "access-token", "refresh-handle",
                         2000, AuthoritySource::ZeroService};
}
}

int main() {
    DisconnectedZeroIdTransport disconnected;
    ZeroIdAuthorityClient offline(disconnected);
    if (offline.SignIn("code", 1000).state != IdentityClientState::Disconnected) return 10;

    Transport transport;
    ZeroIdAuthorityClient client(transport);

    auto forged = validSession();
    forged.authority = AuthoritySource::LocalPackage;
    transport.exchangeResult = {IdentityClientState::Ready, forged, {}};
    if (client.SignIn("code", 1000).state != IdentityClientState::Error) return 11;

    auto expired = validSession();
    expired.expiresAtEpochSeconds = 900;
    transport.exchangeResult = {IdentityClientState::Ready, expired, {}};
    if (client.SignIn("code", 1000).state != IdentityClientState::Error) return 12;

    const auto valid = validSession();
    transport.exchangeResult = {IdentityClientState::Ready, valid, {}};
    const auto signedIn = client.SignIn("code", 1000);
    if (signedIn.state != IdentityClientState::Ready || !signedIn.session ||
        signedIn.session->accessToken != "access-token") return 13;

    auto changedAccount = validSession("acct-2");
    changedAccount.sessionId = "session-2";
    changedAccount.accessToken = "access-2";
    changedAccount.refreshHandle = "refresh-2";
    transport.refreshResult = {IdentityClientState::Ready, changedAccount, {}};
    if (client.RefreshSession(valid, 1000).state != IdentityClientState::Error) return 14;

    auto refreshed = valid;
    refreshed.sessionId = "session-2";
    refreshed.accessToken = "access-2";
    refreshed.refreshHandle = "refresh-2";
    refreshed.expiresAtEpochSeconds = 3000;
    transport.refreshResult = {IdentityClientState::Ready, refreshed, {}};
    const auto refreshResult = client.RefreshSession(valid, 1000);
    if (refreshResult.state != IdentityClientState::Ready || !refreshResult.session ||
        refreshResult.session->sessionId != "session-2") return 15;

    std::string error;
    if (!client.SignOut(refreshed, error) || transport.revokedSession != "session-2") return 16;

    std::cout << "ZERO V5 Zero ID + Session authority acceptance: PASS\n";
    return 0;
}
