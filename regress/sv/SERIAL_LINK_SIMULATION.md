<!--
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# Serial Link RTL/netlist simulation exploration

Local results on 2026-11-11: **RTL 16/16 passed; generated netlist 1/16 passed**.
The idle case passes. All 15 traffic cases reach the one-second simulation-time
watchdog without accepting any AXI channel transaction. Functional equivalence
is **not established**. No frontend, ITA, primitive, dependency pin, or shared CI
changes were made.

## Revisions and reproduction

The worktree branch is `serial-link-rtl-netlist-simulation`, created without an
upstream from freshly fetched `origin/main` at
`ff49b7973be4eb18bae3a7df309892cbdd10edfb`. The Naja build reports
`0.7.35 / ff49b797`. It was built and installed locally with CMake in this
worktree; no CVFPU branch code is included.

- Benchmark: `b4bd97823aa93737dda0bf1f3b2bd5c6dcb47ecc`.
- Serial Link: `164d327b6ebe72c8d72c93e9d7c7169fa41d9b16`.
- Verification dependencies: the six exact revisions in that benchmark's
  `bender/serial_link/Bender.lock`; both the lock and checked-out commits are
  verified by the runner.
- Verilator: `5.052 / ea338be98e1e838d3518809ce8899f85a009963c`, the benchmark pin.
  The local macOS build uses Homebrew flex 2.6.4 headers and disables the pinned
  simulator's Tahoe-specific `int` to `size_t` lexer signature rewrite. That
  build-only adaptation is retained under `build/inputs/verilator`.
- Z3: 4.13.3. Seed: 1. Eight concurrent cases; six C++ build jobs.

The matrix used `--constant-delay-fast-path`: equal AXI delay bounds select
that unique value directly, with unequal bounds still randomized. This avoids
repeated SMT calls for `rand_wait(1,1)`. Both models use the same adapted driver
and all upstream checks remain enabled. It changes RNG advancement, so these
results do not claim the exact unadapted driver's seeded stimulus stream.
The precise patch is in `build/fast-verification/axi-fixed-delay.patch`.
An earlier unadapted bidirectional 100/100/100/100 RTL smoke passed, taking
2013.537 seconds with the installed Verilator 5.052 and Z3 5.1.0. The complete
unadapted 16-case matrix was not established.

```bash
python3 regress/sv/serial_link_simulation.py \
  --recipes build/inputs/recipes --output build/fast-verification \
  --najaeda-path build/package \
  --verilator "$PWD/build/inputs/verilator/bin/verilator" \
  --z3 /path/to/z3 --workers 8 --timeout 3600 \
  --constant-delay-fast-path
```

Use a fresh output directory for a new run. Source and simulator hashes are
recorded before simulation. Completed RTL logs can be revalidated with
`--reuse-rtl`; modified sources or binaries are rejected. Failure remains an
unsuccessful exit status. No CI job has been added.

## Coverage and comparisons

All 16 supplied one-channel/eight-lane DDR combinations were run first against
RTL, then against the dump. Each of four plusargs is independently 0 or 100.
The upstream testbench passes `run(NumWrites, NumReads)` to a master API declared
as `run(n_reads, n_writes)`. The runner retains the supplied labels and validates
the effective counts. The matrix covers both interpretations.

The generated boundary matches the upstream 3-bit AXI IDs and includes both
isolation blocks, clock gate and reset control in `slink_isolate`. The synthesis
benchmark's 4-bit-ID `serial_link_synth` wrapper is a different configuration.
A short diagnostic also simulated the concrete wrapper as RTL, confirming
that it accepts the request while the corresponding dump stalls.

Both upstream `axi_chan_compare` checkers, driver assertions, APB setup and
completion checks are retained. The runner requires requested AW/AR/B counts
and W/R burst-ending `last` counts on both sides of both endpoints. The zero
maximum burst-length setting permits bursts up to 256 beats. Every accepted
AW/W/B/AR/R packed payload and exact handshake timestamp is retained, including
IDs, ordering, address attributes, data, strobes, user bits, response and `last`.
Only ordering of simultaneous log messages from independent clocks is ignored.

The RTL matrix observed **558,926 accepted channel beats**, counting both ends
of the links. Every traffic case's netlist trace is empty, so comparison fails
at channel coverage before payload/order/timing equality can be established.
The idle trace matches. The comparison was independently rerun against all saved
logs after checking that the RTL sources and executable hashes were unchanged.

No memory-to-flop transformation pass is invoked. The dump contained zero
NLDB0 memory instances; the frontend lowers the small packed FIFO register
arrays as registers. Verification-side behavioral code remains in the upstream
environment. This experiment does not establish inferred-memory or SRAM coverage.

Exclusions: four-channel traffic, calibration and other upstream testbenches,
additional seeds/configurations, atomic operations (disabled upstream), dedicated
mid-traffic reset campaigns, four-state X/Z equivalence, physical timing and
formal proof. The results use timed Verilator simulation.

## Localized failure and decision before changing lowering

At 14 microseconds, a short diagnostic observes the protocol credit register as
`8` in RTL and `0` in the dump. Both models have enabled clocks and deasserted
reset by then. APB configuration completes and isolation clears in both.

The source asynchronously resets that mixed-bit register while its clock is
gated off. The dump emits a plain `naja_dff` with a data-input reset mux. The
mux cannot respond to an asynchronous reset without a clock edge. Credits stay
zero and the protocol prevents all AXI requests from being accepted. The load
nevertheless reports success and zero unsupported diagnostics.

