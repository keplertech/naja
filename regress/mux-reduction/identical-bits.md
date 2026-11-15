<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Pending selections with identical bit positions — 2026-11-15

The follow-up to [private pending selections](pending-selections.md) retains
complete branch vectors when some corresponding positions are identical.
The existing frontend builders split the changed runs only when the owning
merge materializes the result. Both common-arm orientations use the same
AND/OR identities as before. A common arm must still match the entire vector
in the same order, including its unchanged positions. A partial match or
permutation cannot justify combining guards.

This changes only the shared buffer's eligibility check. Statement, ownership,
read, scheduling and replay boundaries remain those introduced by #535.
Unavailable values and invalid widths still materialize through the ordinary
builder. No new primitive, primitive capability or HDL construct is introduced.

## Allocation measurements

Baseline: merged #535, `f3cf91ce`. Candidate: the identical-position eligibility
change on that source/dependency tree. Both use the same existing Debug CMake
cache, compiler and build options. The nine public workloads from the earlier
study are supplemented with four independent synthetic cases: a 32-stage
chain, in each HDL and common-arm orientation, selecting between seven-bit
vectors with three unchanged positions and two separate two-bit changed runs.

Each workload was loaded in three fresh processes per version, sequentially,
with normal cleanup enabled. All allocation count/width pairs repeat exactly
within each group. The counter in `SNLInstance::commonPostCreate` records actual
canonical mux creation events. Temporary instrumentation was removed after
measurement. No timing or memory-performance claim is made.

| Workload | #535 allocated muxes / output bits | Candidate allocated muxes / output bits |
|---|---:|---:|
| Overwrite SV | 0 / 0 | 0 / 0 |
| Overwrite VHDL | 1 / 1 | 1 / 1 |
| Repeated SV | 1 / 32 | 1 / 32 |
| Repeated VHDL | 1 / 32 | 1 / 32 |
| Dead RHS SV | 32 / 1,024 | 32 / 1,024 |
| Dead RHS VHDL | 32 / 1,024 | 32 / 1,024 |
| Ibex SV | 2,116 / 15,445 | 2,115 / 15,442 |
| AES VHDL | 273 / 2,906 | 273 / 2,906 |
| FIR VHDL | 0 / 0 | 0 / 0 |
| Mixed-vector common-true chain SV | 64 / 128 | 2 / 4 |
| Mixed-vector common-true chain VHDL | 64 / 128 | 2 / 4 |
| Mixed-vector common-false chain SV | 64 / 128 | 2 / 4 |
| Mixed-vector common-false chain VHDL | 64 / 128 | 2 / 4 |

Final Ibex mux count/width is unchanged at 1,732 / 14,491. Its total instance
count changes from 28,854 to 28,853, nets from 18,047 to 18,045, and bit nets
from 55,931 to 55,927. AES final structure is unchanged. The mixed-vector
fixtures retain two final muxes in both versions; the improvement avoids
constructing the 62 intermediates and avoids duplicating the combined guard
chain for each changed run. These are strong synthetic savings and a small
representative improvement, rather than a broad performance result.

## Regression coverage

Helper tests cover equal/distinct guards, both orientations and identical
positions. A negative case changes an inner selection's previously unchanged
position: matching only the changed bits would incorrectly discard this outer
update, so the result must materialize instead. Permuted common arms and
materialized/shared results retain their fallback coverage.

The frontend tests exhaust binary values for whole and mixed vectors, check
folded origins, and reject abandoned intermediate scalar or bus nets. The dump
simulation covers mixed-vector 0/1/X/Z data with binary procedural guards,
including unchanged Z bits. New cross-process SV/VHDL cases exercise ascending
ranges, separated changed runs, sequential outputs, sharing, slices and reordered
updates, with live-state/dump comparisons and fresh-process snapshot reloads.

All 1,709 selected CMake frontend, primitive, utility, serialization and native
overwrite tests passed. The full cross-process determinism contract passed,
including the two new mixed-vector cases. `reuse lint` and `git diff --check`
also passed.

Local Bazel validation was attempted and stopped before Naja compilation on the
existing macOS SDK builtin-include configuration error. Source-based Bazel CI
remains required before merging a follow-up PR; this run makes no BCR claim.
