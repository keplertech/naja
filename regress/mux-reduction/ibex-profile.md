<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Ibex loader profile — 2026-11-15

**The next candidate is an empty-binding fast path in the SystemVerilog active
loop queries.** Two native samples place approximately 42% of load-call stacks
in expression-description formatting beneath those queries. Diagnostic counters
show that 97.22% of constant lookups and 98.53% of variable scans enter with both
binding tables empty. Further mux reduction is a smaller target in this profile.

This follows the [controlled Release study](release-performance.md), where Ibex
load time remained approximately 481 ms after #535/#536. The input is the same
public pinned default CHERIoT Ibex workload and file list, on `f6388fae`, with
the same compiler, dependencies and Release settings. No optimization is
implemented in this profiling change.

## Native samples of the uninstrumented build

Two independent processes repeatedly loaded the input and destroyed their
universes between loads. `/usr/bin/sample` collected each for ten seconds at
a requested one-millisecond interval. These are warm repeated-load profiles,
rather than the fresh-process timing experiment. Sampling itself perturbs the
program; these results locate work rather than establish a speedup.

The parser retains only main-thread stacks beneath the raw Python binding's
`loadSystemVerilog` call. It excludes universe destruction and parked worker
threads. Capture 1 contains 6,078 such observations out of 6,339 main-thread
observations; capture 2 contains 5,909 out of 6,186. Percentages below are
inclusive **sampled main-thread load stacks**, not aggregate CPU utilization.
Waiting inside the load remains included. Categories overlap and must not be
summed. Recursive occurrences of a function count once per sampled stack.

| Category | Capture 1 | Capture 2 |
|---|---:|---:|
| Procedural lowering (`createSequentialLogic`) | 71.85% | 69.93% |
| Combinational procedural lowering | 69.69% | 67.49% |
| Active-loop queries, union | 45.85% | 45.51% |
| Expression description beneath active-loop queries | 42.12% | 42.07% |
| Continuous assignments | 10.50% | 12.08% |
| Mux emission | 5.31% | 5.28% |
| Source annotation | 4.87% | 3.88% |
| Anonymous-assign cleanup | 2.32% | 2.44% |
| Slang `parseAllSources` | 2.09% | 2.59% |
| Repeated-mux cleanup | 1.58% | 1.73% |

`createSequentialLogic` also dispatches combinational procedural blocks; this
does not mean register construction consumes 70% of the load. Likewise,
`parseAllSources` is one Slang operation, not all Slang semantic work.

The description path includes stream formatting, strings, locale handling and
malloc/free. `describeExpression` can itself query constant indices while
describing selects, which re-enters constant evaluation. The profile identifies
a success-path query cost, not just formatting of emitted diagnostics.

## Empty-binding query counts

Temporary entry counters classified both queries using exactly:

```cpp
activeForLoopConstants_.empty() && activeForLoopNameConstants_.empty()
```

Three fresh-process loads gave identical counts. These counters were compiled
with the normal constructor performance-report flag **off**, so the optional
phase instrumentation does not contribute additional query calls. Counts
include recursive entries, rather than unique AST expressions.

| Function | Entries per load | Both tables empty | Empty fraction |
|---|---:|---:|---:|
| `getActiveForLoopConstant` | 926,088 | 900,327 | 97.22% |
| `isActiveForLoopVariableExpr` | 650,497 | 640,905 | 98.53% |

In [the constructor](../../src/nl/formats/systemverilog/frontend/SNLSVConstructor.cpp),
`getActiveForLoopConstant` formats `describeExpression` before checking its
binding lists. With empty lists, its source-identifier helper and remaining
lookups all return no value. `isActiveForLoopVariableExpr` still recursively
walks expressions in that state, although its binding providers cannot find
an active loop variable. Adjacent source-based loop helpers already use the
empty-binding check.

The first candidate is to return `std::nullopt` or `false`, respectively,
before that work when both lists are empty. Keep all existing matching and
fallback logic when either list is populated. Use the two binding lists as the
predicate: `hasActiveForLoopContext()` tracks the break stack, which is a
different state from captured or named bindings used during replay.

The sampled share is not a predicted speedup. A guarded implementation still
needs correctness checks and a new uninstrumented comparison.

## Existing constructor phase instrumentation

