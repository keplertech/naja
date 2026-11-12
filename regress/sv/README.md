<!---
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# SystemVerilog External Regress

## Serial Link upstream functional validation (local experiment)

See [the local results and reset-lowering finding](SERIAL_LINK_SIMULATION.md):
the documented driver fast-path matrix passes 16/16 RTL cases, while the dump
passes only the idle case. Functional equivalence is not established.

`serial_link_simulation.py` runs the pinned PULP benchmark's `tb_axi_slink`
environment. It completes all 16 original RTL cases before elaborating or
simulating the Naja netlist. The benchmark revision is shared with
`pulp_benchmarks.py`; Serial Link is pinned to
`164d327b6ebe72c8d72c93e9d7c7169fa41d9b16` and the six verification dependencies
are checked against its benchmark `Bender.lock`. Bender and Git must be on PATH.
Initialize `deps/serial_link` in a checkout of that benchmark revision and pass
the checkout as `--recipes`. Inputs are public upstream sources; staged source
copies, patches, logs, traces, generated models and binaries stay under `build/`.

Use the benchmark-pinned Verilator commit
`ea338be98e1e838d3518809ce8899f85a009963c`. The runner records the Z3 version;
the local experiment uses Z3 4.13.3. For example, with a
locally built Naja Python package:

```bash
python3 regress/sv/serial_link_simulation.py \
  --recipes build/inputs/recipes \
  --najaeda-path build/package \
  --verilator "$PWD/build/inputs/verilator/bin/verilator" \
  --z3 /path/to/z3 \
  --constant-delay-fast-path \
  --output build/serial-link-simulation --workers 8 --timeout 3600
```

Use a fresh output directory initially. `--rtl-only` establishes a
baseline without requiring Naja. `--reuse-rtl` checks the saved source/binary
hashes and revalidates all completed RTL logs, running any previously unstarted
cases before netlist work. It rejects modified inputs and incomplete/failed
existing logs. Each simulator/elaboration invocation has a wall-clock bound covering
its children; the testbench also has a one-second simulation-time watchdog.
`summary.json` records revisions, tools, commands, elapsed times, channel counts,
and trace hashes. Full accepted transactions remain in the per-case logs.

The optional `--constant-delay-fast-path` avoids a severe local solver cost in
the upstream AXI driver's `rand_wait`: when `min == max`, assign that sole legal
delay directly and retain its success assertion. For unequal bounds the original
constraint randomization remains intact. Both RTL and netlist use the same
adapted driver. This preserves legal delay values and cycle counts for fixed
waits but changes RNG advancement, so it does **not** reproduce the unadapted
driver's exact seeded traffic stream. The option is recorded in `summary.json`,
and the precise source adaptation is retained in `axi-fixed-delay.patch`.
Omit the option for the exact upstream driver; those write-heavy runs can take
tens of minutes or longer per case on the local machine.

Coverage is the upstream one-channel/eight-lane DDR configuration, two endpoints
with 50 ns and 54 ns system clocks and a 200 ns register clock, and seed 1.
The four independent read/write counts each take values 0 or 100, including the
idle case. Upstream random bursts, backpressure, exclusive accesses, APB setup,
isolation, clock gates, resets and both `axi_chan_compare` checkers remain active.
The pinned testbench passes `run(NumWrites, NumReads)` to a pinned master API
declared as `run(n_reads, n_writes)`: the plusarg labels therefore describe the
opposite operation in this revision. The runner preserves these supplied tests
and checks their actual traffic counts. The complete matrix covers both
interpretations equally. A zero maximum burst length means up to 256 beats,
not single-beat traffic; W/R coverage counts completed bursts via `last` and
records every individual accepted beat.
The benchmark's receive-clock inversion and simulation-completion patches are
applied to both models. There are no added timing delays or ITA changes.

The elaborated boundary includes `slink_isolate`, its clock/reset controls and
both AXI isolation blocks. A concrete wrapper uses the testbench's **3-bit IDs**;
the synthesis benchmark's `serial_link_synth` uses 4-bit IDs and is a different
configuration. The generated netlist replaces both endpoint instances while
retaining the same upstream drivers and checkers. Behavioral verification
memories are outside this elaboration boundary. Inferred NLDB0 memories retain
their behavioral models; no memory-to-flop transformation is invoked. Small
packed register arrays follow the frontend's ordinary lowering.

Success requires upstream `TEST PASSED`, normal completion, no assertion/checker
errors, and exactly the requested AW/AR/B transactions and W/R `last` beats at
all four external AXI interfaces. Every accepted AW/W/B/AR/R packed payload and
its exact handshake timestamp is compared between RTL and netlist, preserving
order within every interface/channel stream. This includes address attributes,
IDs, data, strobes, user bits, responses and burst termination. Ordering of
log messages from independent clocks is ignored; transaction times are not.

Exclusions: four-channel traffic, calibration/channel-allocation testbenches,
additional seeds and configurations, explicit mid-traffic reset campaigns,
atomic operations (disabled upstream), analog/electrical behavior, four-state
X/Z equivalence, physical timing and formal proof. This is functional timed
simulation with Verilator. Shared CI integration is deferred to avoid overlap
with the independent CVFPU work; this runner imports only the existing PULP
runner and adds no CVFPU sources or changes.

This regress checks out pinned external RTL repositories, loads them into Naja,
and runs structural, Verilog generation, lint, or simulation checks.

## Layout

```text
regress/sv/
├── sv_regress.py       # stable runner entry point
├── cases.yml           # pinned repositories and stage configuration
├── cases/              # per-design adapters, requirements, and testbenches
│   ├── axi/
│   ├── borg/
│   ├── cv32e40p/
│   ├── cva6/
│   ├── ibex/
│   ├── ibex_cheriot/
│   └── zcore/
├── common/             # shared analysis helpers
└── tests/              # runner and adapter unit tests
```

Run the focused tests with `python3 -m pytest regress/sv/tests/`.
The CLI and manifest paths are unchanged; stage names such as `helloworld_sim`
remain unchanged too.

## Prerequisites

