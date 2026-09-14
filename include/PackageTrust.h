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

class DisconnectedPublisherTrustProvider final : public IPublisherTrustProvider {
public:
    PackageTrustResult Verify(const PackageSignatureEnvelope& envelope,
                              const std::string& signedPayload) const override;
};

// Verifies ECDSA P-256 / SHA-256 signatures with public keys provisioned under
// %LOCALAPPDATA%\ZERO\Trust\Publishers. The client contains public keys only;
// publisher private signing keys never belong in ZERO.
class CngPublisherTrustProvider final : public IPublisherTrustProvider {
public:
    explicit CngPublisherTrustProvider(std::filesystem::path trustRoot = {});

    PackageTrustResult Verify(const PackageSignatureEnvelope& envelope,
                              const std::string& signedPayload) const override;

    const std::filesystem::path& TrustRoot() const noexcept { return trustRoot_; }
    static std::filesystem::path DefaultTrustRoot();

private:
    std::filesystem::path trustRoot_;
};

class PackageTrustService {
public:
    static PackageTrustResult Evaluate(const std::filesystem::path& packageRoot,
                                       PackageTrustPolicy policy,
                                       const IPublisherTrustProvider& provider);
};

} // namespace zero