A separate archived Release build enabled `ENABLE_SV_CONSTRUCTOR_PERF_REPORT`.
Three fresh-process loads produced the following median [minimum–maximum]
milliseconds. These instrumented durations are not substituted for the
uninstrumented study's timings. In particular, recursive design and instance
buckets include child work; phase buckets overlap and cannot form an additive
time budget.

| Bucket | Median [min–max], ms |
|---|---:|
| Constructor total | 512.444 [503.526–558.932] |
| Parse phase | 20.482 [12.161–23.273] |
| Build top designs | 436.664 [436.289–475.715] |
| Procedural lowering | 349.412 [346.619–378.759] |
| Continuous assignments | 59.597 [58.861–65.077] |
| Source annotation | 6.463 [6.390–6.668] |

The reports agree on 29 unique built designs, 48 lowered combinational blocks,
64 lowered sequential blocks and 50,703 source-annotation calls. This supports
the native sample's procedural-lowering focus. The phase reports do not time
the active-loop queries separately.

## Allocation observations

A single load under malloc stack logging was held after loading for inspection.
The live heap inspection classified 443,926 allocations totaling 51,958,736
bytes. Its largest entries were:

| Tool classification | Live allocations | Allocator bytes |
|---|---:|---:|
| `SNLInstTerm` | 132,217 | 14,808,304 |
| `SNLInstance` | 28,853 | 7,386,368 |
| `SNLBusNetBit` | 39,808 | 6,369,280 |
| Allocations at `SNLRTLInfos::create` | 47,748 | 3,819,840 |
| Source-origin records | 45,006 | 3,600,480 |
| `SNLScalarNet` | 16,120 | 2,579,200 |
| `SNLBusTermBit` | 15,015 | 2,402,400 |
| Source-origin vector storage | 45,006 | 2,291,920 |
| Instance term storage | 28,853 | 1,985,280 |

Allocator bytes include allocation size classes; backtrace-derived labels are
tool classifications. Pools and arenas appear at backing-block granularity.
This is the retained heap after the constructor and its
temporary AST/caches have been released, not a cumulative allocation profile
or an RSS breakdown.

The initial logging mode supported the live heap but could not reconstruct a
high-water mark. A separate run using only `MallocStackLoggingNoCompact=1`
enabled full event history. It reported 6,190,701 allocation and 5,747,241
deallocation operations, approximately 328.1 MB cumulatively allocated and a
tool-reported 75 MB peak live heap plus anonymous VM (495,678 live allocations
at that peak). These process totals include startup, import and brief post-load
activity. Logging greatly perturbs runtime and memory; they are neither the
production peak RSS nor evidence that all transient allocations come from
expression formatting. The CPU samples and query counters independently
identify that formatting path as a large avoidable-work candidate.

The retained heap is dominated by pins, instances, bit objects and source
metadata. Its shape does not justify changing the primitive model or dropping
provenance. The first proposed change operates on negative loop lookups instead.

## Reproduction, restoration and next validation

Numeric measurements are retained in
[ibex-profile-samples.csv](ibex-profile-samples.csv). Raw native reports contain
addresses, local paths and long stack signatures and remain outside the
repository. The public input hashes still match the Release study. Diagnostic
builds retain the same final counts: 28,853 instances, 1,732 muxes, 14,491 mux
output bits, 18,045 nets and 55,927 bit nets.

The CPU profiles use the archived uninstrumented module. Phase timers and
temporary query counters use separate archives. The counter patch was restored
and the isolated build returned to the default performance-report-off setting.
No diagnostic counters or profiler output are added to production code.

With a process repeatedly loading the same public input, collect a native tree:

```sh
sample PID 10 1 -file sample.txt
python3 profile_sample.py sample.txt --output summary.json
```

The [parser](profile_sample.py) subtracts immediate-child counts to recover
exclusive weights, then attributes each weight to matching ancestors. A
recursive-tree check verifies it does not double-count recursive frames.
Phase reports use the existing CMake flag and `NAJA_SV_CONSTRUCTOR_REPORT`.
For a single held load, `heap --noContent --sortBySize PID` inspects live
allocations; full stack logging permits
`malloc_history PID -callTree -highWaterMark -noContent`.

For the proposed fast paths, cover empty bindings, symbol-only and name-only
bindings, nested/shadowed loops, captured bindings during replay, and diagnostics
on unsupported expressions. Run the frontend and cross-process determinism
contracts with both source build paths, then compare uninstrumented Release
loads against `f6388fae`. That implementation and comparison are the next phase.