- Build Naja so `build/test/najaeda` exists.
- Install Python dependencies, including `PyYAML`.
- Install `fusesoc` for the Ibex setup flow.
- Install Docker for the default lint and GitHub smoke-simulation runs.
- Install local `verilator` when using `--lint-runner local` or `helloworld_sim`.
- Install a RISC-V firmware toolchain for `helloworld_sim`. The scripts prefer the
  `riscv32-unknown-elf-` prefix and also accept `riscv-none-elf-` and
  `riscv64-unknown-elf-`. `RISCV` is inferred from `PATH` when unset for
  CV32E40P. CV32E40P's upstream testbench links its firmware against newlib; the
  GitHub workflow installs xPack GNU RISC-V Embedded GCC for that reason.

Lint and simulation stages use the `naja_primitives.v` emitted beside the
generated netlist, including configured firmware and cocotb simulations. This
keeps the models matched to the Naja build and its primitive families. After
updating Naja, rerun `load_dump` rather than reusing an older primitive file.

GitHub CI installs the Python dependencies and uses Dockerized Verilator. The
`External SV Regress` workflow runs lint for the small external designs and
load/dump checks for the large BlackParrot and CVA6 designs, plus Borg's
load/dump, lint, and pin-level cocotb regression. The `External SV
Simulation` workflow runs the checked-in Ibex and CV32E40P smoke simulations
plus the Ibex helloworld simulation, SecureIbex diagnostics, the CV32E40P
helloworld simulation, and an upstream CV32E40P interrupt simulation.

## List Cases

```sh
python3 regress/sv/sv_regress.py list
```

## Borg ASIC Semantic Round-Trip

The `borg` case pins gonsolo/Borg to
`fe915b3763768c7f49be18cc541ad6c96e9de5d8` and targets
`tt_um_gonsolo_borg`. The flow is Chisel-emitted ASIC SystemVerilog → Naja
load/elaboration → structural Verilog dump → upstream pin-level cocotb tests.
No generated Borg RTL is vendored.

Prerequisites in addition to a built Naja/PyYAML environment:

- Mill **1.1.2** (`BORG_MILL` may name an explicit executable). Its launcher
  resolves its versioned JVM/Scala dependencies; the pinned Borg build uses
  Scala 2.13.18 and Chisel 7.15.0, including Chisel's versioned firtool resolver.
  The first run needs network access to Maven Central and the JVM provider.
- A Python environment with `systemrdl-compiler==1.32.2`,
  `peakrdl-cheader==1.1.0`, and Jinja2. Set `BORG_PYTHON` to its Python executable
  if it differs from the Naja runner's interpreter.
- Icarus Verilog (`iverilog`, `vvp`), `cocotb-config` from **cocotb 2.0.1**,
  `riscv-model==0.6.6`, and NumPy. Python 3.13 is supported by this cocotb
  release; Python 3.14 is not. `cocotb-config` must belong to the simulation
  Python environment. The adapter asks it for the Python binary and libpython,
  keeping that ABI separate from Naja's compiled Python bindings.

For example, create a separate environment for RDL/cocotb if Naja is built
against Python 3.14:

```sh
python3.13 -m venv /tmp/naja-borg-tools
/tmp/naja-borg-tools/bin/pip install \
  -r regress/sv/cases/borg/requirements.txt
export BORG_PYTHON=/tmp/naja-borg-tools/bin/python
export PATH="$PATH:/tmp/naja-borg-tools/bin"
```

If Mill is not installed, download the exact version's launcher to a file,
inspect it, then make it executable (no unpinned download or `curl | sh`):

```sh
curl -L --fail \
  https://repo.maven.apache.org/maven2/com/lihaoyi/mill-dist/1.1.2/mill-dist-1.1.2-mill.sh \
  -o /tmp/naja-borg-mill-1.1.2
chmod +x /tmp/naja-borg-mill-1.1.2
export BORG_MILL=/tmp/naja-borg-mill-1.1.2
```

This is Mill's [documented launcher installation flow](https://mill-build.org/mill/cli/installation-ide.html),
with the version fixed for this regression. The generator verifies the actual
Mill version and rejects a mismatch. Nix, PDK libraries, a firmware compiler,
and GNU sed are not required.

Run:

```sh
python3 -m pytest regress/sv/tests/test_sv_regress.py
python3 regress/sv/sv_regress.py list
python3 regress/sv/sv_regress.py run --case borg --stage load_dump
python3 regress/sv/sv_regress.py run --case borg --stage lint --lint-runner local
python3 regress/sv/sv_regress.py run \
  --case borg --stage borg_cocotb_sim --require-firmware-sim-tools
```

Keep `python3` on PATH in Naja's built Python environment. Only `BORG_PYTHON`
and `cocotb-config` need to point to the separate tools environment. As with
other configured simulation stages, missing optional simulation executables
skip unless `--require-firmware-sim-tools` is supplied. **RTL generation is
required and never silently skips**; missing Mill/RDL/firtool dependencies fail
even without that option.

`cases/borg/borg_generate.py` builds in `artifacts/rtl-work`, leaving the pinned checkout
unmodified. Only the pinned `PeakRDL-chisel` submodule is initialized (not Mesa,
Vulkan-Tools, or tapeout tools). The adapter invokes upstream RDL validation
and generation, then `CLOCK_MHZ=4 mill --no-server -j 1 asic.tt.runMain
asic.tt.TTMain` and upstream `init_bram_zero.py`. This preserves the semantics
of `make generate_verilog`, including its zero-initialized cache memory and
translate-on cleanup, without the Nix-specific Python shim or GNU sed.
Debug-trace environment switches are disabled for reproducibility.

Source selection reads **`asic_files.txt`**, resolving its paths relative to
upstream `src/`. TTMain forms that union from each firtool `filelist.f`; the
last `filelist.f` alone is insufficient. Both elaborations retain
`BorgConfig.Asic` and the same 4 MHz clock, preventing configuration-dependent
module overwrites. No broad RTL glob is used to reconstruct the list. SIM and
memory-initializer defines are enabled at Naja loading.

`cases/borg/borg_cocotb_sim.py` compiles exactly:

1. `artifacts/borg_naja.v`;
2. the `naja_primitives.v` model file emitted alongside that netlist;
3. upstream `test/soc/tb.v`.

