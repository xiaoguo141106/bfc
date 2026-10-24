# Versioning

**English** | [简体中文](VERSIONING.md)

This project is in the pre-release stage, before the first official release.
The rules below apply from the publication of this document;
**already-published tags are never changed**.

## Overview

| Stage | Naming | Purpose | Count |
|-------|--------|---------|-------|
| Pre-release (before the first release) | RC-n | Feature freeze, fixes only; numbers start at 1, no zero padding | unlimited |
| Release | YYYY-K | Public release, by season | 4 per year |
| Release fix | YYYY-K-F | Major problems only | as needed |
| Snapshot | YYYY-K-SnapShot-n | Snapshots before a release | 1-3 |

- Removed: Beta (replaced by RC) and PR (pre-release).
- YYYY = target release year; K = season number; F = fix number; n = index
  (no zero padding).
- Examples: RC-1, RC-2, RC-3 ...

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

Starting with the first official release, every official release on GitHub and
Codeberg **must** carry all of the prebuilt binaries below. RC and snapshot
builds are not required to, but attaching them from the RC stage on is
recommended so the packaging flow gets exercised early.

### Asset matrix

| File | --target | Built where | Toolchain |
|------|----------|-------------|-----------|
| bfc-<version>-Windows-x64.exe | x86_64-windows | locally / CI | MinGW-w64 UCRT, g++ -static |
| bfc-<version>-Linux-x86_64.tar.gz | x86_64-linux | locally / CI | g++ -static |
| bfc-<version>-Linux-arm64.tar.gz | aarch64-linux | CI (ubuntu-24.04-arm) | native g++ -static |
| bfc-<version>-macOS-x86_64.tar.gz | x86_64-macos | CI (macos-26-intel) | clang |
| bfc-<version>-macOS-arm64.tar.gz | aarch64-macos | CI (macos-26) | clang |
| SHA256SUMS | - | CI / locally | sha256sum |

- <version> is the tag as is: tag RC-1 gives bfc-RC-1-Windows-x64.exe,
  tag 2026-1 gives bfc-2026-1-Windows-x64.exe.
- Architecture names follow uname -m: x86_64, arm64.
- Linux and macOS use .tar.gz rather than a bare binary: downloading a bare file
  from a release loses the executable bit. The archive holds bfc, LICENSE and
  LICENSE-EXCEPTION.md.
- macOS cannot link statically (only dynamic libSystem is provided; it is a
  system component, not a third-party dependency).
- macOS assets pin the deployment target: 11.0 for arm64, 10.15 for x86_64.
- macOS x86-64 is deprecated; it is still shipped in this release and may be
  removed later.
- Every asset must first pass the full test suite (30/30) on its own platform.

### Automation

.github/workflows/release.yml runs when an RC-* or 20* tag is pushed:

1. builds on five runners: Linux x86_64, Linux arm64, macOS x86_64, macOS arm64,
   Windows x64;
2. checks that kVersion in the source matches the tag (hard failure otherwise);
3. packages with the names above (tar.gz for Linux/macOS, including LICENSE and
   LICENSE-EXCEPTION.md);
4. generates SHA256SUMS;
5. uploads everything to that tag's GitHub Release (creating it if needed, and
   marking RC tags as prerelease).

workflow_dispatch builds only and does not publish, for pipeline testing.

### Release steps

1. Make sure kVersion is set to the tag name and committed.
2. Tag and push: origin carries two push URLs (GitHub and Codeberg), so one push
   reaches both.
3. release.yml builds the five assets and uploads them to the GitHub Release.
4. Create the same release on Codeberg and upload the same assets plus
   SHA256SUMS (or automate it with a token).
5. In the release notes, state the --target and minimum OS for each file.

## Snapshot cycle: before every release

Taking "preparing 2026-3 (autumn)" as the example:

1. SnapShot: 2026-3-SnapShot-1 ... at most 3
2. Release: 2026-3
3. If a major problem appears: 2026-3-1

Across a year boundary the target number follows the new year: after
2026-4 (winter) is released, snapshots start at 2027-1.

## Pre-release stage (current)

- Naming RC-n, numbers starting at 1, no zero padding (RC-1, RC-2, RC-3 ...).
- Replaces both Beta (beta-0.0.N) and the per-release RC (YYYY-K-RC-n).
- Bug fixes and necessary changes are allowed; once feature-frozen, fixes only.
- Published tags beta-0.0.1 ... beta-0.0.5 stay as they are.
- Exit criteria: core features complete, CI green, no known high-severity
  defect, CLI frozen.

## Version constant

kVersion in bfc.cpp must match the newest tag:

- Pre-release: RC-n (for example RC-1)
- Release: YYYY-K
- Release fix: YYYY-K-F

## History

beta-0.0.1 ... beta-0.0.5 are already published and stay as they are:
no renaming, no history rewrite.