The independently written [async_mixed_reset_repro.sv](async_mixed_reset_repro.sv)
reproduces this at 7 ns with a 3-bit reset value `101`: RTL returns `101`; its Naja
dump returns `000` and fails the assertion. The standalone testbench is
[async_mixed_reset_tb.sv](async_mixed_reset_tb.sv). Dumps, primitive models,
build logs and run logs are under `build/reset-repro/` locally.

To reproduce from the repository root with a locally built Python package:

```bash
PYTHONPATH=build/package python3 - <<'PY'
from pathlib import Path
from najaeda import netlist
out = Path("build/reset-repro").resolve()
out.mkdir(parents=True, exist_ok=True)
top = netlist.load_system_verilog(
    ["regress/sv/async_mixed_reset_repro.sv"],
    config=netlist.SystemVerilogConfig(
        top="async_mixed_reset_repro",
        diagnostics_report_path=str(out / "diagnostics.log")))
top.dump_gate_level_verilog(str(out / "netlist.v"))
PY
verilator --binary --timing --assert --timescale 1ns/1ps -Wno-fatal \
  --top-module async_mixed_reset_tb --Mdir build/reset-repro/rtl-obj \
  regress/sv/async_mixed_reset_repro.sv regress/sv/async_mixed_reset_tb.sv
build/reset-repro/rtl-obj/Vasync_mixed_reset_tb
verilator --binary --timing --assert --timescale 1ns/1ps -Wno-fatal \
  --top-module async_mixed_reset_tb --Mdir build/reset-repro/netlist-obj \
  build/reset-repro/netlist.v build/reset-repro/naja_primitives.v \
  regress/sv/async_mixed_reset_tb.sv
build/reset-repro/netlist-obj/Vasync_mixed_reset_tb
```

The RTL command passes; the final command currently fails its reset assertion.

The main sequential lowering path in `SNLSVConstructor.cpp` around lines
35161–35231 recognizes all-zero/all-one asynchronous reset values. Another path,
`emitSequentialConditionalAssignment` around line 31078, already partitions
mixed constants into reset/set groups and explicitly avoids a data-input mux.
Existing scalar reset/set primitives therefore appear sufficient; a new vector
reset primitive has not been shown necessary. The next discussion should choose
whether to route this case through the existing mixed-reset implementation or
extend the vector primitive contract. No substitute lowering or primitive-model
change has been implemented in this exploration. No ITA fix was attempted.

## Execution time and checks

| Stage | Wall seconds |
| --- | ---: |
| RTL source preparation, simulator build and all 16 cases | 484.904 |
| Naja elaboration and dump | 2.190 |
| Generated simulator build | 19.805 |
| Generated 16-case matrix and comparison | approximately 1576.452 |
| Full recorded pipeline | 2083.351 (34 min 43 s) |

The timings above exclude Naja/simulator toolchain preparation, preliminary
unadapted probes, debugging, and time waiting for user interaction. Summed case
times exceed wall time because cases run concurrently.

| Supplied case label | RTL seconds | Netlist seconds | RTL accepted beats | Netlist result |
| --- | ---: | ---: | ---: | --- |
| `r0_w0_r0_w0` | 0.429 | 0.391 | 0 | pass |
| `r0_w0_r0_w100` | 71.226 | 609.857 | 13,690 | watchdog |
| `r0_w0_r100_w0` | 109.431 | 674.520 | 26,508 | watchdog |
| `r0_w0_r100_w100` | 136.721 | 725.504 | 30,114 | watchdog |
| `r0_w100_r0_w0` | 73.661 | 614.247 | 16,046 | watchdog |
| `r0_w100_r0_w100` | 134.759 | 658.645 | 27,410 | watchdog |
| `r0_w100_r100_w0` | 177.236 | 726.550 | 38,850 | watchdog |
| `r0_w100_r100_w100` | 188.885 | 774.510 | 39,668 | watchdog |
| `r100_w0_r0_w0` | 107.226 | 678.350 | 27,236 | watchdog |
| `r100_w0_r0_w100` | 194.577 | 682.387 | 37,990 | watchdog |
| `r100_w0_r100_w0` | 237.038 | 739.976 | 54,142 | watchdog |
| `r100_w0_r100_w100` | 241.509 | 797.872 | 56,654 | watchdog |
| `r100_w100_r0_w0` | 151.916 | 671.044 | 29,932 | watchdog |
| `r100_w100_r0_w100` | 185.151 | 726.544 | 36,540 | watchdog |
| `r100_w100_r100_w0` | 240.072 | 815.285 | 58,982 | watchdog |
| `r100_w100_r100_w100` | 281.393 | 849.837 | 65,164 | watchdog |

Focused checker tests cover burst completion, swapped upstream arguments,
missing completion, checker errors, payload/order/timing drift and truncation.
They pass, as do Python compilation, `git diff --check` and REUSE lint. The
standalone reset reproduction intentionally fails for the generated model.
No build-system or public Python API changes were made; no shared CI integration
or Bazel validation was performed for this local simulation exploration.

Local-only evidence (ignored artifacts, not included in the PR):

- [Pipeline summary](../../build/fast-verification/summary.json).
- [Independent transaction comparison](../../build/fast-verification/comparison-summary.json).
- [Naja diagnostic report](../../build/fast-verification/diagnostics.log).
- [Generated netlist](../../build/fast-verification/netlist.v).
- Per-case original/generated logs under `build/fast-verification/<case>/`.
- Short wrapper/reset diagnostics under `build/debug/` and `build/reset-repro/`.
