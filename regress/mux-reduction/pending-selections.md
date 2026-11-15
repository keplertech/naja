<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Private pending mux selections — 2026-11-14

The first construction-time common-arm reduction buffers private, whole-vector
branch results. It combines same-arm guards with existing AND/OR primitives and
materializes one mux at the owning merge. No placeholder nets enter expression
caches. RHS expressions and diagnostics are still lowered eagerly; normal mux
cleanup remains enabled.

SystemVerilog defers final nested conditional results in sequential and
combinational per-LHS lowering. Earlier statements materialize their results.
Dependent-symbol replay, scheduling replay, loop exits and uncertain effects
remain eager. The tracked output's replay bookkeeping can be deferred until its
owning merge when the subtree summary proves it is the only affected relevant
symbol. Existing SV case-arm batching is retained.

VHDL buffers whole-signal scheduled selections in simple nested branches and
simple case-merge chains. A subsequent statement materializes pending values.
Variables, memory writes, runtime returns and complex branch bodies retain the
existing lowering. Current-signal reads, definite-write tracking, source origins,
storage models and reset/enable policies retain their existing semantics.
Partly identical vectors and incompatible common arms use the ordinary builders.

## Allocation check

The fixed corpus and actual canonical mux-creation counter from [the earlier
study](README.md) were reused. One fresh process per workload was run against
the archived `ad198544` main build and this implementation based on `4f5ba31c`.
The relevant frontend, netlist and dependency sources are identical between
those two base revisions. The new build uses the existing Debug CMake cache;
the archived baseline is Release. These are structural allocation comparisons,
with no timing or memory-performance claim and no new benchmark-equivalence
proof. Temporary instrumentation in `SNLInstance::commonPostCreate` was restored
after measurement. Counts include actual allocations, not just surviving muxes.

| Workload | Baseline allocated muxes / output bits | Pending allocated muxes / output bits |
|---|---:|---:|
| Overwrite SV | 0 / 0 | 0 / 0 |
| Overwrite VHDL | 1 / 1 | 1 / 1 |
| Repeated SV | 32 / 1,024 | 1 / 32 |
| Repeated VHDL | 32 / 1,024 | 1 / 32 |
| Dead RHS SV | 32 / 1,024 | 32 / 1,024 |
| Dead RHS VHDL | 32 / 1,024 | 32 / 1,024 |
| Ibex SV | 2,158 / 15,529 | 2,116 / 15,445 |
| AES VHDL | 305 / 3,071 | 273 / 2,906 |
| FIR VHDL | 0 / 0 | 0 / 0 |

Final folded Ibex changes from 1,733 muxes / 14,492 mux output bits,
28,852 instances and 18,086 nets to 1,732 / 14,491, 28,854 instances
and 18,047 nets. AES retains 255 muxes / 2,566 output bits while changing
from 2,743 instances / 2,834 nets to 2,739 / 2,798. Different guard construction
can change subsequent simplification and sharing; allocation savings do not
promise identical final structural counts or fewer total instances.

## Verification and remaining scope

The helper regression counts emitter calls directly: compatible 32-stage chains
in both common-arm orientations emit exactly one mux. Equal guards need no
extra gate. Partial matches, permuted common arms and materialized/shared
results exercise fallback boundaries. Frontend tests check values, retained
origins and absence of abandoned intermediate bus nets. The four-state dump
comparison covers procedural data with binary guards, and continuous muxes with
unknown guards, within the existing supported domain. New cross-process cases
compare live state, dumps, snapshots and fresh-process snapshot reloads.

The selected CMake frontend, primitive, utility and serialization suites passed
(1,708 tests), as did the determinism contract. Native scheduled-overwrite
comparison is also included in focused verification. Bazel validation was
attempted but blocked before compiling Naja by the local macOS toolchain:
Xcode SDK headers are outside its declared builtin paths; selecting the Command
Line Tools SDK instead fails to find the standard C++ `version` header.
Build-path parity and BCR readiness have not been established by this run.

Broader statement buffering, variable-result buffering, dead-RHS elimination
and a controlled Release performance campaign remain separate work.
