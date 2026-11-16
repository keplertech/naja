<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Loader after the loop-query fast path — 2026-11-15

Profiling merged `34dbb3ef` confirms that the previous loop-query bottleneck
has largely disappeared. The next evidenced redundant operation is core
terminal order-ID maintenance: 94.80% of visited bits already have the value
being assigned. This report follows the [fast-path comparison](loop-fastpath.md).

## Uninstrumented CPU captures

Two independent processes repeatedly loaded the same public pinned Ibex input
and destroyed the universe between loads. Native `sample` captures requested
ten seconds at one-millisecond intervals. The Release build uses the prior
compiler, dependency revisions and configuration, with constructor performance
instrumentation off. Its archive contains the Python module and every Naja
runtime library; worker checks verify their locations and the merged producer
identity. No build or correctness job ran during either capture.

Capture A has 6,232 main-thread load observations out of 6,778 main-thread
observations; capture B has 5,659 out of 6,137. The [parser](profile_sample.py)
uses exclusive tree weights and attributes recursive frames once. Categories
are inclusive, overlap, and must not be added. They describe sampled
main-thread load stacks, including waits, rather than total CPU utilization.
These warm repeated-load profiles do not replace fresh-process timings.

| Category | Capture A | Capture B |
|---|---:|---:|
| Procedural dispatcher | 54.27% | 53.37% |
| Combinational procedural lowering | 52.13% | 51.40% |
| Per-LHS statement lowering | 32.81% | 33.38% |
| Continuous assignments | 13.06% | 13.09% |
| Case-item matching | 12.29% | 13.06% |
| Constant `int64` evaluation | 11.84% | 10.64% |
| Flattened collection iterator advancement | 11.54% | 10.55% |
| Mux emission | 9.90% | 9.75% |
| Instance post-creation | 9.10% | 7.97% |
| Combinational cycle checking | 7.89% | 7.72% |
| Terminal order-ID recomputation | 7.81% | 7.44% |
| Assigned-bit mask construction | 7.80% | 6.77% |
| Source annotation | 6.80% | 7.07% |
| Active-loop query union | 6.47% | 6.29% |
| Source lookup | 4.67% | 5.04% |
| Anonymous-assign cleanup | 4.72% | 4.79% |
| Repeated-mux cleanup | 4.06% | 4.08% |
| Expression description inside loop queries | 4.03% | 3.73% |
| Slang `parseAllSources` | 3.63% | 4.36% |

The earlier profile placed 45.85/45.51% of load stacks under loop queries and
42.12/42.07% under their expression descriptions. Those shares now fall to
approximately 6% and 4%. Their denominators also changed; subtracting shares
is not an elapsed-time measurement. The procedural dispatcher includes
`always_comb`, and the Slang parse bucket excludes other Slang semantic work.

## Allocation capture

A separate process used full malloc stack logging, then held one loaded
universe while `heap --noContent --sortBySize` and
`malloc_history -callTree -highWaterMark -noContent -chargeSystemLibraries`
inspected it. The live heap tool classified 447,858 allocations totaling
46,488,240 allocator bytes. Its largest relevant groups were:

| Classification | Live allocations | Allocator bytes |
|---|---:|---:|
| Instance terminals | 132,217 | 12,692,832 |
| Instances | 28,853 | 6,463,072 |
| Bus net bits | 39,808 | 6,369,280 |
| Source-info objects | 47,748 | 3,055,872 |
| Source-origin records | 45,006 | 2,880,384 |
| Scalar nets | 16,120 | 2,579,200 |
| Bus terminal bits | 15,015 | 2,402,400 |

Full history reports 4,324,838 allocations, 3,877,442 deallocations, approximately
245.6 MB cumulatively allocated, and a 77.8 MB heap-plus-VM high-water mark
with 499,611 allocations live there. These are tool-reported process totals,
including imports and harness activity. Logging perturbs runtime, memory and
allocator size classes. The earlier live-heap capture used a different logging
mode and harness: its byte totals are not a controlled before/after comparison.
Neither capture establishes a netlist-memory or production peak-RSS reduction.

