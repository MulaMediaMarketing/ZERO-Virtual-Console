#include "PackageTrust.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace zero {
namespace {

std::string readAll(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

std::string jsonString(const std::string& text, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    const auto keyPos = text.find(token);
    if (keyPos == std::string::npos) return {};
    const auto colon = text.find(':', keyPos + token.size());
    if (colon == std::string::npos) return {};
    const auto quoteStart = text.find('"', colon + 1);
    if (quoteStart == std::string::npos) return {};
    const auto quoteEnd = text.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) return {};
    return text.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

int jsonInt(const std::string& text, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    const auto keyPos = text.find(token);
    if (keyPos == std::string::npos) return 0;
    const auto colon = text.find(':', keyPos + token.size());
    if (colon == std::string::npos) return 0;
    auto start = colon + 1;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
    auto end = start;
    while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) ++end;
    if (start == end) return 0;
    try { return std::stoi(text.substr(start, end - start)); } catch (...) { return 0; }
}

bool safeIdentifier(const std::string& value, size_t maxLength) {
    if (value.empty() || value.size() > maxLength) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == ':';
    });
}

bool parseEnvelope(const std::filesystem::path& path, PackageSignatureEnvelope& envelope) {
    const auto text = readAll(path);
    if (text.empty() || text.size() > 64 * 1024) return false;
    envelope.schema = jsonInt(text, "schema");
    envelope.publisherId = jsonString(text, "publisher_id");
    envelope.keyId = jsonString(text, "key_id");
    envelope.algorithm = jsonString(text, "algorithm");
    envelope.payload = jsonString(text, "payload");
    envelope.signatureBase64 = jsonString(text, "signature");
    return envelope.schema == 1 &&
           safeIdentifier(envelope.publisherId, 160) &&
           safeIdentifier(envelope.keyId, 160) &&
           safeIdentifier(envelope.algorithm, 64) &&
           envelope.payload == "zero.integrity.sha256" &&
           !envelope.signatureBase64.empty() && envelope.signatureBase64.size() <= 16384;
}

std::string integrityPayload(const std::filesystem::path& packageRoot) {
    return readAll(packageRoot / L"zero.integrity.sha256");
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
        result.detail = L"The package signature envelope is malformed, unsupported, or does not target zero.integrity.sha256.";
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
