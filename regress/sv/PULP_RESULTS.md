<!--
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# PULP EDA benchmark frontend survey

## Ara automatic-function lowering follow-up (2026-10-29)

The pinned `ara.default` benchmark now **passes loading and structural Verilog
dumping**, with no frontend compatibility flags. The original 19 unsupported
lowering errors are cleared. Loading takes **219.37 seconds**, the efficiency
audit 18.33 seconds and dumping 5.56 seconds (248.03 seconds for the runner).
The top has 9 ports / 1,396 port bits; the structural dump is 100,163,430 bytes.
Artifacts are under `build/pulp-ara-final/`. The dump matches the preceding successful run
byte for byte after excluding its timestamp banner.

The fixes preserve signed division through the existing signed primitive,
replay terminal conditional/case returns with scoped automatic arrays, keep
same-named declarations in different lexical scopes separate, initialize
automatic arrays and implicit results to their language-defined defaults,
and preserve output-call copy-outs to locals read by later statements.
Subroutine input materialization uses formal widths for argument conversions.
Calls with unsupported external writes are validated even when their explicit
copy-out results are unused.
Constant case matches are exposed to replay pruning so fixed parameter branches
do not expand every table implementation.

Synthetic regressions cover different array shapes and directions, nested
lookup calls, repeated block entry, signed arithmetic, partial/dynamic result
writes including four-state X defaults, and multi-output argument conversions.
CMake validation passes 1,352 frontend tests, 85 dumper tests, 257 VHDL tests,
32 high-level SV Python tests, 42 raw database tests, 100 runner tests and the
cross-process determinism contract including fresh snapshot reloads. Sphinx
builds successfully. Bazel remains blocked by the local Apple SDK's undeclared
`SDKSettings.json` input; build-path parity is not established by these results.

Remaining findings: 1,751 Slang warnings and 115 existing two-state
case-comparison warnings remain. The efficiency mechanism detects **744 expanded
duplicate mux bits** (456 in one repeated lane model and 288 in the sequencer).
There are no findings in its identical-input, repeated-arm or unconsumed-mux
categories. Full-design structural lint and functional equivalence have not
been established by this load/dump run.

## Cheshire inlined-local diagnostic follow-up (2026-10-29)

The reported `repData64` loop-counter warning is a false positive, not a
missing functional driver. After lowering cleanup, its 32-bit placeholder has
zero instance-terminal connections and zero port connections. The warning
previously ran before unused replay muxes were removed. Checking surviving
connectivity after alias and mux cleanup reduces **32
`undriven_inlined_local` warnings to zero** on the pinned Cheshire benchmark.
Loading still completes without unsupported-element errors; other diagnostics,
including the existing two-state case-comparison warnings, remain.

Deleted anonymous aliases are also removed from the inlined-net tracker before
their addresses can be reused. Connected undriven locals retain declaration
locations and console deduplication; a focused partial-read regression warns
only for the two bits actually read. An independent synthetic loop/case helper
checks all 16,384 binary inputs against expected values. Full validation passes
1,348 frontend tests, focused Python loader tests, cross-process determinism
and Sphinx. Source Bazel validation remains blocked by the local Apple SDK
configuration. Before/after connectivity audits are under
`build/cheshire-inlined-local-before/` and
`build/cheshire-inlined-local-after/`.

## Repeated mux selection cleanup (2026-10-29)

The second remaining efficiency issue is resolved. Both RTL loaders use the
shared SNL pass to combine complete, exclusive mux chains choosing the same
true value via OR, and the dual common-false-value pattern via AND. It rejects
shared outputs, additional pin uses, partial/reordered vectors, disconnected
inputs, extra drivers and direct feedback. The surviving mux keeps its output
identity and receives both source origins; new selector gates/nets retain those
origins too. No primitive capability or language scheduling semantics change.

On the pinned RedMulE benchmark, expanded mux bits decrease from **2,978,881
to 2,946,665** (32,216 fewer), and expanded mux instances from **368,302 to
346,687**. The common-true rewrite removes the previously identified 21,585 mux
bits; supporting the opposite polarity removes another 10,631. The audit now
reports zero findings in all four implemented categories. This is not a proof
that no other lowering inefficiencies exist. Load takes 44.58 seconds, auditing
8.00 seconds and dumping 1.03 seconds in this run. Artifacts are under
`build/pulp-redmule-repeated-mux-final/`.

Synthetic verification exercises both polarities, longer chains, shared values,
reversed creation order, malformed/non-exclusive connections, source metadata,
and both HDL frontends. Source-versus-dump simulations exhaust X/Z data and
selectors. The cross-process contract covers live state, dump artifacts and
fresh-process NajaIF reloads with the new selector logic. Source Bazel tests
remain blocked by the local Apple SDK's undeclared `SDKSettings.json` input.
Full-design RedMulE lint and the other outstanding frontend limitations remain
separate tasks.

## Unconsumed mux cleanup (2026-10-29)

The first remaining efficiency issue is resolved: the shared SNL cleanup removes
canonical mux cones with no path through muxes to a design terminal or a
non-mux consumer. SystemVerilog applies it after alias collapse; VHDL applies
it when finishing each newly lowered model. Ports, register inputs, black-box
connections, and other non-mux consumers keep their mux cones. Nets and all
non-mux instances remain; unobserved named temporaries can therefore remain
without drivers. Producer lookup avoids quadratic scans of high-fanout inputs.

