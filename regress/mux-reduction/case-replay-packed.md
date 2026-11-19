<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Case-local complete packed replay values — 2026-11-18

Complete private packed values can use the scalar accumulator's ownership and
case-exit materialization contract. On the recovered corpus this change avoids
**all 26 identified same-case packed replay mux allocations / 130 output bits**.
Ibex still requires **150 direct cleanup removals / 614 bits**; AES VHDL still
requires **18 / 340**. Keep both frontend cleanup calls enabled.

## Revision and bounded ownership proof

A fresh fetch selected `c9563f30391540a597ce455ce1a698794159092b` (#542).
There are **no changes after #542** on fetched main. #542 changes reports/data
only; its production sources match #541. The original checkout was preserved;
a clean feature worktree/branch was created from this fetched revision.
Implementation and measurement were completed before committing or publishing.
Historical broad planner work was not resumed; no local research/design notes
were present in the original checkout.
The measured production-only Git diff SHA-256 is
`6c792fd90b2aac6fc88746e70b9338a21244a935ef62103a06aae5c7cda66214`.

The case accumulator is separate from active replay environments. Every arm,
selector, item and guard still evaluates eagerly against the incoming replay
environment. Reads and copies inside an arm see eager bits, never pending
results from another arm. Nested cases materialize at their own exit; subsequent
statements and outer cases only receive materialized bits. Expression caches
receive only materialized nets. Declaration-order traversal and existing
source-annotation adapters are unchanged.

New packed eligibility requires a nonempty integral value, its exact declared
bit width, complete non-null arms, and no partial/member/selected writes or
subroutine copy-outs to that symbol in the case subtree. The subtree summary
checks the original assignment LHS **before** replay bookkeeping strips it to
its root symbol. Concatenated/unknown effects retain the existing conservative
fallback. Scheduling replay, active loops and possible breaks remain eager.
The old scalar eligibility is retained. No width or benchmark pattern is
special-cased. Signedness, assignment conversion, range/index mapping and bit
order are resolved by existing eager lowering before accumulation.

`SNLHDLMuxSelection` retains complete ordered vectors, including identical
positions. Its existing whole-common-arm identities fold only exact vector
matches; partial or permuted common arms materialize. Its frontend adapter
emits each changed run and annotates all folded origins. Incompatible chains
materialize inside the case; all remaining values flush in declaration order
at exit. The only helper addition is a width query replacing the scalar-only
width assumption. No expression lowering, mux semantics, primitive, storage,
reset/clock/enable/memory, packaging or compatibility changes were made.
VHDL production sources and its cleanup call are unchanged.

## Environment, inputs and measurement limits

Apple M1 / MacBookPro17,1, eight cores, 16 GiB, arm64 macOS 27.0.1 (26A434),
Apple clang 21.0.0 (`clang-2100.3.34.2`), CMake 4.4.4, Python 3.14.8,
Bazel 8.6.0. Matching uninstrumented CMake Release builds used `-O3 -DNDEBUG`,
six workers, Python and serialization targets enabled; coverage, sanitizers
and constructor performance reporting were off. Baseline and candidate were
built sequentially in the same reusable build directory, explicitly selecting
the same interpreter for `Python3_EXECUTABLE` and `Python_EXECUTABLE`, and
archiving the extension, runtime libraries and serialization executable together.
Both report the base producer hash; the candidate is its uncommitted source
change. Source patches, hashes and caches distinguish the archives.

| Dependency | Exact source revision, both builds |
|---|---|
| slang | `545f88b2ed3a2d20325fbd17413af177a6bca72f` |
| naja-verilog | `4459520a51dd246ce81bc72dd0ca8b1ed75518e6` |
| naja-if | `e1ce72c5e8eeb7803fae0bd96a267d5e3e91146d` |
| googletest | `063de7e9578f82b369302001269680b4b1553359` |
| cpptrace | `3db8da80111171c219ab5839905771386bee06b3` |

Nine of the established thirteen workloads were recovered: default public
CHERIoT Ibex, pinned CI AES decrypt, tracked FIR, both overwrite fixtures and
four mixed-vector cases. The historical repeated/dead-RHS inputs in both HDLs
were missing from the local evidence bundle and could not be recovered; they
were **excluded, not replaced**. Tracked fixture copies and freshly downloaded
pinned AES inputs match the historical hashes. All **235 available historical
source/include hashes** match. Ibex's generated file list was also missing;
it was regenerated from the tracked list with absolute paths and has a different
byte hash. Its original sources/includes match, and the baseline reproduces
all historical folded/expanded counts. This is a nine-workload campaign with
that file-list provenance limit, not a claim to repeat all 240 hashes or all
thirteen workloads. Input hashes were fixed for both variants.

Options match the preceding study: ordered sources/top, SV `keep_assigns=True`,
fixed diagnostics routing, both cleanups enabled, no optimizer/DLE after load.
Only public/synthetic inputs were used. Failed missing-input preflights are
retained separately and are not performance samples.

## Direct allocation and cleanup counters

Temporary diagnostic variants count canonical mux creation events, including
clones, in `SNLInstance::commonPostCreate`, actual canonical output widths,
live pass entry/exit counts and individual inner destructions. Creation sites,
requested widths, scoped case ordinals and materialization boundaries identify
pairs. The utility still performs its normal cleanup. Independent exit recounts
check final live totals; read-only probes inspect pre/post-alias visibility.
Three fresh processes repeat every diagnostic and uninstrumented count.
Diagnostic and uninstrumented folded/expanded results match. Every load's
creation totals equal pass-entry totals; independently recounted pass-exit totals
equal final folded mux totals. Allocation events are not hierarchy-expanded.

Cells are **events or instances / summed output bits**.

| Workload | Base created / before cleanup | Packed created / before cleanup | Base direct removals | Packed direct removals | Final muxes, both |
|---|---:|---:|---:|---:|---:|
| Ibex SV | 1,908 / 15,235 | **1,882 / 15,105** | 176 / 744 | **150 / 614** | 1,732 / 14,491 |
| AES VHDL | 273 / 2,906 | 273 / 2,906 | 18 / 340 | 18 / 340 | 255 / 2,566 |
| Mixed vectors, each HDL/orientation | 2 / 4 | 2 / 4 | 0 / 0 | 0 / 0 | 2 / 4 |
| Overwrite SV; VHDL | 0 / 0; 1 / 1 | same | 0 / 0 | 0 / 0 | 0 / 0; 1 / 1 |
| FIR VHDL | 0 / 0 | same | 0 / 0 | 0 / 0 | 0 / 0 |

The base has exactly 26 whole five-bit same-case case-arm→case-arm removals;
the candidate has zero. Actual creation events fall by 26 and their output
width sum by 130, independently of final counts. This establishes **26/26
coverage on Ibex**, not universal coverage of packed replay shapes.
The 207 scalar allocations avoided by #541 do not reappear.

| Remaining Ibex boundary | Base removals / bits | Packed removals / bits |
|---|---:|---:|
| Case-arm replay → case-arm replay, packed | 26 / 130 | **0 / 0** |
| Other replay involving `if` | 34 / 148 | 34 / 148 |
| Expression → replay accumulator | 14 / 14 | 14 / 14 |
| Owning-`if` pending → case output | 48 / 239 | 48 / 239 |
| Case output → pending | 12 / 17 | 12 / 17 |
| Case output → differing case output | 26 / 135 | 26 / 135 |
| Pending → pending | 10 / 54 | 10 / 54 |
| Expression → case output | 3 / 4 | 3 / 4 |
| Scheduled assignment / storage enable | 3 / 3 | 3 / 3 |

Of the 34 `if`-related replay removals, 15 / 81 bits now occur when an
incompatible pending case chain materializes; 19 / 67 remain eager replay
pairs. These are moved boundaries, not extra savings. AES's seven expression,
five scheduled, three pending, one pending→scheduled and two memory-enable
removals remain unchanged. Ibex has 145 residual pairs visible before aliases,
two visible only afterward and three exposed by earlier cleanup merges
(base: 171, two, three). Transitive alias dependence remains unresolved.

## Final structure, independently counted

| View | Base instances | Packed instances | Base nets / bitnets | Packed nets / bitnets | Muxes / bits, both |
|---|---:|---:|---:|---:|---:|
| Ibex folded | 28,746 | 28,730 | 17,731 / 55,613 | 17,689 / 55,467 | 1,732 / 14,491 |
| Ibex expanded | 28,974 | 28,958 | 17,805 / 56,464 | 17,763 / 56,318 | 1,739 / 14,690 |
| AES folded/expanded | 2,739 | 2,739 | 2,798 / 10,062 | same | 255 / 2,566 |

Ibex's only changed instance-model count is folded `or_2`: 5,050→5,034.
Construction uses existing guard simplification/caching while late cleanup
creates guards directly. The measured 16-instance and 146-bitnet differences
are separate final-structure observations. Every other recovered workload's
final totals are unchanged. These numbers do not establish timing/RSS gains.

## Uninstrumented loading time and peak RSS

The locally declared protocol uses **12 balanced paired blocks**, seed
`542026`, six base-first and six candidate-first blocks, seeded shuffled block
and workload order. One warm-up per case/archive was excluded (18 logged
warm-ups). **All 216 subsequent fresh-process samples were retained**, with no
failed worker, exclusion or replacement. Builds and tests completed before
sampling. Diagnostic archives were separate. SHA-256 checks confirm the two
performance archives did not change; input hashes match before/after sampling.

The unchanged `release_measure.py` runner checks producer identity, extension
location, every loaded Naja library location and fixed structural totals for
each worker. Only raw NLDB loading is timed with `perf_counter_ns`, excluding
imports, universe creation, statistics and startup. Peak `ru_maxrss` is captured
before statistics; on macOS it is peak **process** bytes including Python/import,
not live netlist memory. Processes run sequentially. CPU frequency and unrelated
OS activity are uncontrolled.

Reproduce using the nine-case manifest and `base`/`packed` archive mapping:

```sh
python3 regress/mux-reduction/release_measure.py \
  --cases CASES --archives ARCHIVES --output OUT --blocks 12 --seed 542026
```

Time entries are median **[full min–max] ms**. Changes use the median of the
12 paired ratios, not a ratio of medians. RSS changes are paired median MiB
differences. Intervals are percentile **95% paired bootstrap** intervals from
10,000 resamples using `release_summary.bootstrap_median`, seed `542026`.
They are exploratory, unadjusted for multiple comparisons and conditional on
this short single-machine campaign.

| Workload | Base ms [range] | Packed ms [range] | Paired time change [95% interval] | Paired RSS MiB [95% interval] |
|---|---:|---:|---:|---:|
| aes | 95.894 [92.913–100.106] | 94.887 [92.661–98.036] | -0.36% [-2.59, +1.23] | +0.008 [-0.141, +0.141] |
| common_false_sv | 2.495 [2.344–6.241] | 2.509 [2.445–4.426] | +1.90% [-3.27, +5.01] | -0.109 [-0.195, -0.047] |
| common_false_vhd | 2.533 [2.285–3.094] | 2.551 [2.394–3.112] | +3.88% [-0.08, +8.74] | -0.008 [-0.078, +0.070] |
| common_true_sv | 3.163 [2.952–3.338] | 3.117 [2.964–3.256] | -0.16% [-2.57, +3.36] | -0.164 [-0.312, -0.055] |
| common_true_vhd | 2.995 [2.798–3.269] | 2.922 [2.781–3.438] | -1.98% [-6.26, +1.35] | +0.047 [-0.062, +0.156] |
| fir | 2.566 [2.343–2.705] | 2.524 [2.379–3.559] | +0.63% [-1.23, +1.14] | +0.047 [-0.125, +0.188] |
| ibex | 292.193 [285.320–315.198] | 292.505 [288.469–308.803] | +0.84% [-0.68, +2.14] | -0.273 [-0.383, -0.055] |
| scheduled_overwrite_sv | 1.292 [1.182–1.777] | 1.307 [1.160–2.304] | +1.96% [-1.47, +14.26] | -0.133 [-0.250, -0.047] |
| scheduled_overwrite_vhd | 1.591 [1.474–1.796] | 1.616 [1.515–1.753] | -0.12% [-2.60, +3.63] | -0.016 [-0.133, +0.125] |

Peak process RSS medians **[full min–max] MiB**:

| Workload | Base | Packed |
|---|---:|---:|
| aes | 43.336 [43.125–43.516] | 43.297 [42.938–43.438] |
| common_false_sv | 30.367 [30.141–30.562] | 30.219 [30.062–30.469] |
| common_false_vhd | 28.648 [28.516–28.844] | 28.656 [28.594–28.750] |
| common_true_sv | 30.422 [30.141–30.625] | 30.266 [30.078–30.484] |
| common_true_vhd | 29.055 [28.703–29.156] | 29.062 [28.828–29.438] |
| fir | 28.805 [28.625–29.312] | 28.945 [28.594–29.219] |
| ibex | 99.398 [99.109–99.578] | 99.172 [99.000–99.312] |
| scheduled_overwrite_sv | 30.070 [29.875–30.344] | 29.922 [29.812–30.125] |
| scheduled_overwrite_vhd | 28.219 [28.078–28.578] | 28.234 [28.047–28.422] |

**No loading-time improvement is established:** every workload's time interval
includes zero. Ibex is +0.84% [−0.68, +2.14], despite its measured allocation
reduction. Its peak process RSS is locally lower by 0.273 MiB [−0.383, −0.055].
Unchanged mixed common-true/common-false SV and overwrite SV also have negative
RSS intervals; unchanged VHDL/FIR control intervals include zero. This pattern,
process layout/granularity, uncontrolled OS activity, sample size and multiple
comparisons limit causal attribution. The experiment does not establish a
general memory improvement or a memory benefit caused by the avoided muxes.
All controls and outliers are retained; no synthetic results are aggregated into
a representative-workload speedup. These are CMake observations, not Bazel parity.


## Validation and limits

Uninstrumented CMake Release checks passed:

| Check | Candidate |
|---|---:|
| Full SV constructor/intent executable | 1,370 passed |
| Full VHDL constructor executable | 269 passed |
| HDL primitive/helper executable | 15 passed |
| `SNLUtilsTest.*` | 11 passed |
| Focused serialization CTest suite | 365 passed |
| CTest `determinism` contract | passed |
| Latest common determinism script against each performance archive | 47 passed each |

The native test build used the same source/dependencies/compiler and Python
3.14 with `BUILD_NAJA_PYTHON=OFF` to expose the frontend targets. CTest's
`determinism` label includes setup and the cross-process Python contract.
The latest common archive checks compare live IDs/names/connectivity,
Verilog/DOT, NajaIF bytes, round trips and cold reloads within **each** variant.
They include the new packed replay and scheduling/loop fallback fixtures.
Within-variant determinism does not imply identical IDs/snapshots between
variants; the exact-build snapshot gate is unchanged.

Independent synthetic tests exercise widths 2/4/7/11, both range directions
with nonzero bounds, signed/unsigned assignment extension, declaration order,
whole-vector folding/source origins and absence of abandoned intermediate nets.
The boundary fixture exhaustively checks binary inputs, dependent reads,
sharing, partial updates, partial/permuted common arms, nested exits and
multiple changed runs with identical positions. Existing helper tests now
exercise both common-arm orientations, equal/distinct guards, full/partly
identical arms and widths 1/2/7. A packed unsupported-effect test preserves
eager diagnostics. Icarus compares the packed fixture's emitted model with
its independent behavioral source for all 256 four-state data combinations
and 16 binary-control combinations (4,096 cases); the full suite also retains
the scalar four-state test. Case-comparison X/Z limitations and their
diagnostics are unchanged. No broad external-design functional/four-state
proof was attempted.

Source-based Bazel dependency selection passes with matching source revisions.
The same six targets requested in #542 were attempted with default source
configuration and `--jobs=3 --test_output=errors`: SV/VHDL constructors, HDL
helper, Cap'n Proto serialization, kernel and najaeda determinism. **Zero tests
executed.** The local Xcode toolchain rejects an absolute include of
`MacOSX.sdk/SDKSettings.json` while compiling `thirdparty/sha256/sha-256.c`,
reproducing #542's blocker. No SDK/build-system changes were attempted;
CMake/Bazel behavioral parity remains unverified. Registered/BCR validation
was not attempted and readiness is not claimed.

An initial overly broad CTest invocation in the reused Python build also
selected stale unrelated CLI registrations whose executables were unbuilt;
it had six failures. The explicitly scoped serialization directory subsequently
passed all 365 tests in the uninstrumented native build. An initial Python-build
`-L determinism` invocation had no registrations; the proper native CTest
contract and both archived-build contracts passed. These failed setup/scope
attempts are retained in the local logs, not represented as successful checks.
REUSE lint and `git diff --check` passed after adding the CSV licensing coverage.

## Smallest evidence-supported next step

Retain both frontend cleanup calls and the public utility. The remaining
**168 direct removals / 954 bits** across Ibex and AES preclude removal.
The smallest separate SV investigation is the **26 case-output→case-output
pairs / 135 bits** whose complete output vectors differ: establish whether
owning case-output batching can preserve common changed runs without crossing
read, nested-exit or sharing boundaries. This is a separate ownership/shape
proof, not authorization for a broad planner. VHDL's seven private nested
conditional-expression pairs remain a distinct candidate; do not force the
SV replay mechanism into VHDL. Alias/transitive, scheduling and memory paths
remain independent work.

Evidence: [three-repeat counts](case-replay-packed-counts.csv),
[per-design pass totals](case-replay-packed-designs.csv),
[classified direct removals](case-replay-packed-merges.csv), and
[all retained timing samples](case-replay-packed-timings.csv).
Raw logs, failed preflights, source/instrumentation copies and patches, archive
hashes, caches, input manifests and the declared protocol remain local outside
the tracked tree. Production instrumentation is not part of this change.