It imports the upstream Python tests/helpers and the freshly RDL-generated
`borg_mmio.py`, and reuses upstream test vectors. It runs exactly the core
pin-level smoke test `test.test_start` and peripheral ADD/MUL/FMA math test
`user_peripherals.borg.test.test_borg_shader_math_batch`, in separate simulator
processes. Firmware boot, Scala/Chisel unit tests, PDK simulation, FPGA/GDS,
Arcilator, and the framebuffer harness are not included. Chisel
`EphemeralSimulator` tests elaborate typed Chisel modules; they cannot replace
their DUT with this external Verilog netlist.

All generated design/build/simulation products are under
`build/sv-regress/borg/artifacts/`:

- `borg.flist`, `borg-source-selection.json`, and `rtl-work/.../asic_files.txt`:
  audit the exact ASIC sources and configuration;
- `borg-*-command.json`: exact RDL/Mill/post-step commands and working dirs;
- `borg_naja.v`, `design-stats.json`, `diagnostics.log`, and `summary.json`;
- `borg-cocotb-sim-build-command.json`: actual three-source Icarus command;
- `borg-cocotb-sim-{core,math}-run-command.json`: exact vvp commands and
  simulation environment;
- `borg-cocotb-sim/{core,math}-results.xml` and
  `borg-cocotb-sim-results.json`: explicit selected-test results;
- stable `logs/borg-cocotb-{build,core,math}.log` and combined
  `logs/borg-cocotb-sim.log`.

The pass indication is **`BORG_COCOTB_SIM_PASS`**, emitted only after fresh XML
reports exactly the expected passing test in both runs. Simulator exit alone,
missing/empty XML, selected-test skips, failures, errors, and stale results
cannot pass. Cocotb's explicitly filtered-out tests may appear as skipped XML
entries; these do not count as executed tests. The artifact-local `precision.f`
sets Icarus's [default timescale](https://steveicarus.github.io/iverilog/usage/command_files.html)
to 1ns/1ps for the structural dump and primitives, so upstream helpers' 2 ps
delays can be represented without modifying the 1ns/100ps upstream testbench.
Waveform dumping is disabled with vvp's `-none` argument to bound artifact size.

This is a **Naja structural netlist**, with portable behavioral models of
Naja's inferred operators, flops, and memories—not a technology-mapped PDK
gate-level netlist. The adapter never sets `GATES=yes`/`GL_TEST`, invokes the
upstream simulation Makefile, or compiles original Chisel-emitted RTL alongside
the dump. PDK cell models and power-pin glue are deliberately excluded.

Local macOS verification passed all three stages with 37 selected ASIC input
sources and no Naja unsupported diagnostics. Cold Scala/Chisel generation took
about 69 seconds; warm generation about 9 seconds, Naja load/dump about 13
seconds, local lint 21–27 seconds (about 2 GB allocated), and the complete
cocotb adapter about 110 seconds (including compile/startup). Both tests passed:
core smoke about 39 seconds and peripheral math about 47 seconds.

The central `External SV Regress` workflow (`.github/workflows/sv-regress.yml`)
runs all three Borg stages on pull requests and pushes to `main`, and supports
manual dispatch. It installs Icarus, the pinned Mill 1.1.2 launcher, and
`cases/borg/requirements.txt` in a separate Python 3.13 environment. Only Mill
and `cocotb-config` are added to PATH, preserving the runner's Python 3.12
interpreter and Naja bindings. The job uses `--require-firmware-sim-tools`, so
missing simulation tools fail rather than skip. Borg's logs and test results
are included in the workflow's always-uploaded `sv-regress-artifacts` artifact.

For a second tier, BorgSimTop plus the existing Verilator `cts-uart` replay and
`compare_ppm.py` golden-image comparison is a practical candidate. At this pin,
`BorgSimMain` emits the four-lane `BorgConfig.Simt` design into a separate
directory and collects `sim_files.txt`; it must not share the ASIC build's
module files. The C++ wrapper loads/reads SDRAM through public `dbg_*` top-level
ports, rather than relying on internal signal names. That tier would need a
Naja-only Verilator source adapter, a firmware toolchain/build, support for the
simulation SDRAM model, and a runtime/memory budget for up to 100 million
cycles. Upstream's runner compares the captured render with the pinned golden
using `--max-diff 1 --max-fail-pixels 2`. It is deliberately not implemented here.

## Default Run

The default `run` stages are `lint` and `github_sim`.

```sh
python3 regress/sv/sv_regress.py run --case ibex --case cv32e40p
```

This matches the lightweight CI intent: Verilator lint plus checked-in smoke
simulation testbenches.

## Large Design Load/Dump

BlackParrot and CVA6 are too large for GitHub-hosted simulation jobs, so CI
only checks that Naja can load, elaborate, and dump them back to Verilog:

```sh
python3 regress/sv/sv_regress.py run \
  --case black_parrot \
  --case cva6 \
  --stage load_dump
```

The generated Verilog and `load-dump.log` are written under each case artifact
directory.

## Z-Core Full Dump Verification

The Z-Core case follows the pinned upstream `tb/Makefile` source-list and
simulation model. It elaborates `z_core_top`, dumps the complete reachable
design, lints that complete dump, and runs a top-level reset/execution smoke
simulation against the same dump:

```sh
python3 regress/sv/sv_regress.py run \
  --case zcore \
  --stage load_dump \
  --stage lint \
  --stage github_sim
```

The simulation expects `ZCORE_TOP_SMOKE_PASS` after reset and 200 full-SoC
clock cycles, and rejects unknown UART or GPIO outputs.

## Logic-Cone Signatures

The `logic_cones` stage builds manifest-selected cones while the pinned external
design is loaded, before dumping Verilog. It checks stable structural counts:
nodes, edges, leaves, roots, registers, ports, blackboxes, and internal nodes.

```sh
python3 regress/sv/sv_regress.py run \
  --case cva6 \
  --stage logic_cones
```

Probe roots and expected counts live under `logic_cones` in `cases.yml`. Actual
results are always written to
`build/sv-regress/<case>/artifacts/logic-cones.json`, including on a signature
mismatch, so an intentional update can be reviewed rather than guessed.

## Ibex Smoke Simulation

Run lint and the Ibex smoke simulation:

```sh
python3 regress/sv/sv_regress.py run --case ibex --stage lint --stage github_sim
```

Run only the Ibex smoke simulation stage:

