#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>

namespace zero::v5 {

enum class IdentityClientState : std::uint8_t { Disconnected, Ready, Error };

struct ZeroIdSession {
    std::string accountId;
    std::string sessionId;
    std::string displayName;
    std::string accessToken;
    std::string refreshHandle;
    std::uint64_t expiresAtEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

struct ZeroIdResult {
    IdentityClientState state{IdentityClientState::Disconnected};
    std::optional<ZeroIdSession> session;
    std::string error;
};

class IZeroIdTransport {
public:
    virtual ~IZeroIdTransport() = default;
    virtual ZeroIdResult ExchangeAuthorizationCode(const std::string& code) = 0;
    virtual ZeroIdResult Refresh(const std::string& refreshHandle) = 0;
    virtual bool Revoke(const std::string& sessionId, std::string& error) = 0;
};

class DisconnectedZeroIdTransport final : public IZeroIdTransport {
public:
    ZeroIdResult ExchangeAuthorizationCode(const std::string&) override;
    ZeroIdResult Refresh(const std::string&) override;
    bool Revoke(const std::string&, std::string& error) override;
};

class ZeroIdAuthorityClient {
public:
    explicit ZeroIdAuthorityClient(IZeroIdTransport& transport) : transport_(transport) {}

    ZeroIdResult SignIn(const std::string& authorizationCode, std::uint64_t nowEpochSeconds);
    ZeroIdResult RefreshSession(const ZeroIdSession& current, std::uint64_t nowEpochSeconds);
    bool SignOut(const ZeroIdSession& current, std::string& error);

    static bool ValidAuthoritativeSession(const ZeroIdSession& session,
                                          std::uint64_t nowEpochSeconds,
                                          std::string& error);

private:
    IZeroIdTransport& transport_;
};

} // namespace zero::v5
