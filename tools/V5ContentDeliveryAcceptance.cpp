#include "v5/ContentDeliveryDomain.h"
#include <iostream>
#include <string_view>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

class ReadyTransport final : public zero::v5::IContentDeliveryTransport {
public:
    zero::v5::ContentTransferResult RequestTransfer(const std::string& contentId,
                                                     const std::string& version) override {
        zero::v5::ContentTransferTicket ticket;
        ticket.contentId = contentId;
        ticket.packageId = "zero.game.delivery";
        ticket.version = version;
        ticket.httpsUrl = "https://content.zero.test/packages/game.pkg";
        ticket.sha256Hex = std::string(64, 'a');
        ticket.expectedBytes = 4096;
        ticket.expiresAtEpochSeconds = 2000;
        ticket.authority = zero::v5::AuthoritySource::ZeroService;
        return {zero::v5::ContentDeliveryClientState::Ready, ticket, {}};
    }
};

class ForgedTransport final : public zero::v5::IContentDeliveryTransport {
public:
    zero::v5::ContentTransferResult RequestTransfer(const std::string& contentId,
                                                     const std::string& version) override {
        zero::v5::ContentTransferTicket ticket;
        ticket.contentId = contentId;
        ticket.packageId = "zero.game.delivery";
        ticket.version = version;
        ticket.httpsUrl = "http://insecure.example/package.pkg";
        ticket.sha256Hex = std::string(64, 'b');
        ticket.expectedBytes = 4096;
        ticket.expiresAtEpochSeconds = 2000;
        ticket.authority = zero::v5::AuthoritySource::LocalPackage;
        return {zero::v5::ContentDeliveryClientState::Ready, ticket, {}};
    }
};
}

int main() {
    using namespace zero::v5;
    bool ok = true;

    DisconnectedContentDeliveryTransport disconnected;
    ContentDeliveryClient disconnectedClient(disconnected);
    const auto disconnectedResult = disconnectedClient.Begin("content.game", "1.0.0", 1000);
    ok &= expect(disconnectedResult.state == ContentDeliveryClientState::Disconnected,
        "missing CDN transport must fail closed as disconnected");
    ok &= expect(!disconnectedResult.ticket.has_value(),
        "disconnected content delivery must never synthesize a transfer ticket");

    ReadyTransport ready;
    ContentDeliveryClient readyClient(ready);
    const auto readyResult = readyClient.Begin("content.game", "1.0.0", 1000);
    ok &= expect(readyResult.state == ContentDeliveryClientState::Ready,
        "authoritative HTTPS ticket should be accepted");
    ok &= expect(readyResult.ticket.has_value(), "ready delivery must retain the validated ticket");

    ForgedTransport forged;
    ContentDeliveryClient forgedClient(forged);
    const auto forgedResult = forgedClient.Begin("content.game", "1.0.0", 1000);
    ok &= expect(forgedResult.state == ContentDeliveryClientState::Error,
        "local or insecure transfer authority must be rejected");
    ok &= expect(!forgedResult.ticket.has_value(),
        "rejected delivery must clear the untrusted ticket");

    ContentTransferTicket expired;
    expired.contentId = "content.game";
    expired.packageId = "zero.game.delivery";
    expired.version = "1.0.0";
    expired.httpsUrl = "https://content.zero.test/game.pkg";
    expired.sha256Hex = std::string(64, 'c');
    expired.expectedBytes = 1;
    expired.expiresAtEpochSeconds = 999;
    expired.authority = AuthoritySource::ZeroService;
    std::string error;
    ok &= expect(!ContentDeliveryClient::ValidateAuthoritativeTicket(expired, "content.game", "1.0.0", 1000, error),
        "expired transfer tickets must fail closed");

    if (!ok) return 1;
    std::cout << "ZERO V5 content delivery client acceptance: PASS\n";
    return 0;
}
