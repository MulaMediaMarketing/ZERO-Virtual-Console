# ZERO Runtime V4.1 Package Security

This slice hardens folder import before ZERO accepts a game into the managed Library.

## Import gate

A package is rejected when it contains or exceeds any of the following:

- symbolic links, junctions, or other NTFS reparse points
- multiply-linked regular files
- non-regular filesystem objects
- unsafe relative paths or `..` traversal
- Windows reserved device names such as CON, NUL, COM1, or LPT1
- path components ending in a space or period
- more than 20,000 regular package files
- more than 50 GiB total package content
- a regular file larger than 4 GiB
- a `zero.manifest.json` larger than 1 MiB
- manifest executable/artwork paths that escape the package root
- reparse-point executable/artwork paths
- invalid minimum runtime declarations

## Staged-copy integrity

ZERO computes SHA-256 for every regular source file, copies the package into the managed staging area, then independently scans and hashes the staged copy. Finalization is refused unless path, size, file count, aggregate size, and SHA-256 digest all match.

After verification ZERO writes `zero.integrity.sha256` into the installed package. The integrity manifest intentionally excludes itself from its inventory.

## Authenticity vs integrity

`zero.integrity.sha256` proves what ZERO verified during import and supports later tamper checks. It does **not** prove publisher identity.

Publisher authenticity is a separate release-security layer and must use signed package metadata with a ZERO trust policy / certificate chain. Runtime V4.1 does not claim that public signing PKI exists yet.

## Threat boundary

This import layer reduces path-escape, reparse-link, hard-link, staging-tamper, oversized-package, and device-name abuse. It is not a hostile-code sandbox. Imported native games still execute as native Windows programs under the ZERO runtime process/job policy.
