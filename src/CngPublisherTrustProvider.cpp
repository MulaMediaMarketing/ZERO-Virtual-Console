#include "PackageTrust.h"
#include "PlatformPaths.h"
#include "StrictJson.h"
#include "Utf8Path.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

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

bool safeIdentifier(const std::string& value) {
    return !value.empty() && value.size() <= 160 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == ':';
    });
}

bool hexToBytes(const std::string& text, std::vector<unsigned char>& out) {
    if (text.size() % 2 != 0) return false;
    out.clear();
    out.reserve(text.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < text.size(); i += 2) {
        const int hi = nibble(text[i]);
        const int lo = nibble(text[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return true;
}

int base64Value(char c) noexcept {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

bool base64Decode(const std::string& text, std::vector<unsigned char>& out) {
    std::string clean;
    clean.reserve(text.size());
    for (unsigned char c : text) {
        if (std::isspace(c)) continue;
        clean.push_back(static_cast<char>(c));
    }
    if (clean.empty() || clean.size() % 4 != 0) return false;

    out.clear();
    out.reserve((clean.size() / 4) * 3);
    for (size_t i = 0; i < clean.size(); i += 4) {
        const bool last = i + 4 == clean.size();
        const char c0 = clean[i];
        const char c1 = clean[i + 1];
        const char c2 = clean[i + 2];
        const char c3 = clean[i + 3];
        const int a = base64Value(c0);
        const int b = base64Value(c1);
        if (a < 0 || b < 0) return false;

        if (c2 == '=') {
            if (!last || c3 != '=' || (b & 0x0F) != 0) return false;
            out.push_back(static_cast<unsigned char>((static_cast<uint32_t>(a) << 2) | (static_cast<uint32_t>(b) >> 4)));
            continue;
        }

        const int c = base64Value(c2);
        if (c < 0) return false;
        out.push_back(static_cast<unsigned char>((static_cast<uint32_t>(a) << 2) | (static_cast<uint32_t>(b) >> 4)));
        out.push_back(static_cast<unsigned char>(((static_cast<uint32_t>(b) & 0x0F) << 4) | (static_cast<uint32_t>(c) >> 2)));

        if (c3 == '=') {
            if (!last || (c & 0x03) != 0) return false;
            continue;
        }
        const int d = base64Value(c3);
        if (d < 0) return false;
        out.push_back(static_cast<unsigned char>(((static_cast<uint32_t>(c) & 0x03) << 6) | static_cast<uint32_t>(d)));
    }
    return !out.empty();
}

bool sha256(const std::string& payload, std::vector<unsigned char>& digest) {
    if (payload.size() > static_cast<size_t>(std::numeric_limits<ULONG>::max())) return false;
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectBytes = 0;
    DWORD hashBytes = 0;
    DWORD cb = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    auto closeAlg = [&] { if (alg) BCryptCloseAlgorithmProvider(alg, 0); };
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes), &cb, 0) != 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes), &cb, 0) != 0) {
        closeAlg();
        return false;
    }
    std::vector<unsigned char> object(objectBytes);
    digest.resize(hashBytes);
    if (BCryptCreateHash(alg, &hash, object.data(), objectBytes, nullptr, 0, 0) != 0) {
        closeAlg();
        return false;
    }
    const NTSTATUS update = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(payload.data())), static_cast<ULONG>(payload.size()), 0);
    const NTSTATUS finish = update == 0 ? BCryptFinishHash(hash, digest.data(), hashBytes, 0) : update;
    BCryptDestroyHash(hash);
    closeAlg();
    return finish == 0;
}

std::filesystem::path keyPath(const std::filesystem::path& root, const std::string& publisherId, const std::string& keyId) {
    const auto relative = PathFromUtf8(publisherId + "__" + keyId + ".json");
    return relative ? root / *relative : std::filesystem::path{};
}

struct TrustedKeyRecord { std::string publisherId; std::string keyId; std::string algorithm; std::string x; std::string y; };

