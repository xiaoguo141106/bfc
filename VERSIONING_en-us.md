# Versioning

**English** | [简体中文](VERSIONING.md)

This project is in beta. The rules below apply from the publication of this
document; **already-published tags are never changed**.

## Overview

| Stage | Naming | Purpose | Count |
|-------|--------|---------|-------|
| Early development · Beta | beta-0.0.N | Bug fixes, new features | unlimited |
| Early development · RC | YYYY-K-RC-n | Feature freeze, fixes only | 1-3 |
| Release | YYYY-K | Public release, by season | 4 per year |
| Release fix | YYYY-K-F | Major problems only | as needed |
| Snapshot · SnapShot | YYYY-K-SnapShot-n | Daily snapshots | 7-12 |
| Snapshot · PR | YYYY-K-PR-n | Pre-release | 1-3 |
| Snapshot · RC | YYYY-K-RC-n | Feature-frozen build | 1-3 |

YYYY = release year; K = season number; F = fix number; n = index within the stage.

## Releases: by season, four per year

| K | Season | Window |
|---|--------|--------|
| 1 | Spring | Mar-May |
| 2 | Summer | Jun-Aug |
| 3 | Autumn | Sep-Nov |
| 4 | Winter | December |

- K is decided by the season; a skipped season is not back-filled.
- **K resets to 1 every year**: after winter YYYY-4, the next release is
  (YYYY+1)-1.
- Example: 2026-1 (spring), 2026-2 (summer), 2026-3 (autumn), 2026-4 (winter),
  2027-1 (spring), 2027-2 (summer), ...

## Release fixes: YYYY-K-F

- Published only when a release has a major problem; no new features.
- Example: 2026-2 has a major problem -> 2026-2-1; another -> 2026-2-2.

## Official releases must ship prebuilt binaries

Starting with the first official release (2026-1), every official release on
GitHub and Codeberg **must** carry all of the prebuilt binaries below. RC and snapshot
builds are not required to, but attaching them from the RC stage on is
recommended so the packaging flow gets exercised early.

### Asset matrix

| File | --target | Built where | Toolchain |
|------|----------|-------------|-----------|
| bfc-<version>-Windows-x64.exe | x86_64-windows | locally | MinGW-w64 UCRT, g++ -static |
| bfc-<version>-Linux-x86_64.tar.gz | x86_64-linux | locally (or WSL) | g++ -static |
| bfc-<version>-Linux-arm64.tar.gz | aarch64-linux | CI (ubuntu-24.04-arm) | native g++ -static |
| bfc-<version>-macOS-x86_64.tar.gz | x86_64-macos | CI (macos-13) | clang |
| bfc-<version>-macOS-arm64.tar.gz | aarch64-macos | CI (macos-latest) | clang |
| SHA256SUMS | - | locally | sha256sum |

- <version> is the tag with any prefix such as beta- kept as is: tag beta-0.0.4
  gives bfc-beta-0.0.4-Windows-x64.exe, tag 2026-1 gives
  bfc-2026-1-Windows-x64.exe.
- Architecture names follow uname -m: x86_64, arm64.
- Linux and macOS use .tar.gz rather than a bare binary: downloading a bare file
  from a release loses the executable bit. The archive holds bfc, LICENSE and
  LICENSE-EXCEPTION.md.
- macOS cannot link statically (only dynamic libSystem is provided; it is a
  system component, not a third-party dependency).
- Every asset must first pass the full test suite (28/28) on its own platform.
  Windows and Linux x86-64 are verified locally, the rest by CI.

### Who builds what

| Asset | Builder | Why |
|-------|---------|-----|
| Windows x64 | local | MinGW-w64 toolchain is already here and easy to verify |
| Linux x86-64 | local (or WSL) | same |
| Linux arm64 | CI | no ARM64 environment locally; CI offers native arm64 runners |
| macOS arm64 / x86-64 | CI | no Mac hardware locally |
| BSD (future) | CI | same, via a BSD runner or QEMU |

### Release steps

1. Build Windows x64 and Linux x86-64 locally, run the full suite on each.
2. Let CI build Linux arm64, macOS arm64 and macOS x86-64, and download the
   artifacts from Actions.
3. Generate SHA256SUMS locally.
4. Tag and push: origin carries two push URLs (GitHub and Codeberg), so one push
   reaches both.
5. Create the release on both sites and upload the same assets plus SHA256SUMS.
6. In the release notes, state the --target and minimum OS for each file.

## Snapshot cycle: after every release

Taking "2026-2 released, preparing 2026-3 (autumn)" as the example:

1. SnapShot: 2026-3-SnapShot-1 ... 2026-3-SnapShot-7 to 12
2. PR (pre-release): 2026-3-PR-1 ... 2026-3-PR-1 to 3
3. RC (feature frozen): 2026-3-RC-1 ... 2026-3-RC-1 to 3
4. Release: 2026-3
5. If a major problem appears: 2026-3-1

Across a year boundary the target number follows the new year: after
2026-4 (winter) is released, snapshots start at 2027-1.

## Early development (current)

### Beta (current stage)
- Naming beta-0.0.N, N increasing from 1.
- Published: beta-0.0.1 / beta-0.0.2 / beta-0.0.3, kept as they are.
- Bug fixes, new features and breaking changes are allowed.
- Exit criteria: core features complete, CI green on all three platforms, no
  known high-severity defect, CLI frozen candidate.

### RC (planned stage)
- Naming <target release>-RC-n, feature frozen, fixes only, count 1-3.
- The first release lands in the nearest season window whose exit criteria are
  met (expected 2026-4 winter, otherwise 2027-1 spring), so the RCs are
  2026-4-RC-1... or 2027-1-RC-1....

## Version constant

kVersion in bfc.cpp must match the newest tag:

- Beta: beta 0.0.N
- RC: YYYY-K-RC-n
- Release: YYYY-K
- Release fix: YYYY-K-F

## History

beta-0.0.1, beta-0.0.2 and beta-0.0.3 are already published and stay as they
are: no renaming, no history rewrite.