```sh
python3 regress/sv/sv_regress.py run --case ibex --stage github_sim
```

The expected pass marker is:

```text
IBEX_SMOKE_PASS
```

The main smoke log is:

```text
build/sv-regress/ibex/artifacts/logs/github-sim.log
```

## Ibex CHERIoT Co-simulation

The `ibex_cheriot` case pins the CHERIoT Ibex (`lowRISC/ibex`, commit
`7a4697f5`) and its `ibex_cosim` stage checks Naja's elaboration against the RTL
itself, in six parameter configurations: `default`, `pmp` (16 regions), `icache`
(ECC and scrambling), `cheriot`, `rv32b` (full bit-manipulation) and `opentitan`
(the `ibex_configs.yaml` opentitan set, with CHERIoT).

For each configuration, `cases/ibex_cheriot/ibex_cosim.py`:

1. loads `ibex_top` twice, in separate processes, and requires the two Verilog
   dumps to be identical (determinism);
2. builds the original RTL and the Naja dump as two Verilator models. The RTL
   model only depends on the Ibex commit, the file list and the Verilator
   version, so it is cached under `--rtl-cache`;
3. drives both models in lockstep with the same random stimulus (fixed seeds)
   and compares every `ibex_top` output after both clock edges;
4. on a mismatch, rebuilds both models with VCD tracing, re-runs the failing
   seed up to the first mismatch, and prints the first diverging internal
   signals.

Requirements: a built Naja, Verilator 5 (CI uses 5.052) and a C++20 compiler.
Sources are listed in `cases/ibex_cheriot/ibex_top.f`, relative to the Ibex
checkout; configuration parameters are added with `-G`.

```sh
python3 regress/sv/sv_regress.py run --case ibex_cheriot --stage ibex_cosim
```

Seeds and length default to `1 2 3` and 20000 cycles; set `IBEX_COSIM_SEEDS`
and `IBEX_COSIM_CYCLES` to change them. The script can also run on its own,
for example on one configuration:

```sh
python3 regress/sv/cases/ibex_cheriot/ibex_cosim.py --repo <ibex checkout> \
  --artifacts build/ibex-cosim --config icache --seeds 1 --cycles 5000
```

The expected pass marker is `IBEX_COSIM_PASS`. Results are in
`build/sv-regress/ibex_cheriot/artifacts/ibex-cosim/summary.json` and the log in
`build/sv-regress/ibex_cheriot/artifacts/logs/ibex-cosim.log`. The
`Ibex CHERIoT Co-simulation` workflow runs this stage on pull requests (3 seeds
x 20000 cycles, a few minutes) and nightly with 8 seeds x 200000 cycles.

## CV32E40P Smoke Simulation

Run lint and the CV32E40P smoke simulation:

```sh
python3 regress/sv/sv_regress.py run --case cv32e40p --stage lint --stage github_sim
```

Run only the CV32E40P smoke simulation stage:

```sh
python3 regress/sv/sv_regress.py run --case cv32e40p --stage github_sim
```

The expected pass marker is:

```text
CV32E40P_SMOKE_PASS
```

The main smoke log is:

```text
build/sv-regress/cv32e40p/artifacts/logs/github-sim.log
```

## Local Verilator Lint

Docker remains the default lint runner. To use the local `verilator` binary for
lint:

```sh
python3 regress/sv/sv_regress.py run --case ibex --stage lint --lint-runner local
```

For both smoke cases:

```sh
python3 regress/sv/sv_regress.py run --case ibex --case cv32e40p --stage lint --lint-runner local
```

The lint log is:

```text
build/sv-regress/<case>/artifacts/logs/verilator-lint.log
```

## Helloworld Simulation Tier

The `helloworld_sim` stage is opt-in and runs real firmware in upstream-style
testbench environments:

- Ibex uses upstream `examples/simple_system`, replaces the original
  `ibex_top` with the Naja-generated `ibex_top`, builds `hello_test.vmem`, and
  expects `IBEX_HELLOWORLD_SIM_PASS`.
- The `ibex_dit_sim` and `ibex_dummy_instr_sim` stages reuse the same upstream
  simple-system environment to run the self-checking `dit_test` and
  `dummy_instr_test` programs. They generate a separate `ibex_secure_naja.v`
  with `-GSecureIbex=1`, then compile the upstream simple-system wrapper with
  matching SecureIbex memory-integrity wiring.
- The `ibex_pmp_sim` stage generates a separate `ibex_pmp_naja.v` with
  `-GPMPEnable=1`, then runs upstream `pmp_smoke_test` and checks that the
  protected store raises the expected store access fault.
- The `ibex_secure_dit_sim` and `ibex_secure_dummy_instr_sim` stages generate a
  separate `ibex_secure_naja.v` with `-GSecureIbex=1`, then compile the upstream
  simple-system wrapper with matching SecureIbex memory-integrity wiring. These
  are compatibility aliases for the secure configuration.
- CV32E40P uses upstream `example_tb/core`, replaces the original
  `cv32e40p_top` with the Naja-generated `cv32e40p_top`, builds
  `custom/hello_world.hex`, compiles the upstream Verilator testbench around
  the generated netlist, and expects `CV32E40P_HELLOWORLD_SIM_PASS`.
- The `cv32e40p_hwlp_sim` stage generates a separate `cv32e40p_pulp_naja.v`
  with `-GCOREV_PULP=1`, patches the upstream `hwlp_test` hardware-loop
  mnemonics into raw instruction words for stock RISC-V GNU toolchains, and
  expects `CV32E40P_HWLP_SIM_PASS`.
- CVA6 uses the upstream Verilator `ariane_testharness` and FESVR loader from
  the CVA6 repository, wraps the generated elaborated `cva6` netlist with a
  small `ariane` compatibility module, runs upstream `hello_world.c`, and
  expects `CVA6_HELLOWORLD_SIM_PASS`.

Case-specific stages must be launched per case:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage lint \
  --stage github_sim \
  --stage helloworld_sim \
  --stage ibex_pmp_sim \
  --stage ibex_dit_sim \
  --stage ibex_dummy_instr_sim

python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage lint \
  --stage github_sim \
  --stage helloworld_sim \
  --stage interrupt_sim \
  --stage cv32e40p_hwlp_sim