bool parseTrustedKey(const std::string& text, TrustedKeyRecord& record) {
    const auto parsed = json::Parse(text);
    const auto* object = parsed.ok ? parsed.root.AsObject() : nullptr;
    if (!object) return false;
    const auto* publisherId = json::String(*object, "publisher_id");
    const auto* keyId = json::String(*object, "key_id");
    const auto* algorithm = json::String(*object, "algorithm");
    const auto* x = json::String(*object, "x");
    const auto* y = json::String(*object, "y");
    if (!publisherId || !keyId || !algorithm || !x || !y) return false;
    if (!safeIdentifier(*publisherId) || !safeIdentifier(*keyId) || algorithm->size() > 64 || x->size() > 128 || y->size() > 128) return false;
    record = {*publisherId, *keyId, *algorithm, *x, *y};
    return true;
}

} // namespace

CngPublisherTrustProvider::CngPublisherTrustProvider(std::filesystem::path trustRoot)
    : trustRoot_(trustRoot.empty() ? DefaultTrustRoot() : std::move(trustRoot)) {}

std::filesystem::path CngPublisherTrustProvider::DefaultTrustRoot() { return PlatformPaths::PublisherTrustRoot(); }

PackageTrustResult CngPublisherTrustProvider::Verify(const PackageSignatureEnvelope& envelope, const std::string& signedPayload) const {
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
    if (keyFile.empty()) {
        result.state = PackageTrustState::InvalidSignatureEnvelope;
        result.detail = L"ZERO could not resolve the trusted publisher key path.";
        return result;
    }
    const auto text = readAllBounded(keyFile, 64 * 1024);
    if (text.empty()) {
        result.state = PackageTrustState::UntrustedPublisher;
        result.detail = L"The package is signed, but its publisher key is not trusted on this ZERO installation.";
        return result;
    }
    TrustedKeyRecord record;
    if (!parseTrustedKey(text, record) || record.publisherId != envelope.publisherId || record.keyId != envelope.keyId || record.algorithm != envelope.algorithm) {
        result.state = PackageTrustState::UntrustedPublisher;
        result.detail = L"The trusted publisher key record is malformed, duplicated, or does not match the package signer.";
        return result;
    }

    std::vector<unsigned char> x,y,signature,digest;
    if (!hexToBytes(record.x,x) || !hexToBytes(record.y,y) || x.size()!=32 || y.size()!=32 || !base64Decode(envelope.signatureBase64,signature) || signature.size()!=64 || !sha256(signedPayload,digest)) {
        result.state = PackageTrustState::InvalidSignature;
        result.detail = L"ZERO could not decode or validate the publisher signature material.";
        return result;
    }

    struct PublicBlob { BCRYPT_ECCKEY_BLOB header; unsigned char data[64]; } blob{};
    blob.header.dwMagic=BCRYPT_ECDSA_PUBLIC_P256_MAGIC; blob.header.cbKey=32;
    std::copy(x.begin(),x.end(),blob.data); std::copy(y.begin(),y.end(),blob.data+32);
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_KEY_HANDLE key=nullptr;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)!=0 || BCryptImportKeyPair(alg,nullptr,BCRYPT_ECCPUBLIC_BLOB,&key,reinterpret_cast<PUCHAR>(&blob),sizeof(blob),0)!=0){
        if(key)BCryptDestroyKey(key);if(alg)BCryptCloseAlgorithmProvider(alg,0);
        result.state=PackageTrustState::InvalidSignature;result.detail=L"ZERO could not import the trusted publisher public key.";return result;
    }
    const NTSTATUS verified=BCryptVerifySignature(key,nullptr,digest.data(),static_cast<ULONG>(digest.size()),signature.data(),static_cast<ULONG>(signature.size()),0);
    BCryptDestroyKey(key);BCryptCloseAlgorithmProvider(alg,0);
    if(verified!=0){result.state=PackageTrustState::InvalidSignature;result.detail=L"The package publisher signature is invalid.";return result;}
    result.state=PackageTrustState::TrustedPublisher;result.identityVerified=true;result.detail=L"Publisher signature verified with a trusted ZERO public key.";return result;
}

} // namespace zero