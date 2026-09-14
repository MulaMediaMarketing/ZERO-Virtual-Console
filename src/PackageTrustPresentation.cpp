#include "PackageTrustPresentation.h"

namespace zero {

PackageTrustPresentation PresentPackageTrust(const PackageTrustResult& trust) {
    PackageTrustPresentation out;
    switch (trust.state) {
        case PackageTrustState::Unsigned:
            out.shortLabel = L"Unsigned";
            out.title = L"Local package";
            out.body = L"ZERO verified the installed files, but this game does not have a publisher signature.";
            break;
        case PackageTrustState::SignaturePresentUnverified:
            out.shortLabel = L"Signed · Unverified";
            out.title = L"Publisher not verified";
            out.body = L"A publisher signature is present, but ZERO could not match it to a trusted publisher key.";
            break;
        case PackageTrustState::TrustedPublisher:
            out.shortLabel = L"Trusted Publisher";
            out.title = L"Publisher verified";
            out.body = trust.publisherId.empty()
                ? L"ZERO verified this package against a trusted publisher key."
                : L"ZERO verified this package was signed by " + std::wstring(trust.publisherId.begin(), trust.publisherId.end()) + L".";
            out.verified = true;
            break;
        case PackageTrustState::UntrustedPublisher:
            out.shortLabel = L"Publisher Not Trusted";
            out.title = L"Publisher key not trusted";
            out.body = L"The package is signed, but the signing key is not in ZERO's trusted publisher store.";
            out.blocked = !trust.launchAllowed;
            break;
        case PackageTrustState::InvalidSignatureEnvelope:
            out.shortLabel = L"Blocked · Invalid Signature";
            out.title = L"Signature information is invalid";
            out.body = L"ZERO found malformed or unsupported publisher-signature information and blocked this package.";
            out.blocked = true;
            break;
        case PackageTrustState::InvalidSignature:
            out.shortLabel = L"Blocked · Signature Failed";
            out.title = L"Publisher verification failed";
            out.body = L"The publisher signature does not match this package. ZERO blocked launch to protect the installed game.";
            out.blocked = true;
            break;
        case PackageTrustState::UnsupportedAlgorithm:
            out.shortLabel = L"Blocked · Unsupported Signature";
            out.title = L"Signature type not supported";
            out.body = L"This package uses a publisher-signature algorithm that this ZERO build cannot verify.";
            out.blocked = true;
            break;
    }
    if (!trust.launchAllowed && !out.blocked) out.blocked = true;
    return out;
}

} // namespace zero
