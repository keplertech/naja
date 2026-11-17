<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Frontend mux-cleanup removal readiness — 2026-11-16

Follow-up: [#541 measurement and validation](case-replay-scalars.md#measurement-follow-up--2026-11-17)
avoids all 207 targeted scalar allocations; 176 SV and 18 VHDL direct cleanup
removals remain. The figures below are the historical pre-#541 audit.

**Removal is not justified. Keep both frontend calls enabled.** Newly measured
direct cleanup remains substantial; synthetic-chain success does not cover the
remaining replay, expression and materialization boundaries. No optimization was
implemented. Production sources are restored; this report changes no production behavior.

## Revision, environment and method

Freshly fetched main: **`c968cf72f712fdfa5a2e27cebb2af2ff53b4dd59`**.
An isolated feature worktree preserved the original checkout. Apple M1, eight
cores, 16 GiB, arm64 macOS 27.0.1 (26A434), Apple clang 21.0.0, CMake 4.4.4,
Python 3.14.8; CMake Release `-O3 -DNDEBUG`, six build workers, sanitizers,
coverage and frontend performance reporting off. Dependency checkouts match
main: slang `545f88b2`, naja-verilog `4459520a`, naja-if `e1ce72c5`, googletest
`063de7e9`. No build or dependency policy was changed.

The established 13 public/synthetic cases and options were reused: default
CHERIoT Ibex, CI AES decrypt, FIR, overwrite/dead-RHS pairs, repeated chains,
and both mixed-vector orientations in each HDL. All **240 input/source/include
hashes** match the preceding Release study and remain unchanged. Provenance and
configuration are in [the original report](README.md) and
[Release protocol](release-protocol.md). No confidential inputs were used.

Diagnostic builds counted actual canonical mux creation in
`SNLInstance::commonPostCreate` (including clones), pass entry/exit live counts,
summed output widths, and individual destructions. Creation call sites, requested
vector widths, materialization boundaries and stacks classified removals.
Nonmutating probes inspected compatibility before/after SV alias cleanup.
Three fresh processes per workload repeated all counts; diagnostic and
uninstrumented enabled builds have matching final structure. Instrumentation
was excluded from every performance measurement.

## Newly measured direct cleanup

Figures are **folded mux instances / summed output bits**. Cumulative allocations
equal pass-entry totals on every case; pass-exit totals equal final folded muxes.

| Workload | Allocated / immediately before pass | Immediately after pass | Direct removals |
|---|---:|---:|---:|
| Ibex SV | 2,115 / 15,442 | 1,732 / 14,491 | **383 / 951** |
| AES VHDL | 273 / 2,906 | 255 / 2,566 | **18 / 340** |
| Repeated chain, each HDL | 1 / 32 | 1 / 32 | 0 / 0 |
| Mixed chain, each HDL/orientation | 2 / 4 | 2 / 4 | 0 / 0 |
| Dead RHS, each HDL | 32 / 1,024 | 32 / 1,024 | 0 / 0 |
| Overwrite SV; VHDL | 0 / 0; 1 / 1 | same | 0 / 0 |
| FIR VHDL | 0 / 0 | 0 / 0 | 0 / 0 |

Historical #535/#536 reports recorded 32→1 and 64→2 allocation reductions and
2,115/1,732 Ibex, 273/255 AES allocation/final counts. Those figures are
reproduced; **the new direct counters**, rather than subtraction alone, now
attribute these particular gaps to this pass.

| Inner → outer lowering path | Ibex removals | Materialization boundary |
|---|---:|---|
| Replay → replay | 267 | Eager replay bit vectors; 233 case→case, 34 involving `if` merges |
| Conditional expression → replay | 14 | Expression bits escape into replay state |
| Pending selection ↔ case output | 60 | Owning `if` result flushes; case output batching flushes on different vectors |
| Case output → case output | 26 | Whole-vector batching misses common changed runs |
| Pending → pending | 10 | Owning merge escape or incompatible pending selections |
| Conditional expression → case | 3 | Eager expression result |
| Into assignment / enable mux | 3 | Two scheduled-assignment boundaries, one storage-enable boundary |

Of the 267 replay→replay removals, **214 have whole-scalar requests; 207 are
case-arm→case-arm**. Decoder contributes 256 removals / 258 bits, compressed
decoder 68 / 349; other designs contribute 59 / 344.

AES: seven nested conditional-expression merges (224 bits), five eager scheduled
merges (98), three pending→pending (15), one pending→eager scheduled (one), and
two memory-write-enable merges (two). Four removed pending results flushed at a
statement-list exit; outer helper fallbacks encounter materialized/incompatible
branches. Key expansion contributes 14 / 336; decrypt contributes four / four.
The scheduled fallback includes complex branches/cases. Its finer predicate,
and which SV owning-`if` eligibility predicate failed, were not individually
counted; those conservative boundaries must not simply be relaxed.

All 401 merges use distinct guards. Common-true/common-false counts are
**285/98 for Ibex**, **16/2 for AES**. Width histograms (width: removals):
Ibex `1:296, 2:7, 3:3, 4:1, 5:57, 6:1, 7:9, 24:2, 32:6, 34:1`;
AES `1:6, 6:1, 8:1, 32:10`.

SV probes found 379 immediate compatible pairs before alias cleanup and 381
afterward. Of actual removals, **378 were already visible before aliases, two
one-bit pairs became visible only after aliases**, and three appeared during
earlier mux merges. The two occur in ID-stage and multiply/divide lowering.
Probe edges do not simulate closure: one probed edge is replaced during merging;
the three pass-exposed pairs' transitive alias dependence remains unresolved.
VHDL has no corresponding separate alias-cleanup phase at this call boundary.

## Disabling only the frontend calls

The isolated variant changes exactly the two call statements, leaving the utility
and mux2 semantics intact. No post-load optimizer/DLE runs.

| Final view | Enabled muxes / bits | Disabled muxes / bits |
|---|---:|---:|
| Ibex folded | 1,732 / 14,491 | **2,115 / 15,442** |
| Ibex hierarchy-expanded | 1,739 / 14,690 | **2,122 / 15,641** |
| AES folded and expanded | 255 / 2,566 | **273 / 2,906** |

Other workloads retain identical final structure. Total instances stay unchanged:
Ibex 28,853 folded / 29,081 expanded, AES 2,739. Every direct removal replaces a
mux with one guard gate. Disabling avoids guard nets: folded Ibex nets/bitnets
18,045/55,927 → 17,662/55,544; AES 2,798/10,062 → 2,780/10,044. Enabled cleanup
retains unused intermediate data nets. Allocation events are never expanded by
hierarchical reuse.

Eight focused cross-process mux/overwrite tests passed in **each** uninstrumented
variant, comparing live state, Verilog/DOT, snapshots, round trips and cold
reloads. These establish within-variant determinism, not equal IDs/bytes across
variants or external-benchmark functional equivalence. No full frontend,
four-state-equivalence or Bazel campaign was run for this report-only audit.

Loading cost was measured separately: eight balanced paired blocks, seed 536535,
26 excluded warm-ups and **208 retained fresh-process samples**, none dropped.
Builds finished before timing; workers verified producer identity and all loaded
Naja library locations. Only raw loading is timed; peak process RSS is sampled
before statistics. CPU frequency and unrelated OS activity were uncontrolled.

| Workload | Enabled ms, median [range] | Disabled ms, median [range] | Paired time change [95% interval] | Paired RSS MiB [95% interval] |
|---|---:|---:|---:|---:|
| Ibex | 305.403 [261.491–607.284] | 266.617 [250.796–452.144] | -9.41% [-17.16, -3.42] | +0.008 [-0.047, +0.250] |
| AES | 95.680 [87.203–171.096] | 89.381 [85.025–316.303] | -2.36% [-37.13, +1.68] | -0.203 [-1.188, +0.141] |
| FIR control | 2.330 [1.995–6.185] | 2.408 [2.003–13.715] | +0.08% [-33.40, +80.95] | +0.078 [-0.109, +0.188] |

Intervals use 10,000 paired bootstrap resamples and are exploratory/unadjusted.
Ibex is observed faster disabled; wide ranges and noisy controls limit the
estimate. All other workloads' time intervals include zero. Mixed common-true SV
has a small RSS increase, +0.195 MiB [+0.047, +0.344]; no broad memory benefit is
established. Full samples retain all workloads and ranges. An initial warm-up
failed identity checking because the separate Python extension target had not
been rebuilt from the reused cache; it was rebuilt, archives corrected, and all
structural checks repeated before the accepted campaign. No failed sample entered
these results. These are CMake observations, not build-path parity or BCR evidence.

## Smallest proposed next change — not implemented

**Add a case-local scalar accumulator for SV replay symbols**, using the existing
common-arm identities/helper. Start only with whole scalar case-arm results;
keep branch/guard evaluation eager and keep the incoming replay environment
available for item/selector reads. Materialize at case exit or before any read,
sharing, scheduling, partial update or uncertain escape. Preserve declaration
order, cache ownership and every folded source origin. Do not defer replay across
statements or modify enable/storage/memory paths.

The observed **207 scalar case→case removals are an opportunity ceiling**, not
promised coverage or speedup: construction-time ownership/read eligibility and
cache sharing still need focused proof. Scope is the SV case loop and replay
adapter, the existing mux helper where needed, and focused tests; no general
planner, dead-RHS elimination or primitive capability change is proposed.

VHDL already buffers simple scheduled case chains. Its 18 residual merges cross
different boundaries, so extending the SV replay mechanism to VHDL is not
justified. Keep its cleanup too; seven private nested conditional-expression
merges provide a separate future mux-only candidate. Memory-enable and broad
complex-branch buffering remain outside this recommendation.

Before removal: demonstrate zero direct removals across both frontends on this
corpus and broader boundary regressions; resolve alias/transitive opportunities;
verify diagnostics, source origins, four-state behavior, scheduling, determinism
and snapshots, and run applicable CMake/Bazel checks plus an uninstrumented cost
comparison. No single proposed slice covers all current residuals. Assess utility
deletion independently: it remains a public C++ operation with direct kernel tests;
external callers were not surveyed.

Evidence: [counts](removal-readiness-counts.csv),
[per-design pass totals](removal-readiness-designs.csv),
[classified merges](removal-readiness-merges.csv),
[all timing samples](removal-readiness-timings.csv). Raw logs, protocols,
instrumentation patches, input hashes and binary archives remain local outside
the tracked tree. Older internal planner notes were not resumed.