The retained objects do not show new mux redundancy or justify dropping
provenance. CPU evidence instead points to repeated traversal of existing bits.

## Counter check and implementation boundary

Three separate diagnostic loads produced identical counts:

| Metric | Per load |
|---|---:|
| Term additions | 1,209 |
| Additions after the highest existing term ID | 1,209 |
| Full order-ID recomputations | 1,209 |
| Bit-terminal visits | 301,671 |
| Visits assigning the existing order ID | 285,980 |

The counters include DB0 initialization and loading, and were collected from
a separate diagnostic archive. They are not timing samples. All diagnostic
builds preserve 28,853 instances, 1,732 muxes, 14,491 mux bits, 18,045 nets and
55,927 bit nets. Input hashes match the earlier study and are unchanged across
every capture. Temporary counters were restored before the candidate build.

`SNLDesign::addTerm` currently finishes by traversing all `getBitTerms()` and
assigning compact order IDs again. `NajaFlatCollectionIterator::findNextElement`
also contributes to instance construction, so its sampled share is not an
additional independent opportunity to add to the order-update share.

The candidate keeps existing IDs when a new top-level term follows the
highest term ID. It derives the starting order from the last surviving bit of
the preceding top-level term, then assigns only the new term's bits. It adds
no cached count or deferred update. Bundle-member additions, out-of-order
insertions, empty predecessors and other uncertain cases retain the full
traversal. Deletions and bus resizing retain their existing recomputation.
All IDs remain immediately available. Flat IDs, object IDs, names,
connectivity and serialization formats are unchanged.

Case matching, constant evaluation, cycle checks and bit masks remain possible
later targets. They carry loop-context, invalidation or language-semantics
constraints; this profile alone does not justify caching them. Mux emission
is now about 10% of load stacks but this does not establish that its work is
redundant. The order-ID candidate is selected for its directly counted repeated
work and narrow semantics, not as a claim that 7–8% of load time can all be saved.

## Candidate timing protocol, declared before timing

Compare archived uninstrumented merged `34dbb3ef` (`merged`) against the same
Release configuration plus the order-ID append change (`append`). Both retain
the merged producer identity because the candidate is an uncommitted patch;
source and binary hashes distinguish them. No diagnostic counters are enabled.
Use all 13 existing public workloads, 18 balanced paired blocks, seed 536535,
and one excluded warm-up per workload/revision: 468 retained samples and 26
warm-ups. Each order appears nine times. Finish builds and correctness checks
before sampling, retain every observation, verify input hashes and loaded
libraries, and require the five structural counts to match across builds.

Use `release_measure.py` with a two-entry archive manifest. Report separate
medians and paired time-ratio and peak-RSS-difference estimates using
`bootstrap_median` from `release_summary.py` (10,000 resamples, seed 536535).
Intervals are exploratory and unadjusted across workloads. Do not aggregate
synthetic cases into a representative-design speedup. Full raw captures,
archives, hashes and failed sandbox-attach logs remain local; sanitized metrics
are in [post-loop-profile-samples.csv](post-loop-profile-samples.csv).

## Correctness checks

The CMake candidate passed all 268 kernel tests, 1,709 selected frontend,
shared-RTL, utility and serialization tests, and the full cross-process
determinism suite (146 seconds). New kernel cases verify immediate compact
order IDs after scalar and bus appends, explicit insertion before existing
terms, deleted bus bits, empty predecessors, bundle members and empty bundles.
The new determinism fixture mixes scalar and ascending/descending bus ports
across a hierarchy and compares live state, Verilog, DOT and snapshot artifacts,
including reload in fresh processes. Existing bundle restrictions are retained.

The merged Release archive passed all 365 serialization tests and nine focused
determinism tests. The candidate archive passed all 365 serialization tests and
ten focused determinism tests, including the new case. Running that synthetic
case once under each archive produced identical live state, dumps and snapshots
across the baseline and candidate. Both archived modules were checked for the
absence of the diagnostic counter marker.

