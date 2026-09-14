# ZERO Package Trust Foundation

ZERO separates **package integrity** from **publisher trust**.

## Integrity

`zero.integrity.sha256` records the installed package inventory and SHA-256 digests. Runtime V4.1 verifies this file before launch. Integrity answers:

> Are the installed package files the same files ZERO validated and installed?

Integrity alone does not establish publisher identity.

## Publisher signature envelope

Optional publisher metadata uses `zero.signature.json`:

```json
{
  "schema": 1,
  "publisher_id": "example.publisher",
  "key_id": "publisher-key-1",
  "algorithm": "ecdsa-p256-sha256",
  "payload": "zero.integrity.sha256",
  "signature": "BASE64_RAW_R_S_SIGNATURE"
}
```

The signed payload is the exact byte sequence of `zero.integrity.sha256`. The signature envelope is excluded from the package inventory because it signs the generated integrity manifest.

## Trusted public keys

Runtime V4.1 uses Windows CNG/BCrypt and locally trusted ECDSA P-256 public keys. A trusted-key record is resolved from the ZERO trust root using the publisher/key identifiers and must match the package envelope.

The key record contains:

- matching `publisher_id`;
- matching `key_id`;
- `algorithm` = `ecdsa-p256-sha256`;
- 32-byte P-256 public-key X coordinate encoded as hexadecimal;
- 32-byte P-256 public-key Y coordinate encoded as hexadecimal.

The package signature decodes to the 64-byte raw ECDSA `r || s` representation expected by the current Windows CNG verification path. Runtime V4.1 SHA-256 hashes the signed payload and verifies the signature against the imported P-256 public key.

## Trust states

ZERO models trust independently from package-file integrity:

- `Unsigned`
- `SignaturePresentUnverified`
- `TrustedPublisher`
- `UntrustedPublisher`
- `InvalidSignatureEnvelope`
- `InvalidSignature`
- `UnsupportedAlgorithm`

Unsafe publisher/key identifiers, malformed key material, unsupported algorithms, unknown trusted keys, malformed signatures, and failed cryptographic verification do not become trusted packages.

## Policy

Local/sideloaded PC games may use `AllowLocalUnsigned` so local game support does not require a Store signing authority. File integrity is still enforced for the installed package.

`RequireTrustedPublisher` is the policy boundary for managed/Store distribution. A package subject to that policy must cryptographically resolve to a trusted local publisher key before launch.

## Security boundary

This implementation provides local trusted-key signature verification. It does **not** yet provide:

- a public publisher certificate authority;
- certificate issuance;
- remote key revocation;
- automated key rotation;
- publisher portal enrollment;
- Store signing infrastructure;
- DRM or online entitlement enforcement.

Those capabilities require separate production services. Runtime V4.1 must not imply they exist merely because local ECDSA verification succeeds.
