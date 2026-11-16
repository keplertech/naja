<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Case-local SV replay scalars — 2026-11-16

Based on freshly fetched `origin/main` at `49bfc050c50a9481442112033b665a024c2bd0cd`.
The only change after the [removal-readiness audit](removal-readiness.md) is
CI/documentation maintenance; it does not alter the recommendation. No local
design notes or papers were present. The historical value planner was not resumed.

The case loop now owns a private scalar-selection accumulator alongside its
materialized replay map. Pending entries are absent from that map, never installed
in an active replay environment and never stored in expression caches. Each arm
still executes eagerly against a separate copy of the incoming environment;
selector and item expressions retain that incoming environment too. Thus reads,
copy-outs, dependent assignments and partial updates inside arms use ordinary
materialized bits, never an accumulated result from a different arm.

Only complete values of scalar types participate. Packed vectors (including
one-bit packed vectors), scheduling replay, active loops and uncertain subtree
effects retain eager merging. The tracked LHS retains its existing case-output
batching. The existing frontend-independent `SNLHDLMuxSelection` performs common-arm
folding and materializes incompatible chains. Pending scalars flush in declaration
order at case exit, including nested-case exits, before any result can be read,
shared or passed to a subsequent statement. Folded source ranges use the existing
source-annotation adapter. No primitive or helper semantics changed.

Both frontend calls to `mergeRepeatedMuxSelections` remain enabled. In particular,
this slice does not carry a pending inner-case result into an outer case, defer
replay across statements, or optimize VHDL's different remaining boundaries.

## Verification

Release CMake on arm64 macOS, Apple clang, Python 3.15; source dependency revisions
match main. All 1,367 SV constructor/intent tests, 269 VHDL constructor tests,
365 serialization tests, 15 HDL primitive/helper tests and 11 mux utility tests
passed. The determinism CTest contract passed all 45 Python tests, comparing live
state, Verilog/DOT, NajaIF, round trips and cold reloads. REUSE lint passed.

Focused synthetic coverage includes both helper common-arm orientations, equal
and distinct guards, source origins, nested cases, incoming selector/item reads,
interdependent symbols, copied/shared results, vector/partial-update fallbacks,
scheduled replay and eager unsupported-effect diagnostics. Icarus simulation also
checks every scalar 0/1/X/Z data pair with binary case controls; case-comparison
X/Z limitations and diagnostics are unchanged.

The allocation fixture checks for abandoned intermediate nets before assignment
merging. Against the archived `c968cf72` audit build (Python 3.14), whose frontend
and kernel sources match the new base, both builds finish with four scalar muxes.
The baseline has 40 bitnets, including four abandoned intermediate nets; this
implementation has 32 bitnets and none abandoned. Tracked-local results still
have their own muxes. This structural comparison establishes avoidance for the
synthetic fixture, not actual allocation totals or coverage of the audit's 207
opportunities. No external-workload allocation or timing campaign was run.
Bazel parity and BCR readiness were not evaluated; no build path was changed.