python3 regress/sv/sv_regress.py run \
  --case cva6_testharness \
  --stage lint \
  --stage helloworld_sim \
  --lint-runner local \
  --require-firmware-sim-tools
```

Run only helloworld simulation:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --case cv32e40p \
  --stage helloworld_sim
```

Missing optional firmware simulation tools are reported as skipped. To make missing
tools fail the command:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage helloworld_sim \
  --require-firmware-sim-tools
```

Run the extended Ibex simple-system diagnostics:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage ibex_pmp_sim \
  --stage ibex_dit_sim \
  --stage ibex_dummy_instr_sim \
  --require-firmware-sim-tools
```

Run the CV32E40P hardware-loop diagnostic:

```sh
python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage cv32e40p_hwlp_sim \
  --require-firmware-sim-tools
```

The helloworld simulation log is:

```text
build/sv-regress/<case>/artifacts/logs/helloworld-sim.log
```

## CV32E40P Extended Simulation Tier

The `interrupt_sim` stage runs the CV32E40P upstream `example_tb/core`
interrupt program with the same generated `cv32e40p_top` netlist and expects
`CV32E40P_INTERRUPT_SIM_PASS` when the generated netlist handles the interrupt
sequence correctly:

```sh
python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage interrupt_sim \
  --require-firmware-sim-tools
```

The `cv32e40p_hwlp_sim` stage runs the upstream `hwlp_test` program against a
`COREV_PULP=1` generated netlist:

```sh
python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage cv32e40p_hwlp_sim \
  --require-firmware-sim-tools
```

The extended CV32E40P simulation logs are:

```text
build/sv-regress/cv32e40p/artifacts/logs/interrupt-sim.log
build/sv-regress/cv32e40p/artifacts/logs/cv32e40p-hwlp-sim.log
```

For local diagnosis, pass `--sim-plusarg +naja_irq_debug` to
`regress/sv/cases/cv32e40p/cv32e40p_example_tb.py` to print the interrupt mode,
pending word, visible IRQ lines, acknowledge ID, and core PC. Use
`--sim-plusarg +naja_irq_entry_debug` for a focused 96-cycle trace once Test 2
raises all IRQs. The same helper also accepts `--core-source rtl`, which builds
the upstream CV32E40P RTL from `cv32e40p_manifest.flist` into a separate
`interrupt_sim_rtl` work directory for reference comparison.

The core-v-verif UVM environment remains outside the GitHub open-tool tier
because it needs a full SystemVerilog/UVM commercial simulator stack. Its
Verilator-compatible CV32E40P core-testbench path overlaps with the upstream
CV32E40P core testbench used here, so the CI tier focuses on expanding those
firmware simulations.

## Artifacts

Per case, artifacts are written under:

```text
build/sv-regress/<case>/artifacts/
```

Useful files include:

```text
summary.json
logs/generate.log
logs/load-dump.log
logs/verilator-lint.log
logs/github-sim.log
logs/helloworld-sim.log
logs/ibex-dit-sim.log
logs/ibex-dummy-instr-sim.log
logs/interrupt-sim.log
logs/cv32e40p-hwlp-sim.log
ibex_secure.flist
ibex_secure_diagnostics.log
ibex_pmp.flist
ibex_pmp_diagnostics.log
cv32e40p_pulp.flist
cv32e40p_pulp_diagnostics.log
verilator-lint-command.json
github_sim-build-command.json
github_sim-run-command.json
helloworld-sim-command-<n>.json
ibex-dit-sim-command-<n>.json
ibex-dummy-instr-sim-command-<n>.json
interrupt-sim-command-<n>.json
cv32e40p-hwlp-sim-command-<n>.json
```

The legacy `verilator-command.json` is still written for compatibility and
contains the lint command.

## Clean

Remove the SV regress work directory:

```sh
python3 regress/sv/sv_regress.py clean
```

## PULP prepared EDA benchmarks