On the pinned RedMulE benchmark, the audit's **19,502 expanded unconsumed mux
bits fall to zero**. Expanded mux bits decrease from **2,998,383 to 2,978,881**;
expanded mux instances decrease from **372,945 to 368,302**. Repeated-selection
candidates remain a separate optimization task. Artifacts are under
`build/pulp-redmule-unused-mux-final/`.

Validation passes 227 core tests, 255 VHDL frontend tests, 1,343 SystemVerilog
frontend tests, 85 dumper tests, and focused Python loader tests. Synthetic
source-versus-dump simulation covers binary inputs and X/Z data/selects.
Cross-process determinism covers mux cleanup, hierarchy sharing and NajaIF
round trips. Bazel validation remains blocked by the local Apple SDK's
undeclared absolute `SDKSettings.json` input. This step does not establish
completion of full-design RedMulE lint or address the other remaining frontend
limitations.

## RedMulE lowering efficiency follow-up (2026-10-29)

At the pinned public PULP revision
`b4bd97823aa93737dda0bf1f3b2bd5c6dcb47ecc`, the additional lowering fixes
reduce expanded mux bits from **3,382,510 to 2,998,383** (11.36%) and mux
instances from **386,844 to 372,945**. Load takes 40.61 seconds, the independent
efficiency audit 6.67 seconds, and dumping 0.81 seconds on this machine.

Simple concatenated assignments now replay only actual value and selector
dependencies; compound concatenations retain whole-value dependencies. Generic
mux construction wires identical operand bits directly and shares exact pure
expressions. Adjacent equal case-arm results share one selection, preserving
priority and independently merging replay locals. This applies to procedural
replay and the older sequential path. Synthetic examples reduce concatenation
selection from 24 to 12 mux bits, three equal case arms from 18 to 6, and a
padded constant selection from 8 to 2. Known-selector comparisons and four-state
data simulations pass; existing two-state case-comparison limitations remain.

The frontend-independent Python audit reports source-grouped optimization
opportunities, with configurable warnings, and runs automatically in the PULP
runner. It still identifies approximately 19,500 expanded unconsumed mux bits
outside the corrected patterns, predominantly in converter logic. This audit
is deliberately conservative, and its overlapping categories must not be added.
Artifacts are under `build/pulp-redmule-compact-replay-final/`.

Validation passes 1,341 C++ frontend tests, 85 dumper tests, 32 high-level Python
SystemVerilog tests, 42 raw loader tests, 100 runner tests, cross-process
determinism, Sphinx, and standalone source-versus-dump simulations. Full RedMulE
Verilator lint still exceeds a 120-second check. Source Bazel validation is
blocked locally by an Apple SDK/toolchain undeclared-absolute-input error in
`SDKSettings.json`; CMake success does not establish Bazel validation.

## NVDLA regular regression (2026-10-20)

`nvdla.top` joins the regular load/dump/lint regression tier. The runner sets
`frontend_options={"slang": ["--timescale=1ns/1ps"]}` for this variant, preserving explicit source time
scales and leaving the pinned release sources unchanged. The sequential
lowering fix resolves selected-target arithmetic through the ordinary RHS
lowering path, avoiding a whole-register increment shortcut that required
precomputed incrementer bits.

The maintained runner passes locally: load/dump takes 81.11 seconds and strict
Verilator 5.052 lint takes 44.84 seconds. The same complete dump also passes
strict lint with upstream Verilator v5.046, matching the standard CI image's
version, in 19.34 seconds. Docker was unavailable locally, so the 5.046 check
uses a native build of upstream tag v5.046. All 14 PULP runner/report tests pass,
and the regular/survey matrix has no duplicate NVDLA case. The GitHub workflow
has not yet been run with this promotion.
The frontend suite passes all 1,311 tests, including exhaustive selected-target
next-state checks; cross-process determinism and snapshot round trips pass.
Earlier survey tables below retain their historical failure results.

Regular CI now has five smoke checks, four elaboration checks, C910 lint, and
ITA functional simulation: eleven verification jobs. Nightly surveys add
seventeen variants, keeping twenty-eight verification jobs overall and running
NVDLA only once.

## C910 standard verification pass (2026-10-04)

C910 now passes the standard prepared-PULP runner, with **load/dump in
19.72 seconds and strict Verilator 5.052 lint in 149.60 seconds**. The native
Verilog dumper exposes opt-in `verilatorSplitPackedSignals` through its C++
configuration, raw `SNLDesign.dumpGateLevelVerilog` keyword, and high-level
`GateLevelVerilogDumpConfig`. The runner enables it for prepared-PULP dumps. Internal
bus nets and packed child-module ports receive `split_var` comments; public
top ports are excluded. No logic or warning-suppression flags change.
Artifacts are under `build/pulp-c910-standard-split/`.

The focused regression passes strict lint and four-state simulation for an
acyclic packed-port design, and verifies that a genuine bit-level feedback
loop still triggers `UNOPTFLAT`. Scope tests cover ascending/descending bus
ports, scalar exclusion, public top exclusion, multiple output files,
library streams, and reuse of the dumper. Validation passes 79 dumper tests,
1,166 frontend tests, 17 raw design tests, 21 high-level SystemVerilog tests,
12 PULP runner/report tests, all four existing PULP smoke designs, Sphinx,
and workflow actionlint. Smoke artifacts are under
`build/pulp-standard-split-smoke/`.

