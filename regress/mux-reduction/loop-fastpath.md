<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Empty active-loop bindings — 2026-11-15

The SystemVerilog constructor now returns immediately from
`getActiveForLoopConstant` and `isActiveForLoopVariableExpr` when both the
symbol and name binding tables are empty. This skips diagnostic expression
formatting and recursive scans identified by the [profile](ibex-profile.md).
Matching with either table populated is unchanged. The predicate deliberately
does not use the loop-break stack: captured bindings also serve replay.

## Comparison protocol, declared before timing

Compare the immutable uninstrumented `f6388fae` archive (`after536`) with a
Release build of that revision plus the constructor change (`fastpath`). The
candidate retains the base producer hash because the patch was uncommitted
when measured;
separate archives and binary hashes distinguish the builds. The test-detail
header also adds result fields for focused helper checks. No production
instrumentation is enabled.

Use the same compiler, dependencies, build directory and 13 public workloads
as the [Release study](release-protocol.md). Collect 18 balanced paired blocks,
468 fresh-process samples, after 26 excluded warm-ups. Each revision order
appears nine times; shuffle workloads and order schedule with seed 536535.
Use `release_measure.py` with a two-entry archive manifest. Complete builds
and correctness testing before timing; do not run those tasks concurrently.
Retain every observation, audit loaded library paths, hash all inputs before
and after, and require unchanged structural counts across both builds.

Report median load times, median paired candidate/baseline time ratios and
paired peak-RSS differences, with exploratory percentile bootstrap 95%
intervals (10,000 resamples, seed 536535), using `bootstrap_median` from
`release_summary.py`. RSS includes interpreter startup and import. Raw logs,
archives, source and binary hashes remain local; numeric samples are sanitized.

## Correctness and build validation

The focused helper test checks empty bindings over twelve expression forms,
symbol-only bindings, and existing name-only conditional, concatenation and
call-argument cases. Existing active-binding matching stays intact.

The CMake Debug build passed 1,709 selected SystemVerilog, VHDL, shared RTL,
utility and serialization tests. The full CMake cross-process determinism
suite passed (169 seconds). Both archived Release builds separately passed
all 365 serialization tests and nine focused cross-process tests covering
canonical DB0 families and the four mux-reduction cases in each language.

Local Bazel validation is blocked before frontend compilation: the SHA-256
dependency compile rejects Xcode's `SDKSettings.json` as an undeclared absolute
include. No Bazel test passed in this run; CMake success does not establish
build-path parity. This needs a working Bazel environment or CI validation.

## Release results

All 468 retained samples completed. Input hashes were unchanged, and all five
structural counts matched across revisions and samples for each workload.
The [numeric samples](loop-fastpath-samples.csv) retain every observation.

| Workload | Baseline median, ms | Candidate median, ms | Paired time change [95% interval] | Paired peak RSS change, MiB [95% interval] |
|---|---:|---:|---:|---:|
| aes | 85.997 | 85.361 | -0.60% [-1.82, -0.08] | +0.000 [-0.039, +0.078] |
| common_false_sv | 3.647 | 2.110 | -42.28% [-43.26, -41.32] | +0.016 [-0.109, +0.141] |
| common_false_vhd | 1.998 | 2.014 | -0.79% [-2.30, +1.34] | -0.086 [-0.148, -0.039] |
| common_true_sv | 13.151 | 2.706 | -79.67% [-79.96, -79.41] | -0.070 [-0.148, -0.031] |
| common_true_vhd | 2.420 | 2.426 | +0.79% [-0.94, +6.21] | +0.070 [-0.031, +0.133] |
| dead_rhs_sv | 8.344 | 5.675 | -32.02% [-32.88, -31.43] | -0.070 [-0.148, +0.000] |
| dead_rhs_vhd | 9.569 | 9.580 | -0.26% [-1.04, +0.95] | -0.070 [-0.180, +0.016] |
| fir | 2.026 | 2.008 | -0.33% [-1.57, +0.57] | +0.000 [-0.156, +0.062] |
| ibex | 491.626 | 283.984 | -43.19% [-44.65, -41.67] | -0.039 [-0.188, +0.016] |
| repeated_sv | 5.136 | 3.812 | -25.41% [-26.31, -24.33] | +0.000 [-0.156, +0.109] |
| repeated_vhd | 3.177 | 3.187 | +0.37% [-0.97, +1.35] | -0.117 [-0.172, -0.047] |
| scheduled_overwrite_sv | 1.072 | 0.975 | -12.34% [-14.87, -7.56] | -0.070 [-0.180, +0.047] |
| scheduled_overwrite_vhd | 1.260 | 1.233 | -0.65% [-3.18, +0.89] | +0.008 [-0.070, +0.062] |

Ibex falls from 491.626 ms to 283.984 ms: a
43.19% median paired reduction (95% interval: 41.67–44.65% faster).
Full observed ranges are 482.950–759.314 ms
for baseline and 270.724–393.662 ms for candidate.
Its netlist remains 28,853 instances, 1,732 muxes, 14,491 mux bits, 18,045
nets and 55,927 bit nets. This change removes query work, not netlist objects.

The paired comparison uses contemporaneous baseline measurements; the older
study's approximately 481 ms median is not its denominator. AES has a small
improvement with an interval close to zero. SystemVerilog synthetic workloads
improve substantially, but do not establish a representative-workload average.
All synthetic VHDL time intervals and the FIR interval include zero; these controls do
not show a clear regression. Sub-megabyte RSS differences are small and mixed,
so this study does not establish a material peak-memory benefit. Intervals are
exploratory and unadjusted for multiple comparisons.

The measured result supports keeping the two guards as a focused performance
change. Build-path validation remains limited by the local Bazel SDK failure.