`pulp_benchmarks.py` uses a fixed Git commit of
[PULP EDA benchmarks](https://github.com/pulp-platform/eda-benchmarks).
`REVISION` in `pulp_benchmarks.py` is the single source pin shared by the local
runner, nightly survey, and ITA simulation input package. Update that constant
when advancing the benchmark suite; publication of a release is not required.
The runner checks out that exact commit and runs upstream `setup.sh` with
`SIMULATION=0 KEEP_BUILD_TOOLS=0`. It applies no Naja-owned RTL patches.
Exports are cached under `build/pulp-benchmark-sources/` by revision, with a
manifest recording the repository, commit and every exported file's SHA-256.
Cached directories and supplied archives are verified before use. Each target
uses the export's `targets.mk` top and variant-local `filelist.f`, including
upstream wrappers, include directories, defines and memory stubs.

Preparing an export requires Linux and upstream's build prerequisites:

```sh
sudo apt-get install bash build-essential ca-certificates cmake coreutils curl \
  findutils gawk git libxml-simple-perl openjdk-17-jre-headless perl \
  python3-venv sed tar xz-utils
python3 regress/sv/pulp_benchmarks.py --prepare-only
```

Other platforms can reuse a Linux-prepared export with `--package <directory>`
or `--archive <tar.gz>`. These must contain the runner's pinned-source manifest;
the former v0.1.0 release archive is no longer an input to this runner.

```sh
# PR tier: CV32E40P, FP32 FMA, minimal Ibex, SPI host, NVDLA, and minimal Snitch cluster.
# Prepares/caches the pinned Git revision unless a prepared input is supplied.
python3 regress/sv/pulp_benchmarks.py --lint-runner local

# Complete, bounded survey; known failures deliberately return nonzero.
python3 regress/sv/pulp_benchmarks.py --all --timeout 90

# Individual cases and custom build/package locations are supported.
python3 regress/sv/pulp_benchmarks.py \
  --case ibex.full --timeout 600 \
  --archive /tmp/pulp-benchmarks.tar.gz \
  --najaeda-path build/test/najaeda --output build/pulp-full-ibex
```

Use the Python interpreter matching the built Naja extension (Python 3.12+
for archive extraction). The runner itself needs only the standard library;
the built `najaeda` package must also have its normal runtime dependencies.
`--lint-runner docker` uses the same Verilator v5.046 image as the external SV
runner; `local` requires Verilator on PATH. Omitting the option runs load/dump
only. Each load/dump and lint subprocess has a separate wall-clock limit,
defaulting to 600 seconds (10 minutes).
Lint uses `--verilate-jobs 0` to use the CPUs available to Verilator for internal
compilation. Missing lint tools fail; they do not silently skip.

The `PULP SV Regress` workflow prepares the pinned Git revision once, caches its
verified export, and shares a prepared archive across independent jobs. The
nightly survey uses this same source pin. Case-specific options go through
`SystemVerilogConfig.frontend_options`: Cheshire uses
`{"slang": ["--allow-use-before-declare"]}`, and RedMulE uses
`{"slang": ["--relax-enum-conversions"]}` for its integral-to-enum port connections.
Other cases retain these strict defaults. The runner records the complete
`frontend_options` mapping in each case's JSON result and applies it in load/dump
and elaboration-only modes. Naja is built using the canonical Linux Release
configuration, runs the small tier and lints each complete generated netlist
with the matching `naja_primitives.v` emitted by Naja beside each dump.
Both local and Docker lint use these generated models, including parameterized
division/remainder primitives; Docker mounts the artifact directory once for
both inputs. Dumps containing no Naja primitives need only the netlist file.
Worker and lint timeouts terminate the full process group, so local Verilator
launcher children do not survive a timed-out case. A second tier elaborates
`mempool.minpool`, `ita.default`, `cvfpu.full`, and `cva6.default`, with a 600-second limit per
case and the same strict diagnostic checks. This tier does not dump or lint
netlists; its artifacts and aggregate summary live in
`build/pulp-benchmarks/elaboration/`, preserving the complete-dump summary.
To reproduce it locally:

```sh
python3 regress/sv/pulp_benchmarks.py \
  --case mempool.minpool --case ita.default --case cvfpu.full --case cva6.default \
  --elaboration-only --timeout 600 --output build/pulp-benchmarks/elaboration
```

C910 also runs on every PR, push, and dispatch in an independent `pulp-c910`
job. It uses the same source-pinned Verilator 5.052 build as ITA and a 600-second
limit for each load/dump and strict-lint stage. The prepared PULP runner enables
`GateLevelVerilogDumpConfig(verilatorSplitPackedSignals=True)` for its dumps. This
tracks packed child-module ports and internal buses separately, leaves public
top-level ports unannotated, and does not suppress combinational-cycle warnings.

The workflow builds Naja once and distributes a checked Python 3.12 native
runtime to the verification jobs. The prepared RTL download/matrix planning,
Naja build, pinned Verilator build, and runner unit tests start independently.
Every consumer checks the restored native runtime before running a benchmark.
There is no workflow `max-parallel` cap; GitHub schedules jobs according to the
account's available runner concurrency.

Regular runs fan out into **13 independent verification jobs**: seven complete
smoke checks, four larger-design elaboration checks, C910 strict lint, and ITA
functional validation. A nightly run at 02:23 UTC or manual `survey=true` adds
15 further complete-check jobs, covering **all 23 release variants** together
with the eight regular full checks. That produces **28 independent verification
jobs**, including the four elaboration checks and ITA. Each additional survey
variant has its own 600-second limit per load/dump and per lint, matching C910.
Frontend matrix jobs have a 30-minute overall limit to allow both stages,
setup, container cleanup, and artifact publication. Known failures fail only their own jobs.
Matrix fail-fast is disabled, so other variants finish and retain reports.
No complete-check case is duplicated between regular and survey jobs.
The regular smoke set includes `nvdla.top` alongside CV32E40P, FP32 FMA,
minimal Ibex, SPI host, `snitch_cluster.minimal`, and `ara.default`. The Snitch
cluster and Ara checks load, dump, and lint the complete design on every regular
run. Ara retains the same strict lint and 600-second stage limits as the survey;
a lint failure or timeout fails its independent regression job.
NVDLA uses `frontend_options={"slang": ["--timescale=1ns/1ps"]}` for
modules without an explicit time scale; source time scales still take precedence.
The runner records this fallback in each case's `summary.json`.

The survey and frontend checks do not establish functional correctness. The
separate ITA job adds functional simulation, not formal equivalence. No
technology library is needed.

Naja uses its ordinary strict loading behavior. The upstream Yosys flow's
`--compat-mode`, `--compat=vcs`, `--allow-use-before-declare`,
`--best-effort-hierarchy`, and variant-specific `yosys-slang.args` are not
silently forwarded or emulated. In particular, unknown-module blackboxing is
disabled. Naja unsupported diagnostics fail the run. Ordinary Slang warnings,
2-state comparison warnings, and memory-inference fallbacks remain in reports;
a fallback to generic sequential lowering is not itself an unsupported error.

Artifacts default to `build/pulp-benchmarks/<design>.<variant>/` with an aggregate
`summary.json` in the parent directory. Each case has its own process, so a
crash, exception, or timeout does not prevent subsequent cases from running.
A timeout is an incomplete measurement, not proof of unsupported RTL. Stale
netlist/statistics files are removed before each case. Temporary extracted RTL
is removed afterward; the archive checksum and copied variant filelist make
source selection reproducible.

The initial [23-variant survey and CI selection](PULP_RESULTS.md) records the
local build, timing limits, diagnostics, and remaining verification limits.

### ITA upstream functional validation (experimental)

`ita_simulation.py` reuses the upstream ITA test generator and seven-phase
scoreboard at the exact revision used by the prepared release. It runs both
the prepared RTL and the Naja structural dump against the same seed-0,
64-by-64 attention/feedforward vectors, with bias and randomized stalls.
The PULP workflow runs this command in an independent `pulp-ita` job on PRs,
pushes, nightly runs, and manual dispatch. CI pins Verilator 5.052 to commit
`ea338be98e1e838d3518809ce8899f85a009963c`, caches its installation, and uses
NumPy 2.5.3 and ONNX 1.23.1 in a separate generator environment. It runs vector
seed 0 with stall seed 1, requiring both golden scoreboards and the exact
2,048-transaction phase/data/timestamp comparison to pass. Each subprocess
has a 1,200-second timeout. Additional seeds remain a local validation option.
A passing elaboration must not be interpreted as passing this test.
CI enables `--parallel-simulations`: the original RTL and generated netlist
build/simulation pipelines run concurrently in separate directories and logs.
`--jobs` is a total compiler-job budget shared by the active pipelines (at least
two jobs are required for concurrent execution).
The generated hierarchical build keeps its required single compiler job to
avoid the known Verilator recursive-make issue. Scoreboard validation and trace
comparison still require both independent runs to pass. Local runs remain
sequential unless the option is supplied.


Each matrix job publishes its own summary and `pulp-sv-regress-<tier>` artifact,
including failure logs. ITA artifacts retain the raw dump, adapted simulation
sources, vector hashes, commands, and scoreboard logs; extracted upstream
sources, archives, and compiled simulator objects are excluded.

```sh
python3 -m venv /tmp/ita-vectors
/tmp/ita-vectors/bin/pip install numpy onnx
python3 regress/sv/ita_simulation.py \
  --archive /tmp/pulp-benchmarks.tar.gz \
  --generator-python /tmp/ita-vectors/bin/python \
  --najaeda-path build/test/najaeda
```

Use the interpreter matching the compiled Naja extension for the runner.
Verilator with `--binary --timing` support and a C++ toolchain must be on PATH.
`--no-stalls`, `--seed`, `--jobs`, `--timeout` (per subprocess), and
`--verilator-opt` support diagnosis. By default,
`--hierarchical` with Verilator's default optimizations compiles the large generated controller,
dot-product, requantizer, and softmax modules as separate blocks; this avoids
flattening their full combined simulation model during Verilation.
The local optimized flat build hit an internal Verilator fault, and a flat
`-O0` build exceeded 300 seconds. `--no-hierarchical` allows reproducing these
compiler issues. Failures and timeouts return nonzero.
Hierarchical builds use one build job to avoid a Verilator 5.052 recursive
makefile regeneration race; `--jobs` controls the original-RTL build.
Flat and hierarchical generated models use separate build directories.

All downloaded sources are commit-pinned and SHA-256 checked. The original
simulation replaces the synthesis SRAM stub with the matching upstream
behavioral SRAM. The generated simulation attaches that same model to its
otherwise empty SRAM leaf, checking the pinned variant's exact port shape;
the raw dump is preserved. This validates the surrounding logic with an
external memory model, not Naja lowering of SRAM behavior.

Both simulations apply a **1 ps propagation delay to the latch-based weight
buffer's read port**. Verilator 5.052 otherwise exposes a transient previous
write-data value to a same-edge reader when a bank is reused. The generated
buffer's gates and latches are retained; only its output connection gains the
same delay as the RTL. This is a timed simulation model, not zero-delay
simulation or timing signoff. It adds no full clock cycle at the 2 ns clock
period. The unmodified dump remains in `dump/netlist.v`.

`--weight-read-delay-ps 0` disables the model to reproduce the original
zero-delay failures; the default is 1 and the accepted range is 0..100 ps.
The report records `weight_read_delay_ps`, so a passing modeled run is not
confused with a zero-delay run. Delaying the cascaded clock gates themselves
is not equivalent and is not used.

`ita_latch_repro.sv` is a small independent reproducer. With its default 1 ps
read delay, these commands must both pass:

```sh
iverilog -g2012 -s ita_latch_repro -o /tmp/ita-latch-repro \
  regress/sv/ita_latch_repro.sv regress/sv/ita_latch_dut.sv
vvp /tmp/ita-latch-repro
verilator --binary --timing --top-module ita_latch_repro -Wno-fatal \
  --Mdir /tmp/ita-latch-verilator regress/sv/ita_latch_repro.sv regress/sv/ita_latch_dut.sv
/tmp/ita-latch-verilator/Vita_latch_repro
```

For the zero-delay control, add `-Pita_latch_repro.READ_DELAY_PS=0` to Icarus
or `-GREAD_DELAY_PS=0` to Verilator. Locally, Icarus captures `0x11` as expected;
Verilator 5.052 captures the transient `0x22` and fails the assertion.

To repeat the elaboration/primitive investigation, with Icarus, Verilator,
and the built najaeda module available:

```sh
PYTHONPATH=build/test/najaeda python3 regress/sv/ita_primitive_check.py
```

This elaborates only `ita_latch_dut.sv`; testbench timing stays outside Naja.
It runs 12 configurations: RTL, generated netlist with the existing primitive,
and generated netlist with a temporary nonblocking latch candidate, each on
both simulators with 0/1 ps read propagation. JSON results and build/run logs
are saved in `build/ita-primitive-check/`. The candidate changes only an
artifact copy of the emitted `naja_primitives.v`.

Exit success means all models agree within each simulator/delay combination
and all 1 ps controls pass; **it does not mean zero-delay golden checks pass**.
Each `golden_status` remains explicit. On Verilator 5.052 the NBA candidate
still fails at zero delay, with a `COMBDLY` warning; Icarus and Naja-generated
logic agree with the source RTL. These results do not justify a change to
Naja elaboration or its generic latch primitive for this issue.

The testbench adapter reads packed-array `$fscanf` targets through scalar
temporaries to avoid invalid Verilator-generated C++, checks scan success,
and requires all eight groups of 256 output transactions (phase 3 has two
groups). Like upstream, it continues checking after a golden mismatch;
the runner rejects any log containing a mismatch. It also records all 2,048
output transactions and compares their phase, value, and time between RTL
and netlist. That comparison is reported separately and cannot override a
failed golden check. It does not replace expected outputs with RTL results.
Simulation warnings are retained in logs rather than made
fatal; this runner is separate from strict netlist lint.

Artifacts, commands, timings, and stage outcomes are written under
`build/ita-simulation/`, including `summary.json`, `original-run.log`, and
`generated-build.log`. Both simulations must pass for overall success.

### PULP job report

The PULP workflow publishes a GitHub Actions job summary and includes `report.md`
in each `pulp-sv-regress-*` artifact, even when a regression fails. Each selected
case reports its result, load/statistics/dump/lint timings, and total worker plus
lint time. Unselected release variants and unavailable results are explicit.
Elaboration jobs are independent of smoke results. A final report job downloads
only the small `pulp-summary-*` artifacts and combines every available JSON
result into `pulp-combined-report`, without downloading the generated netlists.
Preparation/runtime artifacts expire after one day; result artifacts remain
available for seven days.

The benchmark worker uses the najaeda `count_child_instances()`, `count_terms()`,
`count_bit_terms()`, `count_nets()`, and `count_bit_nets()` APIs on the elaborated
top. Counts describe the top model, not flattened hierarchy or mapped cell area.
Statistics are saved before dumping, so a later dump failure can retain them.

Regenerate the report from existing JSON without loading najaeda:

```bash
python3 regress/sv/pulp_report.py --root build/pulp-benchmarks
# Downloaded job artifacts must each retain their own subdirectory.
python3 regress/sv/pulp_report.py --aggregate-root build/pulp-job-artifacts
```

An optional `--functional build/ita-simulation/summary.json` includes the
experimental ITA upstream scoreboard and RTL/netlist trace comparison as
separate results. The workflow supplies `build/pulp-benchmarks/ita/summary.json`
for its ITA job. Setting `GITHUB_STEP_SUMMARY` appends the report to the GitHub
job page.


### Lowering efficiency audit

Every prepared-PULP load also saves `lowering-efficiency.json`. This audits
netlist connectivity independently of the frontend and reports local and
hierarchy-expanded mux counts, identical input bits, duplicate muxes, repeated
selection arms, and unconsumed mux cones, grouped by model and source origin.
The runner logs a warning per model/category at 256 expanded avoidable bits.
These findings do not fail elaboration or replace structural lint; categories
can overlap and must not be added. Necessary large mux networks alone do not
trigger a warning. Non-mux consumers are conservatively considered live roots.
The same audit is available as `top.get_lowering_efficiency_report()`; pass
`warn_threshold=None` for a silent report or a positive integer to tune warnings.

### CVFPU FP32 FMA functional simulation

`cvfpu_simulation.py` runs the pinned PULP CVFPU UVM environment against both
the prepared RTL and the Naja structural dump. It requires both upstream
scoreboards to pass, positive reference-model coverage, and exact agreement of
DUT input/output transactions, exception flags, tags, ready/valid/reset/flush
signals and timestamps. Its bind monitor samples before the active clock edge;
it does not alter the DUT. The report records checked and skipped vectors for
each test/seed and the Naja producer identity.

The target is `cvfpu.fp32_fma`, whose exported wrapper uses FP32 arithmetic and
`NumPipeRegs=0`. This checks the elaborated arithmetic and interface behavior;
it does not establish coverage of registered FMA configurations or the full FPU.
The upstream seven arithmetic operations, six available test classes, and
documented reference-model exclusions are retained. The default is 50 seeds
per class with ten requested transactions, for 300 paired testcases. A testcase
with no vectors left for reference checking fails.

Use Python 3.12+ matching the built Naja extension, Git, a C++ toolchain,
MPFR/GMP development libraries discoverable through `pkg-config`, Verilator
`08bbef72acdc8f3ec5b27e3275e75475bc974e2b` (5.053 development), and Z3 4.13.3.
The upstream PULP installer is `flows/tools/install-verilator.sh` in its source
checkout (`tools/install-verilator.sh` in its prepared simulation export).
For example:

```sh
python3 regress/sv/cvfpu_simulation.py \
  --package /path/to/prepared/pulp-benchmarks \
  --najaeda-path build/test/najaeda \
  --verilator /path/to/pinned/verilator --z3 /path/to/z3
```

`--seeds 1` runs one seed per class; repeatable `--test` selects individual
classes. `--timeout` bounds each build and simulation separately (default
1,200 seconds), and `--jobs` sets the compiler budget. Both builds run
sequentially in separate directories.

The runner downloads commit-pinned verification sources into `--inputs`
(default `build/cvfpu-sources`) and stages their committed contents under the
output directory. It verifies the PULP source archive and Accellera UVM 1.1d
archive checksums and applies the pinned public PULP testbench patches. Updating
the shared PULP `REVISION` also requires updating this runner's source archive
checksum. On macOS, staged UVM allocation includes use `stdlib.h` in place of
Linux's `malloc.h`. DUT RTL and Naja primitives receive no simulation patch.

Artifacts default to `build/cvfpu-simulation/`: `summary.json`, strict lowering
diagnostics, original/generated filelists, raw dump and primitive models, build
logs, and separate per-test/seed simulation logs. Retain source/license notices
in local caches; exclude `sources/`, source archives and `*-obj/` compiled
simulator build trees from diagnostic artifacts. The short-lived simulator
transfer artifact contains only the two executables and build manifest. This runner is independent of
`ita_simulation.py` and does not require its SRAM changes.

#### GitHub Actions coverage and parallel execution

The PULP workflow builds the pinned CVFPU compiler and both simulators once,
then shares the simulator executables and build manifest with four Ubuntu
24.04 matrix jobs. Each shard runs two pairs concurrently, with a separate
working directory and logs for every test/seed. RTL and netlist executions
within a pair remain sequential. The shard timeout is 120 minutes.

Pull requests and pushes to `main` use two seeds for each class: 12 pairs,
three per shard. Nightly runs and default manual runs use all 50 seeds:
300 pairs, 75 per shard. A manual run can disable `cvfpu_full` to select the
smoke suite. Summaries record per-case timings, total simulation wall time,
worker count and shard identity. The combined report checks that all four
shards cover the selected matrix exactly once, with matching producer and
coverage settings.

To reproduce the artifact handoff locally:

```sh
python3 regress/sv/cvfpu_simulation.py --phase build \
  --package /path/to/prepared/pulp-benchmarks \
  --najaeda-path build/test/najaeda \
  --verilator /path/to/pinned/verilator --z3 /path/to/z3
# Transfer build-summary.json and both *-obj/Vtop executables together,
# preserving their relative paths and executable permissions.
python3 regress/sv/cvfpu_simulation.py --phase run \
  --workers 2 --shard-index 0 --shard-count 4 --seeds 50 --z3 /path/to/z3
```

Run the second command for shard indices 0 through 3 in separate output
directories restored from the same build artifact. Run-only jobs need the
pinned solver and MPFR/GMP runtime libraries; they do not require Naja, the
verification sources, or Verilator. `--phase all` (the default) retains the
single-command local workflow; `--workers` also applies there.
