<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Mux construction measurement — 2026-11-13

**#525 eliminates overwritten scheduled selections, but changes no allocation counts on the representative workloads sampled here. The next evidenced opportunity is private common-arm mux chains constructed before cleanup collapses them.** No next optimization was implemented.

Revisions: main at `ad198544`, #525 at `a8dadccb`, and its parent `c7ebdfc4`. Measurements used an isolated worktree and builds. No experimental planner/patches or historical RedMulE results were imported. `SNLValuePlan` is absent from inspected main.

The [predeclared protocol](PROTOCOL.md) uses identical Release CMake options and dependency pins, Apple clang 21, CMake 4.4.4, Python 3.14.8, arm64 macOS 27.0.1. One excluded warm-up and seven fresh-process samples per case/revision yielded **189 retained measurements**. Revision order main → #525 → parent creates temporal confounding. Load timing excludes import/statistics; peak RSS covers the entire process. Normal cleanup remained enabled, with no subsequent optimizer/DLE.

The fixed corpus includes the tracked #525 overwrite pair, independent 32-stage repeated-selection/dead-RHS stress pairs, default CHERIoT Ibex (`lowRISC/ibex@7a4697f578c67c550fcad5e5e3078415e1f088d7`, tracked regression file list), CI AES decrypt (`os-fpga/RTL_Benchmark@4eea4a7b479f55dfbf385df392cb362a100098d2`, hashes verified), and Naja's generated FIR fixture. This is two external families plus a filter fixture, not a broad workload survey.

Temporary counters in `SNLInstance::commonPostCreate` measured actual canonical mux creation events and summed output widths, including clones. Separate counters measured live muxes before `mergeRepeatedMuxSelections` and its removals. On every accepted load, cumulative allocations equal pre-cleanup live counts/widths. Structural counts repeat identically across all seven samples per group. These are folded allocation counts, not hierarchy-expanded estimates.

Cells are **mux instances / total output bits**. Final sizes include disconnected cones retained by normal loader cleanup.

| Workload | Parent allocated | #525 allocated | Main allocated | Parent final | #525 final | Main final |
|---|---:|---:|---:|---:|---:|---:|
| Overwrite SV | 2 / 8 | 0 / 0 | 0 / 0 | 1 / 4 | 0 / 0 | 0 / 0 |
| Overwrite VHDL | 3 / 9 | 1 / 1 | 1 / 1 | 2 / 5 | 1 / 1 | 1 / 1 |
| Repeated SV | 32 / 1024 | 32 / 1024 | 32 / 1024 | 1 / 32 | 1 / 32 | 1 / 32 |
| Repeated VHDL | 32 / 1024 | 32 / 1024 | 32 / 1024 | 1 / 32 | 1 / 32 | 1 / 32 |
| Dead RHS SV | 64 / 2048 | 32 / 1024 | 32 / 1024 | 64 / 2048 | 32 / 1024 | 32 / 1024 |
| Dead RHS VHDL | 64 / 2048 | 32 / 1024 | 32 / 1024 | 64 / 2048 | 32 / 1024 | 32 / 1024 |
| Ibex SV | 2164 / 15608 | 2164 / 15608 | 2158 / 15529 | 1738 / 14569 | 1738 / 14569 | 1733 / 14492 |
| AES VHDL | 305 / 3071 | 305 / 3071 | 305 / 3071 | 255 / 2566 | 255 / 2566 | 255 / 2566 |
| FIR VHDL | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |


#525 saves two 4-bit mux allocations per overwrite fixture and halves both dead-RHS cases from 64/2,048 to 32/1,024. It still lowers RHS expressions and guards: the remaining 32 RHS muxes are disconnected but retained. Parent/#525 storage-model histograms match in every case. SV retains its existing DFFE choice; VHDL retains its DFF-plus-enable-mux policy. No fresh functional-equivalence proof is claimed.

Parent/#525 representative allocation and final sizes are identical. Main's separate Ibex changes save six allocations/79 bits relative to #525 while increasing final total instances by 34; this is not #525's effect.

Timing is median **[min–max] milliseconds**; memory is median MiB. Full ranges for every case/metric are retained.

