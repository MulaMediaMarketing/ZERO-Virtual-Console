#pragma once
#include <filesystem>
#include <string>

namespace zero {

enum class PackageTrustState {
    Unsigned,
    SignaturePresentUnverified,
    TrustedPublisher,
    UntrustedPublisher,
    InvalidSignatureEnvelope,
    InvalidSignature,
    UnsupportedAlgorithm
};

enum class PackageTrustPolicy {
    AllowLocalUnsigned,
    RequireTrustedPublisher
};

struct PackageSignatureEnvelope {
    int schema{0};
    std::string publisherId;
    std::string keyId;
    std::string algorithm;
    std::string payload;
    std::string signatureBase64;
};

struct PackageTrustResult {
    PackageTrustState state{PackageTrustState::Unsigned};
    std::string publisherId;
    std::string keyId;
    std::string algorithm;
    bool signaturePresent{false};
    bool identityVerified{false};
    bool launchAllowed{false};
    std::wstring detail;
};

class IPublisherTrustProvider {
public:
    virtual ~IPublisherTrustProvider() = default;
    virtual PackageTrustResult Verify(const PackageSignatureEnvelope& envelope,
                                      const std::string& signedPayload) const = 0;
};

// MVP provider: deliberately performs no publisher authentication. It preserves
// the provider boundary so a future ZERO Store/PKI provider can be substituted
// without changing package parsing, shell UX, or RuntimeV4 launch policy.
class DisconnectedPublisherTrustProvider final : public IPublisherTrustProvider {
public:
    PackageTrustResult Verify(const PackageSignatureEnvelope& envelope,
                              const std::string& signedPayload) const override;
};

class PackageTrustService {
public:
    static PackageTrustResult Evaluate(const std::filesystem::path& packageRoot,
                                       PackageTrustPolicy policy,
                                       const IPublisherTrustProvider& provider);
};

} // namespace zero
