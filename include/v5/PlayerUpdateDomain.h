#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>

namespace zero::v5 {

enum class PlayerUpdateClientState : std::uint8_t {
    Disconnected,
    UpToDate,
    UpdateAvailable,
    Error
};

enum class PlayerUpdateChannel : std::uint8_t {
    Stable,
    Beta,
    Development
};

struct PlayerUpdateManifest {
    std::string version;
    std::string httpsUrl;
    std::string sha256Hex;
    std::uint64_t expectedBytes{0};
    bool mandatory{false};
    PlayerUpdateChannel channel{PlayerUpdateChannel::Stable};
    AuthoritySource authority{AuthoritySource::None};
};

struct PlayerUpdateResult {
    PlayerUpdateClientState state{PlayerUpdateClientState::Disconnected};
    std::optional<PlayerUpdateManifest> manifest;
    std::string error;
};

class IPlayerUpdateTransport {
public:
    virtual ~IPlayerUpdateTransport() = default;
    virtual PlayerUpdateResult Check(const std::string& currentVersion,
                                     PlayerUpdateChannel channel) = 0;
};

class DisconnectedPlayerUpdateTransport final : public IPlayerUpdateTransport {
public:
    PlayerUpdateResult Check(const std::string& currentVersion,
                             PlayerUpdateChannel channel) override;
};

class PlayerUpdateClient final {
public:
    explicit PlayerUpdateClient(IPlayerUpdateTransport& transport) noexcept
        : transport_(transport) {}

    PlayerUpdateResult Check(const std::string& currentVersion,
                             PlayerUpdateChannel channel) const;

    static bool ValidateManifest(const PlayerUpdateManifest& manifest,
                                 PlayerUpdateChannel requestedChannel,
                                 std::string& error) noexcept;

private:
    IPlayerUpdateTransport& transport_;
};

} // namespace zero::v5