The profile parser's recursive-tree check passes, and its original categories
reproduce the earlier captures exactly after adding the new categories.
Local Bazel kernel, snapshot and determinism validation is blocked before Naja
tests: the SHA-256 compile rejects Xcode's `SDKSettings.json` as an undeclared
absolute include. CMake success does not establish build-path parity; Bazel CI
or another working environment is required.

## Candidate Release results

All 468 retained samples completed, with unchanged input hashes and matching
instance, mux, mux-bit, net and bit-net counts across both builds for every
workload. [Numeric samples](term-order-append-samples.csv) retain all retained
observations. Excluded warm-up logs remain local.

| Workload | Merged median, ms | Append median, ms | Paired time change [95% interval] | Paired RSS change, MiB [95% interval] |
|---|---:|---:|---:|---:|
| aes | 84.908 | 84.903 | -0.02% [-0.38, +0.47] | -0.047 [-0.125, +0.078] |
| common_false_sv | 2.072 | 2.074 | +0.57% [-2.44, +3.54] | -0.055 [-0.141, +0.031] |
| common_false_vhd | 1.939 | 1.927 | -0.70% [-1.67, +0.51] | +0.062 [-0.008, +0.156] |
| common_true_sv | 2.687 | 2.634 | -1.98% [-3.14, +0.06] | -0.031 [-0.242, +0.109] |
| common_true_vhd | 2.371 | 2.370 | -0.21% [-1.04, +0.70] | +0.102 [+0.023, +0.164] |
| dead_rhs_sv | 5.647 | 4.234 | -24.52% [-25.46, -24.16] | +0.000 [-0.094, +0.109] |
| dead_rhs_vhd | 9.465 | 8.114 | -14.29% [-14.96, -13.81] | -0.016 [-0.078, +0.055] |
| fir | 1.970 | 1.962 | -0.35% [-1.15, +0.25] | -0.023 [-0.117, +0.023] |
| ibex | 273.137 | 254.492 | -7.04% [-7.49, -6.21] | -0.062 [-0.148, +0.008] |
| repeated_sv | 3.812 | 3.766 | -0.75% [-2.09, +1.09] | -0.094 [-0.188, -0.016] |
| repeated_vhd | 3.125 | 3.108 | -0.51% [-1.27, +0.21] | -0.016 [-0.086, +0.062] |
| scheduled_overwrite_sv | 0.968 | 0.972 | +0.96% [-2.09, +3.43] | +0.000 [-0.023, +0.055] |
| scheduled_overwrite_vhd | 1.208 | 1.181 | -2.63% [-4.44, -0.98] | -0.039 [-0.125, +0.008] |

Ibex's median is 273.137 ms on merged main and 254.492 ms with
the candidate: a 7.04% median paired reduction, with an exploratory 95%
interval of 6.21–7.49% faster.
Full ranges are 270.739–276.382 ms and
250.836–257.163 ms, respectively.
Use this campaign's paired merged baseline, not the earlier fast-path study's
approximately 284 ms candidate median, for the incremental effect.

The dead-RHS stress cases improve by 24.52% in SV and 14.29% in VHDL. The
shared core applies to both frontends; these port-heavy cases are synthetic
and do not establish a broad workload average. The scheduled-overwrite VHDL
time interval also excludes zero. All remaining time intervals include zero,
including AES and FIR; no clear time regression is established in this corpus.

Peak-RSS effects are small and mixed. The common-true VHDL stress case has a
positive paired estimate of 0.102 MiB [0.023, 0.164], while repeated SV has a
negative estimate of 0.094 MiB [0.016, 0.188]. Other intervals include zero.
No material peak-memory improvement is established. The result supports
keeping the append-only optimization, subject to the outstanding Bazel validation.

A final uninstrumented candidate capture, collected after the timing campaign,
contains 5,859 load observations out of 6,417 main-thread observations.
`setOrderIDs` is not observed in those load stacks, versus 7.81/7.44% in the
two merged-baseline captures. Flattened collection advancement remains at
6.61%, including instance construction. This checks that the intended path
lost work; one sample with zero observations does not establish zero calls.
Its input hashes match the baseline captures. These additional sample counts
are retained as `candidate_sample` rows in the profile CSV.
