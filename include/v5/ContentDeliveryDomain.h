#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <optional>
#include <string>

namespace zero::v5 {

enum class ContentDeliveryClientState : std::uint8_t {
    Disconnected,
    Ready,
    Error
};

struct ContentTransferTicket {
    std::string contentId;
    std::string packageId;
    std::string version;
    std::string httpsUrl;
    std::string sha256Hex;
    std::uint64_t expectedBytes{0};
    std::uint64_t expiresAtEpochSeconds{0};
    AuthoritySource authority{AuthoritySource::None};
};

struct ContentTransferResult {
    ContentDeliveryClientState state{ContentDeliveryClientState::Disconnected};
    std::optional<ContentTransferTicket> ticket;
    std::string error;
};

class IContentDeliveryTransport {
public:
    virtual ~IContentDeliveryTransport() = default;
    virtual ContentTransferResult RequestTransfer(const std::string& contentId,
                                                  const std::string& version) = 0;
};

class DisconnectedContentDeliveryTransport final : public IContentDeliveryTransport {
public:
    ContentTransferResult RequestTransfer(const std::string& contentId,
                                          const std::string& version) override;
};

class ContentDeliveryClient final {
public:
    explicit ContentDeliveryClient(IContentDeliveryTransport& transport) noexcept
        : transport_(transport) {}

    ContentTransferResult Begin(const std::string& contentId,
                                const std::string& version,
                                std::uint64_t nowEpochSeconds) const;

    static bool ValidateAuthoritativeTicket(const ContentTransferTicket& ticket,
                                            const std::string& requestedContentId,
                                            const std::string& requestedVersion,
                                            std::uint64_t nowEpochSeconds,
                                            std::string& error) noexcept;

private:
    IContentDeliveryTransport& transport_;
};

} // namespace zero::v5
