#include "PackageTrust.h"
#include "StrictJson.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace zero {
namespace {

std::string readAllBounded(const std::filesystem::path& path, size_t maxBytes) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size > maxBytes) return {};
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

bool safeIdentifier(const std::string& value, size_t maxLength) {
    if (value.empty() || value.size() > maxLength) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == ':';
    });
}

bool parseEnvelope(const std::filesystem::path& path, PackageSignatureEnvelope& envelope) {
    const auto text = readAllBounded(path, 64 * 1024);
    if (text.empty()) return false;
    const auto parsed = json::Parse(text);
    const auto* object = parsed.ok ? parsed.root.AsObject() : nullptr;
    if (!object) return false;

    const auto schema = json::Integer(*object, "schema");
    const auto* publisherId = json::String(*object, "publisher_id");
    const auto* keyId = json::String(*object, "key_id");
    const auto* algorithm = json::String(*object, "algorithm");
    const auto* payload = json::String(*object, "payload");
    const auto* signature = json::String(*object, "signature");
    if (!schema || !publisherId || !keyId || !algorithm || !payload || !signature) return false;

    envelope.schema = static_cast<int>(*schema);
    envelope.publisherId = *publisherId;
    envelope.keyId = *keyId;
    envelope.algorithm = *algorithm;
    envelope.payload = *payload;
    envelope.signatureBase64 = *signature;
    return envelope.schema == 1 &&
           safeIdentifier(envelope.publisherId, 160) &&
           safeIdentifier(envelope.keyId, 160) &&
           safeIdentifier(envelope.algorithm, 64) &&
           envelope.payload == "zero.integrity.sha256" &&
           !envelope.signatureBase64.empty() && envelope.signatureBase64.size() <= 16384;
}

std::string integrityPayload(const std::filesystem::path& packageRoot) {
    return readAllBounded(packageRoot / L"zero.integrity.sha256", 8 * 1024 * 1024);
}

} // namespace

PackageTrustResult DisconnectedPublisherTrustProvider::Verify(const PackageSignatureEnvelope& envelope,
                                                              const std::string& signedPayload) const {
    PackageTrustResult result;
    result.signaturePresent = true;
    result.publisherId = envelope.publisherId;
    result.keyId = envelope.keyId;
    result.algorithm = envelope.algorithm;
    result.state = PackageTrustState::SignaturePresentUnverified;
    result.identityVerified = false;
    result.detail = L"A package signature is present, but ZERO has no connected publisher trust provider to authenticate it.";
    (void)signedPayload;
    return result;
}

PackageTrustResult PackageTrustService::Evaluate(const std::filesystem::path& packageRoot,
                                                 PackageTrustPolicy policy,
                                                 const IPublisherTrustProvider& provider) {
    PackageTrustResult result;
    const auto signaturePath = packageRoot / L"zero.signature.json";
    std::error_code ec;
    const bool signatureExists = std::filesystem::exists(signaturePath, ec) &&
                                 std::filesystem::is_regular_file(signaturePath, ec);

    if (!signatureExists) {
        result.state = PackageTrustState::Unsigned;
        result.signaturePresent = false;
        result.identityVerified = false;
        result.launchAllowed = policy == PackageTrustPolicy::AllowLocalUnsigned;
        result.detail = result.launchAllowed
            ? L"Local package is unsigned. Integrity is enforced, but publisher identity is not verified."
            : L"Launch requires a package signed by a trusted publisher.";
        return result;
    }

    PackageSignatureEnvelope envelope;
    if (!parseEnvelope(signaturePath, envelope)) {
        result.state = PackageTrustState::InvalidSignatureEnvelope;
        result.signaturePresent = true;
        result.launchAllowed = false;
        result.detail = L"The package signature envelope is malformed, unsupported, duplicated, or does not target zero.integrity.sha256.";
        return result;
    }

    const auto payload = integrityPayload(packageRoot);
    if (payload.empty()) {
        result.state = PackageTrustState::InvalidSignature;
        result.signaturePresent = true;
        result.publisherId = envelope.publisherId;
        result.keyId = envelope.keyId;
        result.algorithm = envelope.algorithm;
        result.launchAllowed = false;
        result.detail = L"ZERO could not evaluate package trust because the integrity payload is unavailable.";
        return result;
    }

    auto verified = provider.Verify(envelope, payload);
    verified.signaturePresent = true;
    if (verified.publisherId.empty()) verified.publisherId = envelope.publisherId;
    if (verified.keyId.empty()) verified.keyId = envelope.keyId;
    if (verified.algorithm.empty()) verified.algorithm = envelope.algorithm;

    if (policy == PackageTrustPolicy::RequireTrustedPublisher) {
        verified.launchAllowed = verified.state == PackageTrustState::TrustedPublisher && verified.identityVerified;
        if (!verified.launchAllowed && verified.detail.empty())
            verified.detail = L"Launch requires a trusted publisher signature.";
    } else {
        verified.launchAllowed = verified.state != PackageTrustState::InvalidSignatureEnvelope &&
                                 verified.state != PackageTrustState::InvalidSignature &&
                                 verified.state != PackageTrustState::UnsupportedAlgorithm;
    }
    return verified;
}

} // namespace zero
