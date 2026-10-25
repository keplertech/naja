<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Performance benchmarks

`SNLNetlistBench.cpp` contains the Google Benchmark suite shared by CMake and
Bazel. Ordinary native measurements remain available:

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build-bench --target snl_bench --parallel 4
./build-bench/bench/snl_bench

bazel run -c opt //bench:snl_bench
```

The ordinary CMake build requires an installed Google Benchmark package.

## CodSpeed CPU simulation

The optional CMake integration fetches the pinned CodSpeed Google Benchmark
compatibility library and its upstream dependencies. Use a separate build directory:

```sh
cmake -S . -B build-codspeed -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_BENCHMARKS=ON -DNAJA_BENCHMARK_CODSPEED=ON \
  -DENABLE_SANITIZERS=OFF -DENABLE_THREAD_SANITIZER=OFF -DCODE_COVERAGE=OFF
cmake --build build-codspeed --target snl_bench --parallel 4
```

Running this binary directly checks benchmark execution; actual simulation
measurements require the CodSpeed runner. The Linux CI workflow
`.github/workflows/codspeed.yml` supplies that runner and uploads results on
pull requests, pushes to `main`, and manual dispatch. It uses tokenless public
repository uploads, so fork pull requests do not need a repository secret.
Activate `keplertech/naja` in CodSpeed and configure performance checks as advisory
during calibration; do not add them to required branch checks yet. Build and
runner failures still fail the workflow.

Simulation measures modeled CPU cost, including cache effects. Its reported
times are not native elapsed times and must not be compared with the existing
`gh-pages` timing history. Verilog loading measurements include userspace
parsing work, but exclude time spent in system calls. The initial CI filter
excludes `BM_CapnPSerialize`: that benchmark measures a filesystem-backed dump
and reload, and its paused setup sections need separate compatibility review.

The existing native benchmark workflow and history remain in place during
calibration. Before enabling regression enforcement:

1. Collect at least five independent runs of the same revision and assess
   variance for each benchmark.
2. Verify that a temporary, unmerged slowdown produces a regression report
   with useful flamegraphs.
3. Verify comparisons against a `main` baseline, including a fork pull request.
4. Choose thresholds from the observed variance and retire the old blocking
   native timing alert in a follow-up change.

Keep compiler and dependency versions, optimization flags, and the CodSpeed
library/action versions stable when comparing revisions. Toolchain or benchmark
changes may require a new baseline. CodSpeed is confined to the optional CMake
benchmark build; ordinary builds and Bazel continue to use Google Benchmark.
The benchmark dependency is excluded from CMake installs.

## Connectivity benchmarks

`BM_DNLConstruction` and `BM_Equipotential` share an independently generated
synthetic hierarchy. Each of 16, 128, or 1024 top-level instances expands to eight
leaf inputs, through either one or four hierarchy levels. All inputs share a
single top-level driver, exercising high fanout and repeated model occurrences.
Arguments are `(groups, depth)`; throughput counts expanded leaf inputs.

Fixture creation, result validation, and universe teardown are outside the
measured loops. DNL measurements construct, process, and destroy a fresh DNL each
iteration, avoiding cached singleton lookups. They limit TBB to one thread in
both native and CodSpeed builds; these results measure serial CPU cost, not
parallel scaling. Equipotential measurements construct and destroy a fresh query
result each iteration. Before measurement, the benchmarks check expanded leaf
and terminal counts or equipotential membership counts and report an error if
the expected work is missing. Both families run in the CodSpeed CI job.

`BM_DNLIndependentNets` uses the same hierarchy with a separate driver for each
top-level instance. Its three sizes distinguish the cost of indexing many
independent connectivity groups from traversing one large high-fanout group.
Validation checks the number of groups, expanded terminals, and driver/reader
counts.

`BM_AssignChain` traces 64, 256, or 1024 canonical DB0 Assign instances using
`TraverseAssigns`, starting from either endpoint. Arguments are `(length,
reverse)` with `reverse=1` starting at the output. Validation requires both top
ports and no remaining assign-terminal occurrences in the result.

## Optimization benchmarks

`BM_ConstantPropagation` runs the truth-table engine over two inverter chains
sharing a constant input, at lengths 63, 256, and 1023. Odd and even lengths
exercise both output values. `BM_DeadLogicElimination` uses two chains of 64,
256, or 1024 inverters driven by a top input; only one chain drives a top output.
Validation checks the propagated output constant or the exact surviving instance
names and count, respectively.

Both mutating benchmarks specify `Iterations(1)`: each invocation constructs a
fresh fixture before the measured loop and optimizes it exactly once. This
prevents native Google Benchmark calibration from repeatedly measuring an
already optimized design. Result checks and teardown follow the loop, outside
measurement; constructing and destroying the pass object and any DNL work done
by the pass are measured. TBB is limited to one thread. For native sampling,
use `--benchmark_repetitions=5`; each repetition creates a fresh fixture.

## Editing and lookup benchmarks

`BM_DesignClone` clones and destroys a connected design containing two inverter
chains of 64, 256, or 1024 stages. The source fixture is built outside measurement,
and an untimed clone must pass `deepCompare` before measurement starts. Model
references are shared, matching `SNLDesign::clone` semantics; this does not
recursively duplicate primitive models. Throughput counts cloned instances.

`BM_InstanceLookupByName` and `BM_InstanceLookupByID` query 128, 1024, or 8192
instances. Arguments are `(count, missing)`, testing successful and unsuccessful
lookups separately. Names and IDs are prepared outside measurement and queried
in a deterministic permutation. Each expected result is checked before the
measured loop; timed work includes only lookups and iteration over prepared keys.
Throughput counts lookups.

`BM_TerminalReconnect` disconnects, reconnects, and restores 128, 1024, or 8192
instance inputs each iteration. Arguments are `(count, shared)`; `shared=0` uses
one destination net per input, while `shared=1` builds a single high-fanout net.
Fixture construction is untimed. An untimed reconnection checks destination
fanout; post-measurement checks require original net membership counts and term
connections to be restored and destination nets to be empty. Throughput counts
the three `setNet` calls per input. All iterations start in the same connected
state, without accumulating objects or repeatedly setting an unchanged net.

The core `snl_bench` binary supplies 60 monitored cases. Validation failures
throw an exception so both benchmark harnesses fail the job rather than upload
incomplete work.

## Frontends and output formats

`snl_format_bench` supplies 39 further cases, shared by CMake and Bazel:

```sh
cmake --build build-bench --target snl_format_bench --parallel 4
./build-bench/bench/snl_format_bench
bazel run //bench:snl_format_bench
```

* `BM_SystemVerilogElaboration` and `BM_VHDLElaboration`: matching synthetic
  circuits with 8, 32, or 128 eight-bit lanes. Arguments are `(lanes, kind)`:
  kind 0 is repeated module/entity hierarchy, 1 is unpacked-array wiring,
  2 is addition and XOR, and 3 is a clocked register. Validation checks interface
  widths, output drivers, hierarchy size, and the presence of arithmetic or
  register primitives where expected. These measure file reading, parsing,
  elaboration, and lowering, with diagnostic report files disabled.
* `BM_LibertyLoading`: `(cells, table_size)` for 16, 128, or 512 synthetic inverter
  cells with four 2-by-2 or 8-by-8 timing tables per cell. Validation checks cell
  count, ports, and timing dependencies. This includes parsing numeric tables;
  it does not imply that Naja retains numeric delay tables in the netlist model.
* `BM_SnapshotEncode` and `BM_SnapshotDecode`: separate full NajaIF writes and
  reads for 64, 512, or 4096 connected buffer instances. Both use the public
  filesystem API, including its manifest checks. Encode validation reloads the
  result; decode uses a snapshot prepared before measurement. Both validate all
  instance names and input/output connections after measurement.
* `BM_VerilogDump`: generates the same three buffer designs into a memory stream,
  including primitive declarations and extracting the resulting string. It
  writes that string to disk and reloads it outside measurement, checking all
  connections through the canonical Assign instances introduced by Verilog
  port-to-wire assignments.

Fixture generation, validation, and temporary-directory cleanup are untimed.
Loading cases use `Iterations(1)` to avoid loading into a populated library or
universe; native repetitions each create a fresh fixture. Encoding and dumping
can repeat against immutable input. Temporary files are unique to each
invocation. File-based simulation measures userspace work, excluding kernel I/O
time; these results do not establish disk throughput or end-to-end walltime.

## Python workflows

`python/test_workflows.py` supplies 12 `pytest-codspeed` cases: leaf traversal,
cross-hierarchy connectivity, statistics, and the instance visitor, each with
16, 128, or 1024 repeated blocks containing eight primitive leaves. Results
must match expected leaf, reader, primitive, or visitor counts. Setup and
validation are outside the measured callable. Statistics uses a fresh cache on
every call; traversal and connectivity consume the generators completely.

Run against the native module built from the same checkout, using its matching
Python interpreter. For example, after configuring `build-bench` with that
interpreter and building its `naja` target:

```sh
python -m pip install -r bench/python/requirements.txt
cmake -DSRC="$PWD/src/najaeda/najaeda" \
  -DDST="$PWD/build-bench/python/najaeda" \
  -DNAJA_SO="$PWD/build-bench/src/nl/python/naja_wrapping/naja.so" \
  -P test/najaeda/setup_test_env.cmake
PYTHONPATH="$PWD/build-bench/python" python -m pytest bench/python/test_workflows.py \
  --codspeed --codspeed-mode=simulation
```

The CI workflow stages this package from its own CMake build; it does not use an
installed najaeda wheel. The pinned Python integration is a benchmark-only
dependency, not a package dependency. Python benchmark fixtures can also be run
against a staged Bazel-built module with its matching Python interpreter.
Local simulation-mode execution validates the benchmarks without uploading
measurements. Omit `--codspeed` to run just the workload and correctness checks.

CodSpeed CI selects all **111 cases**: 60 core C++, 39 format C++, and 12 Python.
The three commands run under one action invocation, producing one combined run
with distinct benchmark identities. No macro runners are required.

See the [CodSpeed C++ integration](https://codspeed.io/docs/benchmarks/cpp) and
[GitHub Actions integration](https://codspeed.io/docs/integrations/ci/github-actions).
