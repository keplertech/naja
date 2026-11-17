<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Case-local SV replay scalars

The [2026-11-17 measurement follow-up](#measurement-follow-up--2026-11-17)
measures #541 against its parent: all 207 target allocations are avoided, but
cleanup is still required in both frontends. The implementation notes below
record the earlier 2026-11-16 verification.

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

## Measurement follow-up — 2026-11-17

**#541 avoids all 207 identified scalar case-arm replay allocations on this
corpus. Keep both frontend cleanup calls enabled:** cleanup still directly
removes 176 muxes / 744 output bits from Ibex and 18 / 340 from AES VHDL.
The following are newly measured results, separate from the historical synthetic
40→32 bitnet comparison above and the preceding removal-readiness audit.
No additional optimization, cleanup removal or primitive change was made.

### Revisions and reproducible setup

Fresh `git fetch origin` selected `1689438c423cedf7ecfcf45f5801321025410bd8`
(#541); its parent is `49bfc050c50a9481442112033b665a024c2bd0cd`.
Latest main is exactly #541: there are no later changes to measure separately.
Between the historical audit's `c968cf72` and #541's parent, production sources,
dependency pins, CMake and Bazel configuration are unchanged. The producer
identity differs, so those historical binaries were not used as the baseline.
New builds were made from the exact parent and merged revisions.

The original checkout was preserved. A feature worktree based on fetched main
holds this report; a separate detached worktree/build directory was reused
sequentially for both revisions and their diagnostic variants. The dependencies
were selected from clean source trees at these identical revisions:

| Dependency | Revision |
|---|---|
| slang | `545f88b2ed3a2d20325fbd17413af177a6bca72f` |
| naja-verilog | `4459520a51dd246ce81bc72dd0ca8b1ed75518e6` |
| naja-if | `e1ce72c5e8eeb7803fae0bd96a267d5e3e91146d` |
| googletest | `063de7e9578f82b369302001269680b4b1553359` |
| cpptrace | `3db8da80111171c219ab5839905771386bee06b3` |

Apple M1 (MacBookPro17,1), eight cores, 16 GiB RAM, arm64 macOS 27.0.1
(26A434), Apple clang 21.0.0 (`clang-2100.3.34.2`), CMake 4.4.4,
Python 3.14.8, Bazel 8.6.0. Both performance builds used CMake Release,
`-O3 -DNDEBUG`, six build workers, `BUILD_NAJA_PYTHON=ON`,
`BUILD_NAJA_SERIALIZATION_TESTS=ON`; sanitizers, coverage and
`ENABLE_SV_CONSTRUCTOR_PERF_REPORT` were off. Configure explicitly with the
same Python interpreter for both `Python3_EXECUTABLE` and `Python_EXECUTABLE`.
Reconfigure at every revision switch, build the `naja` module as well as its
runtime libraries and serialization executable, then archive them together.
Each worker verifies `getGitHash()`, the selected module path and every loaded
Naja dylib path. Binary hashes and CMake caches are retained locally.

Reuse the exact 13 cases/configurations from the [original corpus](README.md)
and [Release protocol](release-protocol.md): default public CHERIoT Ibex, CI AES
decrypt, generated FIR, overwrite/dead-RHS/repeated pairs, and both mixed-vector
orientations in both HDLs. All 240 input/source/include hashes match the prior
audit and are checked again after sampling. SV uses `keep_assigns=True`, the
same file list and `--top`; VHDL uses the same ordered sources and top. Diagnostics
report routing is fixed. Both cleanup calls stay enabled, with no post-load
optimizer or DLE. No confidential inputs were used.

Temporary diagnostic instrumentation counts canonical mux creation in
`SNLInstance::commonPostCreate`, including clones, and sums actual output widths.
At each `mergeRepeatedMuxSelections` entry and exit it counts live muxes/widths;
it separately records each inner destruction, its width, common-arm orientation,
guard equality, creation call site, original requested width and materialization
boundary. SV replay tags record scalar type and a scoped case-evaluation ordinal,
so whole-scalar same-case chains can be distinguished from vector and other
boundaries without using pointer order to drive any construction. Read-only
compatibility probes before and after alias collapse retain the original cleanup.

Run three fresh processes per case/revision/variant. All three diagnostic counts
repeat, and all diagnostic final folded and expanded structural totals match
uninstrumented runs. On every load, cumulative mux creation equals cleanup-entry
live totals, and cleanup-exit totals equal final folded mux totals. Allocation
counters are never multiplied by hierarchy reuse. Instrumentation patches,
source copies, raw logs and exact manifests remain in the local evidence bundle;
production source changes were restored before validation/performance work.

### Allocations and remaining cleanup

Cells are **mux creation events or live instances / summed output bits**.
These are direct counters, not estimates from final net counts.

| Case | Parent created / before cleanup | #541 created / before cleanup | Parent direct removal | #541 direct removal | Final folded, both |
|---|---:|---:|---:|---:|---:|
| Ibex SV | 2,115 / 15,442 | **1,908 / 15,235** | 383 / 951 | **176 / 744** | 1,732 / 14,491 |
| AES VHDL | 273 / 2,906 | 273 / 2,906 | 18 / 340 | **18 / 340** | 255 / 2,566 |
| Repeated, each HDL | 1 / 32 | 1 / 32 | 0 / 0 | 0 / 0 | 1 / 32 |
| Mixed, each HDL/orientation | 2 / 4 | 2 / 4 | 0 / 0 | 0 / 0 | 2 / 4 |
| Dead RHS, each HDL | 32 / 1,024 | 32 / 1,024 | 0 / 0 | 0 / 0 | 32 / 1,024 |
| Overwrite SV; VHDL | 0 / 0; 1 / 1 | 0 / 0; 1 / 1 | 0 / 0 | 0 / 0 | 0 / 0; 1 / 1 |
| FIR VHDL | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |

Thus Ibex saves **207 mux allocations (9.79%) and 207 output bits (1.34%)**.
All 207 baseline pairs have one-bit whole requests, scalar types and the same
case-evaluation ordinal. After #541, none remains in that class: **207/207
(100%) of the audit target is covered**. The residual 26 case→case replay pairs
all request five-bit vectors; #541 intentionally excludes them. The other 34
replay→replay removals involve `if` merges. This is corpus-specific coverage,
not proof that every scalar replay shape is buffered.

| Ibex inner → outer path/boundary | Parent removals / bits | #541 removals / bits |
|---|---:|---:|
| Case-arm replay → case-arm replay, scalar | 207 / 207 | **0 / 0** |
| Case-arm replay → case-arm replay, five-bit vector | 26 / 130 | 26 / 130 |
| Other replay → replay, involving `if` | 34 / 148 | 34 / 148 |
| Eager conditional expression → replay | 14 / 14 | 14 / 14, now at scalar case exit |
| Owning-`if` pending result → case-output flush | 48 / 239 | 48 / 239 |
| Case-output flush → pending result | 12 / 17 | 12 / 17 |
| Case output → case output, differing whole vectors | 26 / 135 | 26 / 135 |
| Pending → pending, owning merge/incompatible selection | 10 / 54 | 10 / 54 |
| Eager conditional expression → case output | 3 / 4 | 3 / 4 |
| Into scheduled assignment or storage enable | 3 / 3 | 3 / 3 |

Expression values remain eager: the 14 expression→replay opportunities move to
the scalar accumulator's case-exit materialization boundary rather than vanish.
No additional residual count/width class changes. AES still has seven nested
conditional-expression merges / 224 bits, five eager scheduled merges / 98,
three pending→pending / 15, one pending→scheduled / one, and two memory-write-enable
merges / two. The four pending statement-list exits and complex scheduled fallback
boundaries remain. No eligibility predicates were relaxed.

For Ibex, 171 actual residual removals are already visible before alias collapse,
two one-bit pairs become visible only after it, and three appear after earlier
pass merges (parent: 378, two, three). The probes do not simulate transitive
closure, so the last three pairs' alias dependence remains unresolved.

### Final structure, independently counted

| View | Parent muxes / bits | #541 muxes / bits | Parent total instances | #541 total instances |
|---|---:|---:|---:|---:|
| Ibex folded | 1,732 / 14,491 | 1,732 / 14,491 | 28,853 | 28,746 |
| Ibex hierarchy-expanded | 1,739 / 14,690 | 1,739 / 14,690 | 29,081 | 28,974 |
| AES folded and expanded | 255 / 2,566 | 255 / 2,566 | 2,739 | 2,739 |

The Ibex final model histogram differs only in `or_2`: 5,157→5,050 folded.
The measured 107-instance reduction is a separate structural result; it is not
inferred from mux savings. Construction-time guard gates use the existing
frontend simplification/cache path, whereas late cleanup creates guards directly.
Folded Ibex nets/bitnets fall 18,045/55,927→17,731/55,613; expanded totals fall
18,119/56,778→17,805/56,464. All other corpus final totals are unchanged.
None of these structural measurements establishes lower RSS or faster loading.

### Uninstrumented loading time and peak RSS

The declared protocol uses **12 balanced paired blocks**, seed `541207`, with
six parent-first and six #541-first blocks, a seeded shuffled block order and
shuffled workload order per block. One warm-up per workload/revision is excluded
(26 logged warm-ups). **All 312 subsequent fresh-process samples are retained**;
none was dropped, replaced or excluded, and no performance worker failed.
All builds and tests completed before sampling. Diagnostic libraries are in
separate archives; hashes confirm the performance archives remained unchanged.

Reproduce with the fixed case manifest and `parent`/`pr541` archive mapping
described above:

```sh
python3 regress/mux-reduction/release_measure.py \
  --cases CASES --archives ARCHIVES --output OUT --blocks 12 --seed 541207
```

The existing runner records the execution plan,
checks producer/library identity and fixed structural totals, and hashes inputs
before and after. It times only the raw NLDB load with `perf_counter_ns`, excluding
import, universe creation, statistics and startup. `ru_maxrss` is captured before
statistics; on macOS it is peak process bytes including Python/import, not live
netlist memory. Processes run sequentially; CPU frequency and unrelated OS work
are uncontrolled. Input and archive hashes remain unchanged.

Each row reports time medians **[full min–max] ms**, median paired time ratio
change, and median paired RSS difference. Intervals are percentile **95% paired
bootstrap** intervals from 10,000 resamples with seed `541207`, using the existing
`release_summary.bootstrap_median` helper on the 12 paired ratios/differences.
A ratio of medians is not substituted for the median paired ratio. Intervals are
exploratory, unadjusted for multiple comparisons and conditional on this short
single-machine campaign. All workloads are reported; synthetic timings are not
aggregated into a representative-workload speedup.

| Workload | Parent ms [range] | #541 ms [range] | Paired time change [95% interval] | Paired RSS MiB [95% interval] |
|---|---:|---:|---:|---:|
| aes | 85.839 [84.788–98.001] | 86.757 [84.095–99.286] | +0.52% [-1.05, +3.13] | -0.031 [-0.164, +0.102] |
| common_false_sv | 2.109 [2.005–4.015] | 2.079 [1.996–2.326] | -1.46% [-5.38, +0.11] | +0.039 [-0.133, +0.188] |
| common_false_vhd | 1.985 [1.923–2.633] | 1.967 [1.888–2.428] | +0.72% [-3.20, +2.58] | +0.000 [-0.109, +0.047] |
| common_true_sv | 2.668 [2.567–2.885] | 2.679 [2.629–3.292] | +1.92% [-0.61, +4.38] | -0.023 [-0.141, +0.102] |
| common_true_vhd | 2.404 [2.322–2.722] | 2.477 [2.366–2.695] | +0.95% [-6.16, +5.66] | -0.062 [-0.148, +0.117] |
| dead_rhs_sv | 4.399 [4.221–4.894] | 4.293 [4.152–9.190] | -1.22% [-3.04, +1.99] | +0.070 [+0.016, +0.258] |
| dead_rhs_vhd | 8.210 [8.070–16.980] | 8.238 [8.110–8.689] | -0.07% [-3.40, +1.36] | +0.094 [-0.062, +0.172] |
| fir | 2.042 [1.980–4.484] | 2.101 [1.936–3.143] | -0.50% [-3.07, +5.01] | -0.008 [-0.133, +0.094] |
| ibex | 260.165 [252.109–363.924] | 255.250 [250.597–261.911] | -1.64% [-3.44, -0.86] | +0.086 [-0.141, +0.156] |
| repeated_sv | 3.816 [3.684–3.990] | 3.816 [3.687–5.579] | +0.76% [-2.81, +3.82] | +0.094 [-0.008, +0.133] |
| repeated_vhd | 3.189 [3.078–5.221] | 3.182 [3.112–3.491] | -0.50% [-2.62, +2.15] | -0.023 [-0.156, +0.047] |
| scheduled_overwrite_sv | 0.985 [0.906–1.124] | 0.955 [0.917–2.384] | -2.92% [-6.63, +3.34] | +0.023 [-0.047, +0.070] |
| scheduled_overwrite_vhd | 1.298 [1.202–1.566] | 1.214 [1.176–1.551] | -1.91% [-7.46, +0.16] | +0.062 [-0.156, +0.156] |

Representative peak process RSS, median **[full min–max] MiB**:

| Workload | Parent | #541 |
|---|---:|---:|
| Ibex | 99.297 [99.141–99.641] | 99.352 [99.172–99.484] |
| AES | 43.367 [43.188–43.578] | 43.352 [43.047–43.547] |
| FIR | 28.812 [28.578–29.016] | 28.812 [28.656–29.125] |

Ibex shows a modest local loading-time reduction, **−1.64% [−3.44, −0.86]**;
its parent range includes a retained 363.924 ms observation. This is not evidence
of a general speedup. AES and allocation-free FIR control intervals include zero;
so do every other workload's time intervals. No representative RSS reduction is
established. Dead-RHS SV shows a small RSS increase, +0.070 MiB [+0.016, +0.258],
despite unchanged structural totals. Other RSS intervals include zero. Process
RSS granularity/noise, uncontrolled OS conditions, the small sample count and
multiple comparisons limit interpretation. These are newly measured CMake costs,
not projections from mux counts or reuse of historical timings.

### Validation and limits

Uninstrumented CMake builds passed:

| Check | Parent | #541 |
|---|---:|---:|
| Full SV constructor/intent executable | 1,363 passed | 1,367 passed |
| Full VHDL constructor executable | 269 passed | 269 passed |
| HDL primitive/helper executable | 15 passed | 15 passed |
| `SNLUtilsTest.*`, including mux cleanup | 11 passed | 11 passed |
| Serialization CTest suite | 365 passed | 365 passed |
| CTest `determinism` contract | 44 Python tests passed | 45 Python tests passed |
| Common latest determinism script against archived builds | 45 passed | 45 passed |

Native frontend/helper checks used the same Release toolchain/dependencies with
`BUILD_NAJA_PYTHON=OFF` to expose those CMake test targets. The performance archives
retain `ON`; they are not these test executables. Commands were the complete
`snlSVConstructorTests`, `snlVHDLConstructorTests`, `snlRTLPrimitivesTests`
executables, `snlTests --gtest_filter=SNLUtilsTest.*`, serialization `ctest --output-on-failure` in the Python build, and:

```sh
ctest --test-dir BUILD -L determinism --no-tests=error --output-on-failure
```

The common archived-build campaign also runs the newly added scalar fixture
against the parent. It compares live IDs/names/connectivity, Verilog/DOT,
NajaIF bytes, snapshot round trips and fresh-process reloads within each variant.
The full SV run includes the synthetic four-state simulation test; no broad
external-design functional/four-state equivalence campaign was performed.
Determinism within a revision does not imply equal IDs or snapshot bytes between
revisions, and the exact-build snapshot gate was left intact.

Source-based Bazel's dependency-selection checker passed. The attempted default
source configuration selected the same local naja-if/naja-verilog revisions and
the matching registered slang revision. The requested targets were:

```sh
bazel test --jobs=3 --test_output=errors \
  //test/nl/formats/systemverilog/frontend:snlSVConstructorTests \
  //test/nl/formats/vhdl:snlVHDLConstructorTests \
  //test/nl/formats/hdl:snlRTLPrimitivesTests \
  //test/nl/snl/serialization/capnp:snlCapnPTests \
  //test/nl/snl/kernel:snlTests \
  //test/najaeda:najaeda_determinism_tests
```

**Bazel executed zero tests.** Compilation of `thirdparty/sha256/sha-256.c`
failed the local toolchain's absolute-include check for
`MacOSX.sdk/SDKSettings.json` under the Xcode SDK. No build-system or SDK fix
was attempted. Thus source-based Bazel validation and CMake/Bazel behavioral
parity remain blocked, not passed. Registered/BCR validation was not attempted.
This measurement-only change does not establish BCR readiness. REUSE lint and
`git diff --check` passed for the report and CSV additions.

### Recommendation — stop after measurement

Keep both frontend cleanup calls. #541 fully covers the measured 207-opportunity
slice, but leaves 194 direct removals / 1,084 bits across Ibex and AES. The
smallest evidenced SV follow-up is a separately scoped eligibility/proof exercise
for the **26 five-bit, same-case replay pairs**: determine whether complete private
packed values can use the same case-local ownership and exit-flush contract.
Preserve eager reads/diagnostics, partial-update fallbacks, shape/index semantics,
source origins and declaration order. Do not infer that a wider accumulator is
safe merely because the helper accepts vectors. No such change is implemented
or authorized by this report.

That candidate covers only 26 of 176 remaining SV removals and none of VHDL's 18.
Expression materialization, owning-`if` and case-output boundaries, scheduled and
memory-enable paths, and alias/transitive cases remain separate work. Removal
still requires zero direct cleanup across both frontends on a broader supported
corpus plus boundary, semantic, serialization, determinism and build-path checks.
No broad value planner, dead-RHS work, storage policy or primitive changes are
needed for this measurement conclusion; none was resumed.

Evidence: [three-repeat allocation/final totals](case-replay-counts.csv),
[per-design pass totals](case-replay-designs.csv),
[classified direct removals](case-replay-merges.csv), and
[every accepted performance sample](case-replay-timings.csv).
The local evidence bundle retains the declared protocol, execution plan, warm-up
and process logs, input/binary hashes, CMake caches, independent exit-recount
patches, source copies and archives. Production instrumentation is not part of
this tracked report change.
