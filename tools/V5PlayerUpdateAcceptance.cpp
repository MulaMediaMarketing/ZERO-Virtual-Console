#include "v5/PlayerUpdateDomain.h"
#include <iostream>
#include <string_view>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

class ReadyUpdateTransport final : public zero::v5::IPlayerUpdateTransport {
public:
    zero::v5::PlayerUpdateResult Check(const std::string&,
                                       zero::v5::PlayerUpdateChannel channel) override {
        zero::v5::PlayerUpdateManifest manifest;
        manifest.version = "5.1.0";
        manifest.httpsUrl = "https://updates.zero.test/zero-player-5.1.0.zip";
        manifest.sha256Hex = std::string(64, 'd');
        manifest.expectedBytes = 1024;
        manifest.mandatory = false;
        manifest.channel = channel;
        manifest.authority = zero::v5::AuthoritySource::ZeroService;
        return {zero::v5::PlayerUpdateClientState::UpdateAvailable, manifest, {}};
    }
};

class ForgedUpdateTransport final : public zero::v5::IPlayerUpdateTransport {
public:
    zero::v5::PlayerUpdateResult Check(const std::string&,
                                       zero::v5::PlayerUpdateChannel channel) override {
        zero::v5::PlayerUpdateManifest manifest;
        manifest.version = "999.0.0";
        manifest.httpsUrl = "http://untrusted.example/update.zip";
        manifest.sha256Hex = std::string(64, 'e');
        manifest.expectedBytes = 1024;
        manifest.channel = channel;
        manifest.authority = zero::v5::AuthoritySource::LocalPackage;
        return {zero::v5::PlayerUpdateClientState::UpdateAvailable, manifest, {}};
    }
};
}

int main() {
    using namespace zero::v5;
    bool ok = true;

    DisconnectedPlayerUpdateTransport disconnected;
    PlayerUpdateClient disconnectedClient(disconnected);
    const auto disconnectedResult = disconnectedClient.Check("5.0.0", PlayerUpdateChannel::Stable);
    ok &= expect(disconnectedResult.state == PlayerUpdateClientState::Disconnected,
        "missing update provider must fail closed as disconnected");
    ok &= expect(!disconnectedResult.manifest.has_value(),
        "disconnected updater must not synthesize a release manifest");

    ReadyUpdateTransport ready;
    PlayerUpdateClient readyClient(ready);
    const auto readyResult = readyClient.Check("5.0.0", PlayerUpdateChannel::Stable);
    ok &= expect(readyResult.state == PlayerUpdateClientState::UpdateAvailable,
        "authoritative update manifest must be accepted");
    ok &= expect(readyResult.manifest.has_value(), "validated update must retain its manifest");

    ForgedUpdateTransport forged;
    PlayerUpdateClient forgedClient(forged);
    const auto forgedResult = forgedClient.Check("5.0.0", PlayerUpdateChannel::Stable);
    ok &= expect(forgedResult.state == PlayerUpdateClientState::Error,
        "local or insecure update manifests must be rejected");
    ok &= expect(!forgedResult.manifest.has_value(),
        "rejected updater response must clear the untrusted manifest");

    PlayerUpdateManifest wrongChannel;
    wrongChannel.version = "5.2.0";
    wrongChannel.httpsUrl = "https://updates.zero.test/5.2.0.zip";
    wrongChannel.sha256Hex = std::string(64, 'f');
    wrongChannel.expectedBytes = 1;
    wrongChannel.channel = PlayerUpdateChannel::Beta;
    wrongChannel.authority = AuthoritySource::ZeroService;
    std::string error;
    ok &= expect(!PlayerUpdateClient::ValidateManifest(wrongChannel, PlayerUpdateChannel::Stable, error),
        "update channel substitution must fail closed");

    if (!ok) return 1;
    std::cout << "ZERO V5 Player updater acceptance: PASS\n";
    return 0;
}
