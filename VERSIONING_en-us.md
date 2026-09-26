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
