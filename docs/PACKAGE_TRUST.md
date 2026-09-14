# ZERO Package Trust Foundation

ZERO separates **package integrity** from **publisher trust**.

## Integrity

`zero.integrity.sha256` records the installed package inventory and SHA-256 digests. Runtime V4.1 verifies this file before launch. Integrity answers only:

> Are the installed package files the same files ZERO validated and installed?

It does not identify the publisher.

## Publisher trust

Optional publisher metadata uses `zero.signature.json`:

```json
{
  "schema": 1,
  "publisher_id": "example.publisher",
  "key_id": "publisher-key-1",
  "algorithm": "ed25519",
  "payload": "zero.integrity.sha256",
  "signature": "BASE64_SIGNATURE"
}
```

The signature envelope is intentionally excluded from the package inventory because it signs the generated integrity manifest. The signed payload is the exact bytes of `zero.integrity.sha256`.

## Trust states

ZERO models these independently from file integrity:

- `Unsigned`
- `SignaturePresentUnverified`
- `TrustedPublisher`
- `UntrustedPublisher`
- `InvalidSignatureEnvelope`
- `InvalidSignature`
- `UnsupportedAlgorithm`

## MVP policy

The current runtime uses `AllowLocalUnsigned`. This preserves local/imported PC game support while still enforcing launch-time file integrity.

A syntactically valid signature with no connected publisher trust provider is reported as **present but unverified**. ZERO must never label that package as trusted or verified.

Malformed/invalid signature metadata fails closed rather than silently becoming an unsigned package.

## Future Store policy

`RequireTrustedPublisher` is already represented as a separate policy. A future Store/PKI implementation can provide `IPublisherTrustProvider` and require a cryptographically verified publisher without changing Runtime V4 launch architecture.

## Explicit non-claims

This foundation does **not** provide production PKI, certificate issuance, revocation, key rotation, Store signing, DRM, entitlement enforcement, or publisher authenticity today. The `DisconnectedPublisherTrustProvider` deliberately makes no identity-verification claim.