C910 has a separate regular `pulp-c910` workflow job on PRs, pushes, schedules,
and manual dispatch. It shares ITA's pinned Verilator 5.052 source build and
uses a 300-second limit per stage. This is load/dump/lint coverage; full C910
functional simulation is not yet part of the workflow. The broad nightly
survey retains its independent 120-second stage limit. The new GitHub job
has been configured locally but has not yet been run on GitHub.

## C910 sequential lowering follow-up (2026-10-04)

The working-tree fix on top of `3989a4b1` addresses actual lowering errors
behind the mixed-assignment diagnostics. Whole-vector replay was emitting
flops for bits that the process never wrote, and nested reset writes could
emit duplicate drivers. The MMU's `lsu_data_flop[58:0]` consequently also drove
bits `[63:59]`, which belong to a separate RTL process. AXI `id_queue` element
and field resets similarly overlapped. The frontend now masks emitted storage
to the process's assigned bits and merges nested targets before replay.
The mask also applies to direct and blocking-scheduling replay, with loop
selections resolved in their iteration context.

Simulation exposed a second issue in conditional multi-assignment lowering:
mixed zero/one asynchronous resets were implemented as data muxes. These now
use separate reset/set cells, preserving asynchronous behavior, reset polarity,
and clock edge. The focused regression checks one driver per output bit and
compares dumped Verilog with the original RTL for both clock edges and reset
polarities, including nested partial writes, procedural loops, blocking replay,
reset assertions between clock edges, and X-valued data.

The final local run loads/dumps C910 in 16.51 seconds and completes Verilator
5.052 lint in 142.62 seconds. The previous **five `BLKANDNBLK` errors and six
`MULTIDRIVEN` warnings are gone**. Four `UNOPTFLAT` warnings remain around AXI response/ready paths through the
zero-memory adapter, demux, and burst unwrap. No lint suppression was added;
C910 remains outside the passing strict-lint tier.

A separate diagnostic lint of the unmodified source RTL also reports
`UNOPTFLAT` through the zero-memory response, burst-unwrap `b_cnt_err`, and
AXI arbitration path. It reports one cycle warning rather than the generated
netlist's four; this establishes a source-side warning in the same region,
not bit-level equivalence of all reported cycles. That diagnostic run uses
`-Wno-fatal` to collect upstream warnings and is **not** a strict-lint pass.
Its log is under `build/pulp-c910-original-lint/`.

Further diagnostic runs distinguish packed dependency tracking from a change
in logic. Adding `/* verilator split_var */` to internal packed wires alone
leaves all four `UNOPTFLAT` warnings. Adding the annotation to packed child
module ports as well eliminates them. Public top-level ports are excluded:
Verilator cannot split them and otherwise emits five `SPLITVAR` warnings.
The annotated copy completes strict lint with return code 0 in 116.86 seconds,
using the original runner flags and no additional warning suppression. This
provides evidence that packed-port dependency tracking contributes to these
reports; it is not a functional equivalence proof. The annotations are confined
to diagnostic artifacts, so the ordinary dump still reports four warnings.
Commands and the successful result are recorded under
`build/pulp-c910-split-ports-diagnostic/strict-summary.json`.

Validation passes all 1,165 frontend tests, all four existing PULP smoke designs,
and the Sphinx HTML build. Final C910 artifacts are under
`build/pulp-c910-sequential-verified/`; smoke artifacts are under
`build/pulp-c910-fix-smoke/`.

## C910 division/remainder follow-up (2026-10-04)

The working-tree fix on top of `5fb0566a` corrects two independent issues:
the PULP runner now lints with the dumper-generated `naja_primitives.v`, and
the dumper preserves division/remainder model defaults even when no instance
parameter overrides exist. Both C910 instances retain `.WIDTH(40)` and
unsigned operation. The shared parameter emitter omits `.SIGNED(0)` because
it matches the generated module's default. The generated model also uses separate signed
and unsigned branches: the previous conditional expression incorrectly
performed unsigned arithmetic for cases such as signed 4-bit `1 / -1`.

The rebuilt Release frontend loads and dumps C910 in 25.06 seconds. Verilator
5.052 now completes lint in 128.42 seconds with **no missing-module or width
errors**, but reports five `BLKANDNBLK` errors, six `MULTIDRIVEN` warnings, and
four `UNOPTFLAT` warnings. These involve the MMU data flop, AXI linked-data
queues, separately clocked datapath bits, and combinational paths; their causes
are not established by this run. No suppression of those diagnostics was added, and C910
remains outside the passing lint tier. The follow-up used a 240-second limit
and records its commands, generated models, netlist, and diagnostics under
`build/pulp-c910-fixed/`.

Focused simulation compares source RTL with the dumped design at widths 1,
4, and 40, signed and unsigned, for both combinational and clocked quotient
and remainder outputs. It covers 1,572 input vectors, including division by
zero, negative operands, signed overflow, and X/Z inputs. The original model
fails the signed comparison; the corrected model passes. Lint timeouts now
terminate the full process group, including Verilator launcher children.

