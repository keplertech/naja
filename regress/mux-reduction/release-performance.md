<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Controlled Release performance — 2026-11-15

**The accumulated mux reductions have clear benefits on the targeted stress
cases, a small observed AES benefit, and no demonstrated Ibex load-time benefit.**
This completes the Release-measurement phase after #535 and #536. It introduces
no frontend or primitive changes.

Compared revisions: `4f5ba31c` (before #535), `f3cf91ce` (#535), and `f6388fae`
(#536). The only production changes between the endpoints are the two mux
optimizations. All three were built uninstrumented with identical CMake Release
settings (`-O3 -DNDEBUG`), Apple clang 21, Python 3.14.8 and the same dependency
checkouts, in one isolated build directory; each module and all Naja runtimes
were archived before switching revisions. Every timed process checked its
producer identity and verified that all loaded Naja libraries came from its
selected archive.

The [predeclared protocol](release-protocol.md) fixes 13 public workloads,
18 paired blocks and all six revision orders repeated three times. Workload
order is seeded and shuffled per block. There are **702 retained fresh-process
samples**, plus 39 excluded warm-ups, with no failed, omitted or replaced
samples. Inputs matched the prior study's hashes; all source/include hashes
also matched before and after this run. No study build or validation job ran
during sampling. Final structural counts repeated within every group.

Platform: Apple M1, 8 CPU cores, 16 GiB RAM, arm64 macOS 27.0.1, CMake 4.4.4.
The machine reported battery power at 92% at both recorded endpoints. CPU
frequency and unrelated OS activity were not pinned. This is one machine and
one balanced campaign, not a multi-platform or broad workload survey.

## Findings

- **Ibex:** final median 481.490 ms versus 481.325 ms before #535. The median
  paired change is -0.11%, with a 95% interval of [-0.38%, +0.48%]. Neither
  this time interval nor the final paired RSS interval establishes a benefit.
  Reducing mux allocations from 2,158 to 2,115 did not produce a measurable
  load-time improvement here.
- **AES:** final median 84.331 ms versus 85.308 ms, with a paired change of
  -1.42% [-1.76%, -0.38%]. Paired peak RSS falls 1.445 MiB
  [-1.562, -1.328]. The benefit is principally associated with #535;
  #536's incremental AES intervals include zero.
- **Repeated-selection stress:** final paired load time falls 17.61% in SV
  and 26.22% in VHDL, with RSS reductions of 0.625 and 0.703 MiB. #535
  supplies the gain; #536's incremental time intervals include zero.
- **Mixed-vector stress:** #536 reduces paired time versus #535 by 1.53%
  (common-true SV), 5.82% (common-false SV), 12.63% (common-true VHDL),
  and 14.05% (common-false VHDL). All four time intervals exclude zero.
  These cases previously allocated 64 muxes and now allocate two.
- **Regressions and controls:** #535 alone raises the mixed common-true VHDL
  paired time by 3.43% [+0.89%, +4.45%]; #536 more than reverses that case.
  #535 also has positive RSS point estimates in both SV mixed cases; only
  the common-false interval excludes zero. #536 reverses those point estimates.
  Dead-RHS cases show no clear time or RSS improvement. The final
  FIR negative-control time interval includes zero. Small final positive
  point estimates in unchanged workloads are retained below.

Intervals are exploratory, unadjusted 95% percentile bootstrap intervals for
median paired values (10,000 resamples), not family-wise significance tests.
Time changes are medians of within-block ratios, **not ratios of the separate
revision medians**; their small point estimates can have different signs.
Negative changes mean less time or memory. Do not aggregate the synthetic
cases into a representative-design speedup.

## Loader time

Median [minimum–maximum] milliseconds, all 18 retained samples per cell.
Only the raw NLDB load call is timed; import, universe creation, statistics,
identity auditing and process startup are outside the interval.

| Workload | Before #535 | #535 | #536 |
|---|---:|---:|---:|
| Ibex SV | 481.325 [476.954–501.705] | 482.415 [476.068–499.205] | 481.490 [477.616–512.043] |
| AES VHDL | 85.308 [84.457–87.110] | 84.021 [83.481–86.257] | 84.331 [83.644–85.670] |
| FIR VHDL | 1.962 [1.931–2.146] | 2.006 [1.920–2.273] | 2.000 [1.906–2.453] |
| Overwrite SV | 1.082 [1.010–1.167] | 1.104 [1.026–1.281] | 1.079 [0.995–4.066] |
| Overwrite VHDL | 1.217 [1.173–1.377] | 1.188 [1.171–1.346] | 1.202 [1.166–1.338] |
| Repeated SV | 6.141 [6.034–7.227] | 5.051 [4.958–5.354] | 5.023 [4.973–17.009] |
| Repeated VHDL | 4.242 [4.195–4.395] | 3.098 [3.066–3.250] | 3.119 [3.050–3.548] |
| Dead RHS SV | 8.250 [8.140–8.752] | 8.274 [8.090–8.988] | 8.258 [8.158–8.958] |
| Dead RHS VHDL | 9.333 [9.259–9.494] | 9.384 [9.234–9.543] | 9.368 [9.281–9.875] |
| Mixed common-true SV | 13.066 [12.983–14.004] | 13.097 [12.981–13.373] | 12.886 [12.784–13.433] |
| Mixed common-true VHDL | 2.657 [2.625–2.874] | 2.751 [2.654–2.880] | 2.375 [2.312–2.529] |
| Mixed common-false SV | 3.838 [3.723–4.179] | 3.863 [3.744–4.138] | 3.631 [3.520–3.913] |
| Mixed common-false VHDL | 2.298 [2.217–2.530] | 2.284 [2.243–2.427] | 1.969 [1.889–2.297] |

## Peak process RSS through loading

Median [minimum–maximum] MiB. `ru_maxrss` is sampled immediately after loading,
before statistics and library auditing. It includes Python startup and import;
it is not live netlist memory. Small control-workload RSS shifts can reflect
runtime/code layout and allocator noise. Compare these values within this
campaign, not directly with earlier workers that had different imports or
sampled RSS after statistics.

| Workload | Before #535 | #535 | #536 |
|---|---:|---:|---:|
| Ibex SV | 99.406 [99.250–99.781] | 99.273 [99.109–99.703] | 99.344 [99.203–99.453] |
| AES VHDL | 44.773 [44.359–44.938] | 43.219 [42.922–43.469] | 43.289 [43.078–43.453] |
| FIR VHDL | 28.891 [28.719–29.172] | 28.875 [28.641–29.031] | 28.820 [28.672–29.141] |
| Overwrite SV | 30.109 [29.984–30.344] | 30.008 [29.875–30.391] | 30.031 [29.828–30.266] |
| Overwrite VHDL | 28.367 [28.234–28.516] | 28.250 [28.047–28.656] | 28.258 [28.109–28.578] |
| Repeated SV | 31.125 [31.016–31.375] | 30.500 [30.328–30.734] | 30.500 [30.328–30.703] |
| Repeated VHDL | 29.406 [29.344–29.703] | 28.734 [28.547–29.047] | 28.703 [28.609–28.984] |
| Dead RHS SV | 31.602 [31.453–31.922] | 31.586 [31.438–31.828] | 31.578 [31.484–31.812] |
| Dead RHS VHDL | 31.086 [30.938–31.344] | 30.984 [30.891–31.250] | 31.070 [30.875–31.266] |
| Mixed common-true SV | 30.594 [30.359–30.875] | 30.641 [30.406–30.969] | 30.500 [30.328–30.703] |
| Mixed common-true VHDL | 29.172 [28.891–29.312] | 29.172 [28.969–29.438] | 28.984 [28.906–29.219] |
| Mixed common-false SV | 30.477 [30.391–30.812] | 30.516 [30.297–30.875] | 30.352 [30.188–30.609] |
| Mixed common-false VHDL | 28.867 [28.703–29.219] | 28.883 [28.703–29.219] | 28.727 [28.547–29.031] |

## Final versus before #535

Median paired change [95% interval].

| Workload | Loader time, % | Peak RSS, MiB |
|---|---:|---:|
| Ibex SV | -0.11 [-0.38, +0.48] | -0.086 [-0.188, +0.008] |
| AES VHDL | -1.42 [-1.76, -0.38] | -1.445 [-1.562, -1.328] |
| FIR VHDL | +0.44 [-1.35, +4.39] | -0.078 [-0.250, +0.000] |
| Overwrite SV | +0.21 [-5.29, +4.50] | -0.109 [-0.156, +0.047] |
| Overwrite VHDL | +0.50 [-2.00, +1.35] | -0.117 [-0.148, -0.008] |
| Repeated SV | -17.61 [-19.26, -16.93] | -0.625 [-0.656, -0.578] |
| Repeated VHDL | -26.22 [-26.85, -24.67] | -0.703 [-0.766, -0.648] |
| Dead RHS SV | +0.65 [-0.55, +1.35] | +0.039 [-0.117, +0.086] |
| Dead RHS VHDL | +0.26 [-0.07, +1.37] | -0.039 [-0.156, +0.031] |
| Mixed common-true SV | -1.28 [-1.69, -0.71] | -0.109 [-0.188, -0.047] |
| Mixed common-true VHDL | -11.19 [-14.97, -7.70] | -0.148 [-0.242, +0.031] |
| Mixed common-false SV | -5.72 [-6.53, -3.52] | -0.102 [-0.195, -0.062] |
| Mixed common-false VHDL | -14.67 [-17.35, -12.09] | -0.195 [-0.250, -0.125] |

## #536 versus #535

Median paired change [95% interval].

| Workload | Loader time, % | Peak RSS, MiB |
|---|---:|---:|
| Ibex SV | -0.23 [-0.59, +0.37] | +0.016 [-0.047, +0.172] |
| AES VHDL | -0.04 [-0.34, +0.70] | +0.086 [-0.055, +0.234] |
| FIR VHDL | -1.19 [-6.22, +4.05] | -0.109 [-0.172, +0.117] |
| Overwrite SV | -1.44 [-7.26, +0.96] | -0.008 [-0.133, +0.078] |
| Overwrite VHDL | -0.36 [-2.00, +3.49] | +0.023 [-0.148, +0.141] |
| Repeated SV | -0.11 [-0.58, +0.91] | +0.031 [-0.078, +0.078] |
| Repeated VHDL | +1.25 [-0.20, +2.65] | -0.047 [-0.102, +0.078] |
| Dead RHS SV | +0.54 [-0.64, +1.39] | -0.062 [-0.133, +0.102] |
| Dead RHS VHDL | -0.19 [-1.10, +1.63] | +0.070 [+0.000, +0.148] |
| Mixed common-true SV | -1.53 [-2.23, -1.10] | -0.172 [-0.289, -0.055] |
| Mixed common-true VHDL | -12.63 [-14.27, -11.59] | -0.172 [-0.273, -0.047] |
| Mixed common-false SV | -5.82 [-8.12, -4.54] | -0.164 [-0.281, -0.125] |
| Mixed common-false VHDL | -14.05 [-16.39, -12.23] | -0.234 [-0.328, -0.109] |

## Verification and reproducibility

Each archived revision passed all 365 native serialization tests and nine
focused determinism tests, including whole and mixed-vector selections, live
state, dumps, snapshots and reloads in fresh processes. This verifies the measurement
builds on those contracts; it is not a new functional-equivalence proof for the
external benchmarks. All three caches agree on compiler, Release flags,
Python interpreter, feature settings, disabled sanitizers/coverage and disabled
frontend performance instrumentation.

The initial reused cache referred to Python 3.15. Its first explicit configure
failed while switching to 3.14; the build's subsequent CMake regeneration
succeeded before compilation. The first multi-target make invocation then
could not see the newly generated native-test target; a separate invocation
built it successfully. Both setup logs are retained. All three final archives
use Python 3.14.8; these setup events preceded validation and every sample.

[release-samples.csv](release-samples.csv) contains every retained numeric
sample and final folded structural count. Machine-local input/configuration
files, raw logs (including warm-ups), binary hashes, cache files and the
execution plan remain local. No RTL or archived binaries are added here.
The corpus provenance is described in [the original study](README.md) and
[the mixed-vector study](identical-bits.md).

The scripts accept local cases and archive manifests; their formats are in
[release_measure.py](release_measure.py). After preparing the three archives:

```sh
python3 release_measure.py --cases cases.json --archives archives.json --output run
python3 release_summary.py run/samples.csv --output summary.json
```

The measurement driver requires macOS for byte-valued RSS and dyld auditing.
The summary script uses only the Python standard library and reproduces the
paired results from the committed CSV. This is a CMake performance study;
Bazel/BCR performance or readiness is not inferred from it.

## Next phase

Keep the two targeted optimizations and their correctness coverage. Their
benefit is real on the stress patterns and modest on the sampled AES workload,
but allocation counts alone do not justify a wider value planner. Before
extending buffering further for Ibex, profile its loader CPU and allocation
costs to identify which construction paths dominate the approximately 481 ms
load. Use diagnostic instrumentation separately from these uninstrumented
timings. A second platform or campaign would be needed to generalize the small
representative-workload effect.
