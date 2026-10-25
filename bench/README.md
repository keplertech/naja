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

See the [CodSpeed C++ integration](https://codspeed.io/docs/benchmarks/cpp) and
[GitHub Actions integration](https://codspeed.io/docs/integrations/ci/github-actions).