The four existing smoke designs (CV32E40P, FP32 FMA, minimal Ibex, SPI host)
all pass load/dump/lint with the generated models. The generated latch model
carries the same narrowly scoped intentional-latch annotation as the previous
handwritten model. Validation also passes 76 dumper tests, 1,164 frontend
tests, 12 PULP runner/report tests, five ITA adapter tests, and the Sphinx HTML
build. The smoke artifacts are under `build/pulp-generated-model-smoke/`.

## Current full survey (2026-10-04)

Reconfigured and rebuilt the existing macOS arm64 **Release** build from
Naja `5fb0566a` (0.7.27), using Python 3.14.7, Verilator 5.052, and the same
SHA-256-verified v0.1.0 archive as the original survey. The working-tree changes
at measurement time only affect CI/reporting/documentation, not the frontend.
All 23 variants were attempted without RTL edits, compatibility relaxations,
or unknown-module blackboxing. Each load/dump and each lint has a separate
120-second wall-clock limit. These are local results, not Ubuntu CI timings.

**Frontend outcome: 12/23 elaborate successfully; 11/23 also finish dumping.**
The remaining 11 fail with diagnostics: four Slang compilation failures
(Ara, Cheshire, NVDLA, RedMule) and seven Naja lowering failures (O-POPE,
Serial Link, four Snitch variants, Spatz). Terapool elaborates successfully
but does not finish dumping within the combined load/dump limit.

The initial end-to-end sweep records four strict lint passes, one lint
failure (C910), six lint timeouts, eleven load failures, and one load/dump
timeout. The local Verilator Perl launcher left child processes running after
some timeouts. Those children were stopped; all six timed-out lint cases were
subsequently rechecked with process-group cleanup. The original JSON records
are retained rather than overwritten. Timings from the initial sweep are not
isolated performance measurements.

After clean lint follow-ups, **four variants pass the complete load/dump/lint
path**, two have confirmed lint failures (full Ibex and C910), and five still
exceed the 120-second lint limit (CVA6, full CVFPU, ITA, MinPool, MemPool).
Together with eleven load failures and Terapool's dump timeout, these account
for all 23 variants. The five clean lint timeouts used new process groups and
killed the entire group on timeout, preventing the earlier orphan overlap.

| Variant | Load/dump result | Load/dump wall time (s) | Observation |
|---|---|---:|---|
| `ara.default` | failed | 0.84 | Slang: implicit enum conversions in `lane_sequencer.sv`. |
| `cheshire.default` | failed | 0.78 | Slang: enum conversion and `dtmcs_q` used before declaration. |
| `cv32e40p.default` | pass | 1.31 | Strict lint passes. |
| `cva6.default` | pass | 37.95 | Loads and dumps; flat lint exceeds 120 seconds in both the initial survey and clean follow-up. |
| `cvfpu.fp32_fma` | pass | 0.41 | Strict lint passes. |
| `cvfpu.full` | pass | 12.67 | Loads and dumps; flat lint exceeds 120 seconds in both the initial survey and clean follow-up. |
| `ibex.minimal` | pass | 0.84 | Strict lint passes. |
| `ibex.full` | pass | 2.78 | Loads and dumps; follow-up lint fails two shadow-ALU `UNOPTFLAT` warnings. |
| `ita.default` | pass | 50.25 | Loads and dumps; flat lint exceeds 120 seconds in both the initial survey and clean follow-up. |
| `mempool.minpool` | pass | 65.13 | Loads and dumps; flat lint exceeds 120 seconds in both the initial survey and clean follow-up. |
| `mempool.mempool` | pass | 119.27 | Loads and dumps; flat lint exceeds 120 seconds in both the initial survey and clean follow-up. |
| `mempool.terapool` | timeout | 120.64 | Elaboration passes; dump exceeds the 120-second combined budget. |
| `nvdla.top` | failed | 0.73 | Slang: inconsistent/missing timescales. |
| `opope.default` | failed | 6.88 | Naja: continuous-assignment LHS in `hci_core_fifo.sv:278`. |
| `pulp_c910.default` | pass | 32.62 | Loads and dumps; lint cannot find simulation primitive `naja_divmod`. |
| `redmule.default` | failed | 0.46 | Slang: implicit conversions to `fp_format_e`. |
| `serial_link.default` | failed | 0.78 | Naja: register/interface aggregate types (`reg2hw`, `hw2reg`). |
| `snitch_cluster.minimal` | failed | 46.46 | Naja: conditional RHS in `axi_burst_splitter_gran`; peripheral register aggregates/sequential lowering. |
| `snitch_cluster.default` | failed | 59.46 | Naja: conditional RHS in `axi_burst_splitter_gran`; peripheral register aggregates/sequential lowering. |
| `snitch_cluster.default_latch` | failed | 54.58 | Naja: conditional RHS in `axi_burst_splitter_gran`; peripheral register aggregates/sequential lowering. |
| `snitch_cluster.occamy` | failed | 57.44 | Naja: conditional RHS in `axi_burst_splitter_gran`; peripheral register aggregates/sequential lowering. |
| `spatz.default` | failed | 86.91 | Naja: function-call RHS in `spatz_vfu.sv:1522`. |
| `spi_host.default` | pass | 0.30 | Strict lint passes. |

