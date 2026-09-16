#include "v5/PlayerUpdateDomain.h"
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

PlayerUpdateResult Invalid(std::string error) {
    PlayerUpdateResult result;
    result.state = PlayerUpdateClientState::Error;
    result.error = std::move(error);
    return result;
}

} // namespace

PlayerUpdateResult DisconnectedPlayerUpdateTransport::Check(const std::string&,
                                                            PlayerUpdateChannel) {
    PlayerUpdateResult result;
    result.state = PlayerUpdateClientState::Disconnected;
    result.error = "ZERO Player update service is not connected.";
    return result;
}

PlayerUpdateResult PlayerUpdateClient::Check(const std::string& currentVersion,
                                             PlayerUpdateChannel channel) const {
    if (currentVersion.empty()) return Invalid("Current ZERO Player version is required.");

    auto result = transport_.Check(currentVersion, channel);
    if (result.state == PlayerUpdateClientState::Disconnected ||
        result.state == PlayerUpdateClientState::Error) {
        result.manifest.reset();
        if (result.error.empty()) {
            result.error = result.state == PlayerUpdateClientState::Disconnected
                ? "ZERO Player update service is not connected."
                : "ZERO Player update service returned an error.";
        }
        return result;
    }

    if (result.state == PlayerUpdateClientState::UpToDate) {
        result.manifest.reset();
        result.error.clear();
        return result;
    }

    if (result.state != PlayerUpdateClientState::UpdateAvailable || !result.manifest)
        return Invalid("Update service returned an inconsistent update state.");

    std::string validationError;
    if (!ValidateManifest(*result.manifest, channel, validationError))
        return Invalid(validationError);

    result.error.clear();
    return result;
}

bool PlayerUpdateClient::ValidateManifest(const PlayerUpdateManifest& manifest,
                                          PlayerUpdateChannel requestedChannel,
                                          std::string& error) noexcept {
    if (manifest.authority != AuthoritySource::ZeroService) {
        error = "ZERO Player updates require ZERO service authority.";
        return false;
    }
    if (manifest.version.empty()) {
        error = "Update manifest is missing a version.";
        return false;
    }
    if (manifest.channel != requestedChannel) {
        error = "Update manifest channel does not match the requested channel.";
        return false;
    }
    if (!IsHttpsUrl(manifest.httpsUrl)) {
        error = "Update manifest must use HTTPS.";
        return false;
    }
    if (!IsSha256Hex(manifest.sha256Hex)) {
        error = "Update manifest must provide a valid SHA-256 digest.";
        return false;
    }
    if (manifest.expectedBytes == 0) {
        error = "Update manifest must provide a non-zero payload size.";
        return false;
    }
    error.clear();
    return true;
}

} // namespace zero::v5
