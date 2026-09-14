#include "PackageTrust.h"
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

class TestTrustedProvider final : public zero::IPublisherTrustProvider {
public:
    zero::PackageTrustResult Verify(const zero::PackageSignatureEnvelope& envelope,
                                    const std::string& signedPayload) const override {
        zero::PackageTrustResult result;
        result.publisherId = envelope.publisherId;
        result.keyId = envelope.keyId;
        result.algorithm = envelope.algorithm;
        result.signaturePresent = true;
        if (envelope.publisherId == "zero.test.publisher" &&
            envelope.keyId == "test-key-1" &&
            envelope.algorithm == "ed25519" &&
            signedPayload.find("# ZERO package integrity v1") == 0) {
            result.state = zero::PackageTrustState::TrustedPublisher;
            result.identityVerified = true;
            result.detail = L"Test trust provider accepted the publisher identity.";
        } else {
            result.state = zero::PackageTrustState::InvalidSignature;
            result.identityVerified = false;
            result.detail = L"Test trust provider rejected the signature.";
        }
        return result;
    }
};

std::filesystem::path makeRoot() {
    wchar_t temp[MAX_PATH]{};
    const DWORD count = GetTempPathW(MAX_PATH, temp);
    if (!count || count >= MAX_PATH) return {};
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(temp) / (L"zero-package-trust-" + std::to_wstring(tick));
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file << text;
    file.flush();
    return file.good();
}

void check(bool value, const char* name, bool& allPassed) {
    std::cout << (value ? "[PASS] " : "[FAIL] ") << name << "\n";
    allPassed &= value;
}

} // namespace

int wmain() {
    const auto root = makeRoot();
    if (root.empty()) return 2;
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) return 2;

    bool allPassed = true;
    const std::string integrity =
        "# ZERO package integrity v1\n"
        "# files=1 bytes=4\n"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  4  game.bin\n";
    check(writeText(root / L"zero.integrity.sha256", integrity), "integrity_fixture_written", allPassed);

    zero::DisconnectedPublisherTrustProvider disconnected;
    auto trust = zero::PackageTrustService::Evaluate(root, zero::PackageTrustPolicy::AllowLocalUnsigned, disconnected);
    check(trust.state == zero::PackageTrustState::Unsigned && trust.launchAllowed && !trust.identityVerified,
          "unsigned_local_package_allowed_without_identity_claim", allPassed);

    trust = zero::PackageTrustService::Evaluate(root, zero::PackageTrustPolicy::RequireTrustedPublisher, disconnected);
    check(trust.state == zero::PackageTrustState::Unsigned && !trust.launchAllowed,
          "unsigned_package_blocked_by_strict_policy", allPassed);

    check(writeText(root / L"zero.signature.json", "{\"schema\":1,\"publisher_id\":\"bad publisher\"}"),
          "malformed_signature_fixture_written", allPassed);
    trust = zero::PackageTrustService::Evaluate(root, zero::PackageTrustPolicy::AllowLocalUnsigned, disconnected);
    check(trust.state == zero::PackageTrustState::InvalidSignatureEnvelope && !trust.launchAllowed,
          "malformed_signature_envelope_blocks_launch", allPassed);

    const std::string validEnvelope =
        "{\n"
        "  \"schema\": 1,\n"
        "  \"publisher_id\": \"zero.test.publisher\",\n"
        "  \"key_id\": \"test-key-1\",\n"
        "  \"algorithm\": \"ed25519\",\n"
        "  \"payload\": \"zero.integrity.sha256\",\n"
        "  \"signature\": \"TEST_SIGNATURE_NOT_PRODUCTION_CRYPTO\"\n"
        "}\n";
    check(writeText(root / L"zero.signature.json", validEnvelope), "valid_signature_envelope_fixture_written", allPassed);

    trust = zero::PackageTrustService::Evaluate(root, zero::PackageTrustPolicy::AllowLocalUnsigned, disconnected);
    check(trust.state == zero::PackageTrustState::SignaturePresentUnverified && trust.launchAllowed &&
          !trust.identityVerified && trust.publisherId == "zero.test.publisher",
          "disconnected_provider_never_claims_publisher_verified", allPassed);

    trust = zero::PackageTrustService::Evaluate(root, zero::PackageTrustPolicy::RequireTrustedPublisher, disconnected);
    check(trust.state == zero::PackageTrustState::SignaturePresentUnverified && !trust.launchAllowed,
          "strict_policy_blocks_unverified_signature", allPassed);

    TestTrustedProvider testProvider;
    trust = zero::PackageTrustService::Evaluate(root, zero::PackageTrustPolicy::RequireTrustedPublisher, testProvider);
    check(trust.state == zero::PackageTrustState::TrustedPublisher && trust.identityVerified && trust.launchAllowed,
          "trusted_provider_can_satisfy_strict_policy", allPassed);

    std::filesystem::remove_all(root, ec);
    std::cout << "Result: " << (allPassed ? "PASS" : "FAIL") << "\n";
    return allPassed ? 0 : 2;
}