Terapool's separate 240-second elaboration-only follow-up passes in 64.39 s
(including startup/cleanup; load itself is 53.93 s). Full Ibex's separate lint
follow-up fails in 33.71 s with the two known `UNOPTFLAT` warnings; its limit
was 240 seconds. Thus its initial `lint_timeout` does not replace the confirmed
lint diagnostic.

Notable changes from the October 1 baseline:

- C910's parser errors no longer reproduce. It loads and dumps, but the lint
  harness lacks the `naja_divmod` simulation module referenced by the dump.
- All three MemPool variants elaborate; MinPool and MemPool also dump.
- CVA6, full CVFPU, and ITA load and dump.
- All four Snitch variants now finish and report explicit lowering failures.
  They are no longer merely inconclusive timeouts.
- Spatz's compound-shift issue is gone; the function-call RHS remains unsupported.

Raw results, build provenance, logs, generated netlists, and the rendered
report are under `build/pulp-current-survey/`. `verified-summary.json` and
`verified-report.md` combine the initial sweep with the completed lint
follow-ups. Lint follow-ups have separate
`lint-followup.json` and `lint-followup.log` files in each case directory.
Terapool's elaboration follow-up is under `build/pulp-current-elaboration/`.
No functional simulation was rerun as part of this survey; earlier ITA
functional results below remain separate evidence.

Reproduction (using a package linked to the freshly built Release extension):

```sh
python3 regress/sv/pulp_benchmarks.py --all \
  --archive /tmp/pulp-benchmarks-v0.1.0.tar.gz \
  --najaeda-path build/pulp-current-package \
  --output build/pulp-current-survey --timeout 120 --lint-runner local
python3 regress/sv/pulp_benchmarks.py --case mempool.terapool --elaboration-only \
  --archive /tmp/pulp-benchmarks-v0.1.0.tar.gz \
  --najaeda-path build/pulp-current-package \
  --output build/pulp-current-elaboration --timeout 240
```

## Original baseline (2026-10-01)