| Workload | Parent ms; MiB | #525 ms; MiB | Main ms; MiB |
|---|---:|---:|---:|
| Dead RHS SV | 9.64 [9.54–9.88]; 29.52 | 8.68 [8.42–10.63]; 28.97 | 9.23 [8.50–12.18]; 29.11 |
| Dead RHS VHDL | 11.51 [10.93–14.61]; 29.59 | 10.14 [9.87–10.37]; 28.64 | 9.87 [9.81–10.43]; 28.77 |
| Ibex SV | 493.00 [484.71–569.93]; 98.03 | 480.28 [479.16–480.49]; 97.94 | 504.61 [500.28–552.96]; 96.83 |
| AES VHDL | 88.06 [86.58–109.05]; 41.94 | 86.14 [85.74–86.81]; 42.42 | 88.34 [87.11–89.41]; 42.42 |
| FIR VHDL | 2.95 [2.78–4.02]; 26.31 | 2.42 [2.36–2.62]; 26.25 | 2.43 [2.35–3.11]; 26.48 |


Stress medians improve with #525; no representative speedup is established. Observed regressions include AES RSS +0.48 MiB parent→#525 despite unchanged allocations, and main Ibex median load +5.1% versus #525 while RSS falls. Fixed revision order and timing variation, including allocation-free FIR, prevent causal performance claims. All inputs were supported. Resolved download/import failures and an excluded build-overlap batch are retained in [setup deviations](protocol-deviations.md); there are no unresolved environmental blockers.

On main, cleanup removes **425 muxes/1,037 bits from Ibex** (19.7% of mux allocations; 6.7% of width) and **50/505 from AES** (16.4%; 16.4%). All use distinct selectors with a common data arm. Ibex decoder contributes 257/259, compressed decoder 70/359; AES key expansion contributes 30/475. Each repeated fixture removes 31/992. This is the largest demonstrated avoidable pattern here. It establishes allocation relevance, not a performance benefit from an unimplemented fix; distinct-selector collapses also create guard gates.

Final main Ibex is **1,733 muxes/14,492 bits, 28,852 instances, 18,086 nets folded**, versus **1,740/14,691, 29,080 instances, 18,160 nets expanded**. AES is **255/2,566, 2,743 instances, 2,834 nets** in both views; FIR has zero muxes and 156 instances. All revisions' final instance/net/bitnet sizes are in [samples.csv](samples.csv). Construction counts must not be multiplied by hierarchy reuse.

**Smallest proposed fix:** a statement-local pending-selection buffer for private branch-merge chains in both frontends. Keep RHS/guard lowering and diagnostics eager; accumulate adjacent, width-matched common-arm selections and combine guards with existing AND/OR identities before emitting a mux. Flush at reads, sharing, named destinations, replay boundaries, partial/permuted selections or uncertain ownership; cache only materialized results. Keep cleanup enabled. SV already batches adjacent case arms with identical whole result vectors (`pendingBits`/`pendingMatch`); extend the approach to remaining merge paths rather than proposing it again. VHDL `caseStatement`/`merge` provides the parallel entry point. This is a local mux-only buffer, not a general value planner.

Expected scope: a small common chain-reduction helper, two frontend adapters, focused tests. The 425/50 removals are upper bounds, not promised coverage for the first restricted slice. No new primitives or language support are required. Keep storage, clock/reset/enable policies and diagnostics unchanged; dead-RHS elimination remains separate.

Validation: rerun allocation measurements separately from final counts; test both common-arm orientations, equal/distinct guards, partial matches, shared/named results, slices/reordering and fallback boundaries. Preserve all folded source origins. Run supported four-state RTL/dump comparisons, old-state/overwrite diagnostics, DFFE/reset/enable checks, snapshots, cross-process determinism, and relevant CMake/Bazel frontend/kernel tests. Principal risks are pending-result escape, cache aliasing, SV replay dependencies, VHDL current/variable/scheduled semantics, origin loss and deterministic creation order. No simulation suite was rerun for this measurement-only study; no build-path/BCR parity claim is made.

The committed [CSV](samples.csv) contains every accepted measurement. Full raw logs, warm-ups, failed pilots, source hashes, measurement scripts and instrumented binaries are retained locally; they are not included in this report-only change. Source instrumentation was restored after measurement. Unrelated follow-ups are in [backlog](backlog.md).
