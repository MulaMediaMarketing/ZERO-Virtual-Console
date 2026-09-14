#include "PackageTrust.h"
#include "PlatformPaths.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

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
    const auto q1 = text.find('"', colon + 1);
    if (q1 == std::string::npos) return {};
    const auto q2 = text.find('"', q1 + 1);
    if (q2 == std::string::npos) return {};
    return text.substr(q1 + 1, q2 - q1 - 1);
}

bool safeIdentifier(const std::string& value) {
    return !value.empty() && value.size() <= 160 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == ':';
    });
}

bool hexToBytes(const std::string& text, std::vector<unsigned char>& out) {
    if (text.size() % 2 != 0) return false;
    out.clear(); out.reserve(text.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < text.size(); i += 2) {
        const int hi = nibble(text[i]), lo = nibble(text[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return true;
}

bool base64Decode(const std::string& text, std::vector<unsigned char>& out) {
    static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    out.clear();
    int val = 0, bits = -8;
    for (unsigned char c : text) {
        if (std::isspace(c)) continue;
        if (c == '=') break;
        const auto p = alphabet.find(static_cast<char>(c));
        if (p == std::string::npos) return false;
        val = (val << 6) + static_cast<int>(p);
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<unsigned char>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return !out.empty();
}

bool sha256(const std::string& payload, std::vector<unsigned char>& digest) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectBytes = 0, hashBytes = 0, cb = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    auto closeAlg = [&] { if (alg) BCryptCloseAlgorithmProvider(alg, 0); };
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes), &cb, 0) != 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes), &cb, 0) != 0) {
        closeAlg(); return false;
    }
    std::vector<unsigned char> object(objectBytes);
    digest.resize(hashBytes);
    if (BCryptCreateHash(alg, &hash, object.data(), objectBytes, nullptr, 0, 0) != 0) { closeAlg(); return false; }
    const NTSTATUS update = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(payload.data())), static_cast<ULONG>(payload.size()), 0);
    const NTSTATUS finish = update == 0 ? BCryptFinishHash(hash, digest.data(), hashBytes, 0) : update;
    BCryptDestroyHash(hash); closeAlg();
    return finish == 0;
}

std::filesystem::path keyPath(const std::filesystem::path& root, const std::string& publisherId, const std::string& keyId) {
    std::wstring name(publisherId.begin(), publisherId.end());
    name += L"__";
    name.append(keyId.begin(), keyId.end());
    name += L".json";
    return root / name;
}

} // namespace

CngPublisherTrustProvider::CngPublisherTrustProvider(std::filesystem::path trustRoot)
    : trustRoot_(trustRoot.empty() ? DefaultTrustRoot() : std::move(trustRoot)) {}

std::filesystem::path CngPublisherTrustProvider::DefaultTrustRoot() {
    return PlatformPaths::PublisherTrustRoot();
}

PackageTrustResult CngPublisherTrustProvider::Verify(const PackageSignatureEnvelope& envelope,
                                                     const std::string& signedPayload) const {
    PackageTrustResult result;
    result.signaturePresent = true;
    result.publisherId = envelope.publisherId;
    result.keyId = envelope.keyId;
    result.algorithm = envelope.algorithm;

    if (envelope.algorithm != "ecdsa-p256-sha256") {
        result.state = PackageTrustState::UnsupportedAlgorithm;
        result.detail = L"ZERO does not support the package signature algorithm.";
        return result;
    }
    if (!safeIdentifier(envelope.publisherId) || !safeIdentifier(envelope.keyId)) {
        result.state = PackageTrustState::InvalidSignatureEnvelope;
        result.detail = L"ZERO rejected an unsafe publisher or key identifier.";
        return result;
    }

    const auto keyFile = keyPath(trustRoot_, envelope.publisherId, envelope.keyId);
    const auto text = readAll(keyFile);
    if (text.empty()) {
        result.state = PackageTrustState::UntrustedPublisher;
        result.detail = L"The package is signed, but its publisher key is not trusted on this ZERO installation.";
        return result;
    }
    if (text.size() > 64 * 1024 || jsonString(text, "publisher_id") != envelope.publisherId ||
        jsonString(text, "key_id") != envelope.keyId || jsonString(text, "algorithm") != envelope.algorithm) {
        result.state = PackageTrustState::UntrustedPublisher;
        result.detail = L"The trusted publisher key record is invalid or does not match the package signer.";
        return result;
    }

    std::vector<unsigned char> x, y, signature, digest;
    if (!hexToBytes(jsonString(text, "x"), x) || !hexToBytes(jsonString(text, "y"), y) ||
        x.size() != 32 || y.size() != 32 || !base64Decode(envelope.signatureBase64, signature) ||
        signature.size() != 64 || !sha256(signedPayload, digest)) {
        result.state = PackageTrustState::InvalidSignature;
        result.detail = L"ZERO could not decode or validate the publisher signature material.";
        return result;
    }

    struct PublicBlob {
        BCRYPT_ECCKEY_BLOB header;
        unsigned char data[64];
    } blob{};
    blob.header.dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    blob.header.cbKey = 32;
    std::copy(x.begin(), x.end(), blob.data);
    std::copy(y.begin(), y.end(), blob.data + 32);

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) != 0 ||
        BCryptImportKeyPair(alg, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key,
                            reinterpret_cast<PUCHAR>(&blob), sizeof(blob), 0) != 0) {
        if (key) BCryptDestroyKey(key);
        if (alg) BCryptCloseAlgorithmProvider(alg, 0);
        result.state = PackageTrustState::InvalidSignature;
        result.detail = L"ZERO could not import the trusted publisher public key.";
        return result;
    }

    const NTSTATUS verified = BCryptVerifySignature(key, nullptr, digest.data(), static_cast<ULONG>(digest.size()),
                                                     signature.data(), static_cast<ULONG>(signature.size()), 0);
    BCryptDestroyKey(key);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (verified != 0) {
        result.state = PackageTrustState::InvalidSignature;
        result.detail = L"The package publisher signature is invalid.";
        return result;
    }

    result.state = PackageTrustState::TrustedPublisher;
    result.identityVerified = true;
    result.detail = L"Publisher signature verified with a trusted ZERO public key.";
    return result;
}

} // namespace zero
