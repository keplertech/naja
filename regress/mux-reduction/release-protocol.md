<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Release timing protocol — declared 2026-11-15

Compare `4f5ba31c` (before #535), `f3cf91ce` (#535), and `f6388fae` (#536).
The production differences are the two mux-buffer changes. Use the same source
dependency revisions, Apple clang, Python 3.14.8, CMake Release flags and six
build workers for all three. Build each without allocation instrumentation in
the same isolated build directory, then archive its Python module and all Naja
runtime libraries. Verify the producer hash and loaded Naja library locations
in every fresh process. Complete all builds before timing begins.

Reuse the nine public workloads in the original study and four independent
mixed-vector chain cases from the #536 study. Keep input order, options, top,
source metadata and normal cleanup fixed. No post-load optimizer or DLE.
Hash the inputs, including the Ibex file list's transitive sources and include
files, before sampling and check them again afterward.

Exclude one warm-up per workload/revision, retaining its log. Collect 18
retained fresh-process samples per workload/revision: 702 retained samples in
total. Each block visits all workloads in a seeded shuffled order. For each
workload, run the three revisions consecutively. Use each of the six possible
revision orders three times, with the permutation schedule shuffled in advance.
Seed: `536535`. Run sequentially, with no concurrent study builds. Keep every
sample; stop and report a failure rather than silently skipping or replacing it.

Time only the raw NLDB load call with `perf_counter_ns`, excluding import,
universe creation, statistics, identity checks and process startup. Capture
`getrusage(RUSAGE_SELF).ru_maxrss` immediately after the load, before statistics
or dynamic-library auditing. On macOS this is process peak RSS in bytes,
including Python startup and import; it is not live netlist memory. Statistics
are collected afterward and must repeat within each workload/revision group.

Report medians and full ranges per revision. Compare final versus baseline,
and #536 versus #535, using paired samples from the same block. Report the
median paired time ratio and median paired RSS difference with percentile
bootstrap 95% intervals (10,000 paired resamples, fixed seed). Intervals are
exploratory and unadjusted for multiple workloads/comparisons; do not turn
them into a broad performance claim. Report regressions and inconclusive cases
alongside improvements. FIR is an allocation-free negative control. Do not
aggregate synthetic cases into a representative-workload speedup.

Retain raw process logs, warm-ups, environment, build configurations, binary
hashes, input hashes and the execution plan locally. Commit sanitized numeric
samples and a report without machine-local paths or design contents. This is a
CMake Release performance study, not BCR validation or a new equivalence proof.