Measured on 2026-10-01 with Naja `927b9ca2` (0.7.27), rebuilt from source
using the existing macOS arm64 **Debug** CMake build and Python 3.14. The input is the
[prepared v0.1.0 release](https://github.com/pulp-platform/eda-benchmarks/releases/tag/v0.1.0),
SHA-256 `a27eaca9359cd2c0b646892325355e102ae563bd8f772c77f18697ff8c4ad603`.
This checksum also matches GitHub's published asset digest.

Every variant uses its supplied top, filelist, macros, wrappers, and source
stubs. No RTL fixes, best-effort hierarchy, or unknown-module blackboxing were
used. The initial survey applied a 90-second wall-clock limit per load/dump.
Timings are local observations, not Linux Release CI performance guarantees; some
follow-up tests ran concurrently with the survey.

## Recommended PR tier

| Target | Input sources | Load/dump | Complete-dump lint |
| --- | ---: | ---: | ---: |
| `cv32e40p.default` | 28 | ~7–9 s | ~4 s, pass |
| `cvfpu.fp32_fma` | 8 | ~1.5 s | ~1 s, pass |
| `ibex.minimal` | 45 | ~4–5 s | ~2 s, pass |
| `spi_host.default` | 25 | ~1 s | <1 s, pass |

These are the runner's default cases and the new `PULP SV Regress` workflow's
PR/push selection. FP32 FMA and SPI host add coverage beyond the existing
Ibex/CV32E40P external regressions. Those CPU cases also exercise this release's
specific source pins, configuration, and wrappers.

Local lint used Verilator 5.052 and the complete Naja dump plus
`najaeda_primitives.v`, with only `ASCRANGE` suppressed, matching the existing
external-regression convention. CI uses the existing pinned Verilator 5.046
Docker image. The Docker daemon was unavailable locally, so that image and the
Ubuntu GitHub job have not been executed here. No simulation or equivalence
check was performed; successful load/dump/lint does not prove functional
correctness.

`ibex.full` also loads and dumps in ~20–22 seconds but fails strict lint with
two `UNOPTFLAT` warnings involving the secure/lockstep shadow core's ALU. It is
available as an explicit case, but not promoted to the green tier. These
warnings need investigation before either suppression or promotion.

## Complete initial survey

All 23 variants were attempted: **5 load/dump passes, 7 failures, 11 timeouts**.

| Target | Load/dump outcome | Seconds | Observation |
| --- | --- | ---: | --- |
| `ara.default` | fail | 1.94 | Slang rejects implicit enum conversions in `lane_sequencer.sv`. |
| `cheshire.default` | fail | 2.68 | Slang rejects an enum conversion and use of `dtmcs_q` before declaration. |
| `cv32e40p.default` | pass | 6.52 | Also passes complete-dump lint; PR tier. |
| `cva6.default` | timeout | 90.08 | No unsupported error emitted before timeout; inconclusive. |
| `cvfpu.fp32_fma` | pass | 1.39 | Also passes complete-dump lint; PR tier. |
| `cvfpu.full` | timeout | 90.03 | No unsupported error emitted before timeout; inconclusive. |
| `ibex.minimal` | pass | 4.27 | Also passes complete-dump lint; PR tier. |
| `ibex.full` | pass | 20.15 | Load/dump passes; lint fails two `UNOPTFLAT` warnings. |
| `ita.default` | timeout | 90.02 | Before timeout: unsupported streaming assignment LHS, `ita_inp1_mux.sv:16`. |
| `mempool.minpool` | timeout | 90.12 | Before timeout: unsupported `always_latch` pattern, `register_file_1r_1w.sv:146`. |
| `mempool.mempool` | timeout | 90.09 | Before timeout: same unsupported latch pattern as minpool. |
| `mempool.terapool` | timeout | 90.10 | No unsupported error emitted before timeout; inconclusive. |
| `nvdla.top` | fail | 0.77 | Slang rejects inconsistent/missing timescales across modules. |
| `opope.default` | fail | 17.02 | Unsupported continuous-assignment LHS in `hci_core_fifo.sv:278`. |
| `pulp_c910.default` | fail | 4.04 | Slang reports expected-expression errors in `ct_lsu_sq.v:3860–3866`. |
| `redmule.default` | fail | 2.85 | Slang rejects implicit conversions in `redmule_streamer.sv:245,395`. |
| `serial_link.default` | fail | 2.02 | Unsupported register/interface aggregate types in `slink`/`slink_reg`, with cascading connection errors. |
| `snitch_cluster.minimal` | timeout | 90.04 | No unsupported error emitted before timeout; inconclusive. |
| `snitch_cluster.default` | timeout | 90.04 | No unsupported error emitted before timeout; inconclusive. |
| `snitch_cluster.default_latch` | timeout | 90.04 | Before timeout: unsupported latch pattern, `snitch_regfile_latch.sv:76`. |
| `snitch_cluster.occamy` | timeout | 90.05 | No unsupported error emitted before timeout; inconclusive. |
| `spatz.default` | timeout | 90.06 | Before timeout: unsupported compound shift assignment, `spatz_controller.sv:123`. |
| `spi_host.default` | pass | 0.97 | Also passes complete-dump lint; PR tier. |

The initial local logs and summary are under `build/pulp-survey/`; the final
strict smoke/lint artifacts are under `build/pulp-benchmarks/`. Full-Ibex lint
artifacts are under `build/pulp-additional-passing/ibex.full/`. These generated
files are intentionally untracked.

Useful follow-up work is streaming assignment LHS lowering (ITA), latch
statement patterns (MemPool/Snitch), compound shifts (Spatz), and aggregate/LHS
lowering (Serial Link/O-POPE). Re-run the inconclusive cases in a Release build
with a larger budget before deciding whether they belong in a scheduled tier.

## Follow-up: compound shifts and streaming LHS

The working-tree frontend changes add combinational compound shifts and
fixed-size streaming assignment LHS lowering. They preserve signed right
shifts, full-width shift counts, blocking-assignment replay, nested streams,
non-divisible slice widths, fixed arrays, multiple targets, and MSB-aligned
consumption of wider streaming sources. Dynamic/with-clause/overlapping targets
and sequential streaming assignments remain explicitly rejected.

The generated `compound_shifts` benchmark passes 262,144 vectors, including a
70-bit shift count. Original RTL passes the same vectors. The generated
`streaming_lhs` benchmark passes 1,048,576 vectors. Verilator 5.052 disagrees on
partial streaming-target slices and swapping target variables when simulating
the original streaming RTL. A separate C++ constructor test forces Slang
constant evaluation through localparams and confirms the independent expected
partial-slice, nested-stream, wider-source, and swap values. The streaming CI
simulation therefore checks the Naja dump against these golden values, not
Verilator's streaming RTL behavior.

Re-running the unmodified release with a 180-second limit:

- **ITA:** the streaming-LHS diagnostic is gone. Loading finishes with only
  unsupported latch patterns in `ita_register_file_1w_multi_port_read.sv:161`
  (two elaborations) and `ita_register_file_1w_multi_port_read_we.sv:171`.
- **Spatz:** the compound-shift diagnostic is gone; the full cluster still
  times out after 180 seconds in the local Debug build. This is not a full
  Spatz pass.

## Follow-up: independent always_latch writes

The latch lowering now handles independent guarded writes in statement lists
and statically bounded nested loops. It preserves per-bit retention while an
enable is low, source-order priority for repeated writes, static array and
part-select targets, and both blocking and nonblocking scheduling when used
consistently. Dynamic targets, same-block read-after-write dependencies,
function calls, timed assignments, and mixed scheduling are rejected.

The focused `latch_loops` fixture passes 16,385 state transitions in both the
original RTL and the generated Naja netlist. The complete frontend suite passes
1,140 tests. The ITA register-file modules, MemPool register file, and Snitch
register file now each load, dump, and pass Verilator lint, so these cases do
not require a new NLDB0 primitive.

The MemPool full design now elaborates successfully with zero Naja unsupported
diagnostics. The `axi_dw_downsizer.sv` `resp_precedence` function-call RHS
cases and the `ctrl_registers.sv` loop-step lowering are handled by the
frontend. Structural dumping of the full MemPool netlist exceeded 300 seconds
locally, so CI runs this design as an elaboration-only frontend regression;
full dumping remains a separate performance investigation.

The exhaustive case path accepts a no-`default` function case only when all
values of a small fixed-width domain are covered by constant labels. Other
incomplete cases remain unsupported. This keeps the lowering conservative while
supporting the AXI response precedence function.

Logs are under `build/pulp-lowering-fixes/`. The initial survey table above is
retained as the baseline; neither design has been promoted to the passing CI
tier.

## Follow-up: expanded elaboration tier (2026-10-03)

Rebuilt Naja `20d67a27` in the existing macOS arm64 Release build, using
Python 3.14 and the same checksum-verified v0.1.0 archive. The workflow now
adds `ita.default`, `cvfpu.full`, and `cva6.default` alongside `mempool.minpool` in the strict
elaboration-only tier, with a 240-second limit per case:

| Target | Elaboration outcome | Wall-clock seconds |
| --- | --- | ---: |
| `mempool.minpool` | pass | 24.66 |
| `ita.default` | pass | 52.00 |
| `cvfpu.full` | pass | 11.03 |
| `cva6.default` | pass | 27.98 |

Each passing case has a successful diagnostics summary, no error/fatal or
Naja unsupported diagnostics, the expected top, and a nonempty top interface.
These measurements include process startup and cleanup; other local work was
running, so they are not isolated performance measurements. This tier does
not establish structural dump, lint, simulation, or equivalence success.

`snitch_cluster.minimal` completes in Release but fails strict elaboration
after about 50 seconds on register aggregate types and sequential lowering
in `snitch_cluster_peripheral`. `spatz.default` fails after about 75 seconds
on an unresolved function-call RHS in `spatz_vfu.sv:1522` (`always_comb`).
O-POPE and Serial Link also remain failures
when rechecked with the refreshed Debug build. They are not promoted.

Release reports are under `build/pulp-expansion-release/`; the additional
failure reports are under `build/pulp-expansion-extra/`. CI writes the
elaboration tier into `build/pulp-benchmarks/elaboration/`, so its aggregate
summary no longer overwrites the complete-dump tier's summary.

## ITA upstream functional validation (2026-10-03)

The experimental `ita_simulation.py` runner reuses upstream ITA revision
`ba96519becce195d64e85eb9a5302e8a1d5487e7`, matching the benchmark release,
and its golden-vector generator and seven-phase testbench. Local validation
used Verilator 5.052, Naja `20d67a27` Release, Python 3.14, NumPy 2.5.3, and
ONNX 1.23.1. Vectors use seed 0, dimensions S/E/P/F=64, one head, bias,
identity activation, and randomized stalls (Verilator seed 1).

The original RTL and Naja netlist each completed all **2,048 output
transactions**, with **identical phase, output data, and timestamps**. This
is finite simulation evidence of preserved behavior, not formal equivalence.
Both also reported the **same six mismatches against upstream golden values**.
The first occurs in phase 3 at 4622.6 ns: actual
`80131f1db08af3ec80807f753c7f80dd`, expected
`80131f1db08af3ec80807f753c7f80ef`. The cause of this shared baseline mismatch
was not established in that initial run (see the follow-up below); the runner returns failure even when RTL and
netlist traces agree. It is not promoted to a green CI gate.

The synthesis SRAM stub is replaced by the upstream behavioral model on the
RTL side; the same model is attached to the generated netlist's explicit
SRAM blackbox. Its shape is checked (256 words, 416 bits, two ports, one-cycle
latency). The initial zero-delay run otherwise left the generated logic unchanged. The testbench adapter
works around Verilator's packed-array `$fscanf` C++ generation issue using
scalar temporaries and checks successful reads. Golden comparisons remain
intact, with complete transaction counts required.

Flat optimized Verilation hit an internal compiler fault; flat `-O0`
exceeded a 300-second limit. Hierarchical Verilation of the four large
datapaths succeeded, and the generated simulation completed in about
2.6 seconds. This is now the runner's default compile strategy.

Reports and logs are under `build/ita-simulation/`, including
`functional-comparison.json`, `original-complete-run.log`, and
`generated-complete-run.log`. See the README for the repeatable command.

## Reproduction and interpretation

See [README.md](README.md#pulp-prepared-eda-benchmarks) for the commands and
workflow dispatch options. A complete survey intentionally returns nonzero
when any variant fails or times out. There are no expected-failure masks.

A timeout alone is inconclusive. Some designs emit explicit unsupported
construct diagnostics before their timeout; those observations are useful
frontend work items even though the entire load did not finish. Strict Slang
compilation errors are separate from Naja lowering failures: upstream's Yosys
flow enables permissive compatibility options that this runner does not apply.

The local code knowledge graph was refreshed incrementally for code. Its full
semantic refresh needs an unavailable LLM API key; the code refresh also
reported existing duplicate-node collisions in `SNLDesignModeling`.


## ITA zero-delay mismatch diagnosis (2026-10-03)

The six golden mismatches originate at the latch-based weight buffer read
boundary in Verilator 5.052. Instrumenting the original RTL found eight
64-output blocks whose final captured weight vector differed from the other
63 vectors: transactions 831, 959, 1087, 1535, 1599, 1727, 1855 and 1919.
Only the first 128-bit write chunk was overwritten; six corruptions survived
requantization/saturation as visible output mismatches in processing lane 0.
The expected values in the scoreboard match the on-disk golden vectors.

The reduced `ita_latch_repro.sv` isolates the scheduling difference without
Naja or the ITA testbench. At the same edge as a bank overwrite, Icarus reads
the bank's old `0x11`, while Verilator reads the shared write register's
transient `0x22`; the final bank contents are `0x33` in both. The full upstream
buffer also reproduces this in Verilator with `-O0`. Verilator documents that
its conversion of nonblocking assignments in combinational logic can cause
simulation races: <https://verilator.org/guide/latest/warnings.html#combdly>.

The runner now models 1 ps read propagation on that buffer in both RTL and
generated simulation. This preserves the complete buffer logic and golden
vectors, while separating the memory output update from the reader's clock
edge. The JSON and Markdown reports disclose the delay; setting
`--weight-read-delay-ps 0` retains the failing control. The original structural
dump is not edited. A delay on the cascaded clock gates was tested and rejected
because it changes enable sampling; the implemented model delays only read data.


Validation with the 1 ps model completed successfully on the same local
Verilator 5.052 / Naja `20d67a27` Release environment:

| Stage | Result | Seconds |
| --- | --- | ---: |
| Vector generation | pass | 0.958 |
| Naja load + dump | pass | 45.524 |
| Original RTL build | pass | 8.398 |
| Original golden scoreboard | pass | 0.188 |
| Generated netlist build | pass | 317.899 |
| Generated golden scoreboard | pass | 2.697 |
| RTL/netlist trace comparison | pass, 2,048 transactions | — |

Reusing these binaries, all six combinations of vector seeds 0/1/2 and
Verilator stall seeds 1/7 also passed both golden scoreboards and exact
phase/data/timestamp comparison: **12,288 compared transactions**. Seed-0
vectors were restored afterward. Reports are in
`build/ita-simulation-timed/summary.json` and `seed-matrix.json`; the combined
PULP/ITA Markdown preview is `build/pulp-report-check/report.md`.
This establishes finite functional coverage under the disclosed read-delay
and external SRAM models. The full ITA simulation remains an explicit local
command; CI now includes its adapter unit tests but does not run the full
functional simulation by default.


## Naja primitive follow-up (2026-10-03)

The reproducer is now split into synthesizable `ita_latch_dut.sv` and a
simulation-only `ita_latch_repro.sv` driver. `ita_primitive_check.py` elaborates
only the DUT, with no unsupported diagnostics, and dumps 27 immediate
instances including five `naja_dlatch` instances and two `naja_dff` instances.
The read propagation delay is on the testbench feedback connection, outside
elaboration. No timing construct is silently discarded during lowering.

| Simulator | Read delay | Source RTL | Naja primitives | Nonblocking latch candidate |
| --- | ---: | --- | --- | --- |
| Icarus | 0 ps | pass (`0x11`) | pass (`0x11`) | pass (`0x11`) |
| Icarus | 1 ps | pass (`0x11`) | pass (`0x11`) | pass (`0x11`) |
| Verilator 5.052 | 0 ps | fail (`0x22`) | fail (`0x22`) | fail (`0x22`) |
| Verilator 5.052 | 1 ps | pass (`0x11`) | pass (`0x11`) | pass (`0x11`) |

The observed value is the reader flop's captured data; the memory's settled
contents are `0x33` in every case. The temporary candidate changes only
`if (E) Q = D` to `if (E) Q <= D` in an artifact copy of `naja_dlatch`.
Verilator emits `COMBDLY` for this assignment and the zero-delay failure
persists. No elaboration mismatch was observed, and switching the primitive
to nonblocking assignment does not resolve this case. The generic primitive
and Naja frontend remain unchanged; the disclosed ITA read-delay model stays
in the simulation harness.

The maintained runner reproduced all 12 outcomes in
`build/ita-primitive-check-repeat/summary.json`, with per-configuration logs.
Its comparison outcome is `matched`; golden failures remain explicitly marked
`failed` rather than being relabeled as passing functional tests.


## Parallel workflow (2026-10-04)

Regular runs now fan out into ten independent verification jobs: four smoke
checks, four elaboration checks, strict C910 lint, and the ITA functional
simulation. Survey runs add eighteen distinct variants, for twenty-eight
verification jobs covering all twenty-three variants. There is no matrix
concurrency cap; available GitHub runner capacity determines actual overlap.

Naja and pinned Verilator are built once and shared through runtime artifacts.
Case planning, both builds, and runner unit tests start independently. Frontend
cases do not wait for Verilator. ITA's original and generated build/simulation
pipelines run concurrently; hierarchical generated compilation retains one
compiler job because of the documented Verilator recursive-make issue, while
the original build receives the remaining compiler budget. Per-case failures
do not cancel sibling jobs, and small JSON artifacts feed a combined report.

The updated PULP workflow passes actionlint, and all twenty runner/report unit
tests pass, including a barrier test proving both ITA pipelines overlap. The
regular and survey case plans were evaluated locally against the pinned RTL
archive. The GitHub artifact-transfer path still requires a CI run.

The concurrent ITA run also passed both golden scoreboards and the exact
phase/data/timestamp comparison for 2,048 transactions using vector seed 0,
stalls, and the disclosed 1 ps weight-read delay. The original RTL build
finished while the generated build continued. Full results are in
`build/ita-parallel-verification/summary.json`.
