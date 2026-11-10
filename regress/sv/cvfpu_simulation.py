#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Run pinned CVFPU UVM tests against RTL and the Naja FP32 FMA dump."""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.request

import pulp_benchmarks as pulp

CVFPU_REV = "2f787402256b76d00e397d72a00b57120db455cf"
HELPERS_REV = "1dfa4836868fc2fc8e3754ee5c1c43fb42948aa3"
VERILATOR_REV = "08bbef72acdc8f3ec5b27e3275e75475bc974e2b"
PULP_SHA = "4510f42b6065ce024ce86e3014826cd149a1892844f4302fed690743387d4821"
UVM_SHA = "b6f74e64dda5efa08e86dd15c22e921c404dd7d4eb4334ab62afe61fca0f6369"
TESTS = ("fpu_single_op_test", "fpu_random_test", "fpu_op_group_test",
         "fpu_fmt_single_op_test", "fpu_fmt_random_test", "fpu_fmt_op_group_test")

TRACE = """// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module naja_cvfpu_trace #(parameter TAG_WIDTH=1)(
  input clk_i, rst_ni, input [95:0] operands_i, input [2:0] is_boxed_i, rnd_mode_i,
  input [4:0] op_i, input op_mod_i, input [TAG_WIDTH-1:0] tag_i,
  input mask_i, aux_i, in_valid_i, in_ready_o, flush_i,
  input [31:0] result_o, input [4:0] status_o, input extension_bit_o,
  input [TAG_WIDTH-1:0] tag_o, input mask_o, aux_o, out_valid_o, out_ready_i, busy_o
);
  timeunit 1ns; timeprecision 1ps;
  clocking cb @(posedge clk_i);
    default input #1step;
    input rst_ni, operands_i, is_boxed_i, rnd_mode_i, op_i, op_mod_i, tag_i;
    input mask_i, aux_i, in_valid_i, in_ready_o, flush_i;
    input result_o, status_o, extension_bit_o, tag_o, mask_o, aux_o;
    input out_valid_o, out_ready_i, busy_o;
  endclocking
  always @(cb) begin
    $display("NAJA_CVFPU_STATE time=%0t reset=%b iv=%b ir=%b ov=%b or=%b flush=%b busy=%b",
      $time, cb.rst_ni, cb.in_valid_i, cb.in_ready_o, cb.out_valid_o,
      cb.out_ready_i, cb.flush_i, cb.busy_o);
    if (cb.rst_ni && cb.in_valid_i && cb.in_ready_o)
      $display("NAJA_CVFPU_INPUT time=%0t operands=%h boxed=%h rounding=%h op=%h mod=%b tag=%h mask=%b aux=%b",
        $time, cb.operands_i, cb.is_boxed_i, cb.rnd_mode_i, cb.op_i, cb.op_mod_i,
        cb.tag_i, cb.mask_i, cb.aux_i);
    if (cb.rst_ni && cb.out_valid_o && cb.out_ready_i)
      $display("NAJA_CVFPU_OUTPUT time=%0t result=%h status=%h extension=%b tag=%h mask=%b aux=%b",
        $time, cb.result_o, cb.status_o, cb.extension_bit_o, cb.tag_o, cb.mask_o, cb.aux_o);
  end
  final $display("NAJA_CVFPU_COMPLETE");
endmodule
bind cvfpu_fp32_fma_synth naja_cvfpu_trace #(.TAG_WIDTH($bits(tag_i))) naja_trace (.*);
"""


def fetch(url, path, checksum):
    if not path.exists():
        with urllib.request.urlopen(url, timeout=60) as response:
            path.write_bytes(response.read())
    if hashlib.sha256(path.read_bytes()).hexdigest() != checksum:
        raise ValueError(f"Checksum mismatch: {path}")
    return path


def checkout(url, revision, path, sparse=None):
    if not (path / ".git").exists():
        path.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "init", "-q", str(path)], check=True)
        subprocess.run(["git", "-C", str(path), "remote", "add", "origin", url], check=True)
        if sparse:
            subprocess.run(["git", "-C", str(path), "sparse-checkout", "set", sparse], check=True)
        subprocess.run(["git", "-C", str(path), "fetch", "--depth", "1", "--filter=blob:none",
                        "origin", revision], check=True)
        subprocess.run(["git", "-C", str(path), "checkout", "--detach", "FETCH_HEAD"], check=True)
    actual = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()
    if actual != revision:
        raise ValueError(f"Verification source revision mismatch: {path}")
    return path


def export_git(source, destination, paths=()):
    destination.mkdir(parents=True, exist_ok=True)
    archive = destination.parent / (destination.name + ".tar")
    with archive.open("wb") as output:
        subprocess.run(["git", "-C", str(source), "archive", "HEAD", *paths], stdout=output, check=True)
    with tarfile.open(archive) as contents:
        contents.extractall(destination, filter="data")
    archive.unlink()


def prepare_sources(inputs, output):
    inputs.mkdir(parents=True, exist_ok=True)
    recipes = fetch(f"https://codeload.github.com/pulp-platform/eda-benchmarks/tar.gz/{pulp.REVISION}",
                    inputs / "eda-benchmarks.tar.gz", PULP_SHA)
    uvm = fetch("https://www.accellera.org/images/downloads/standards/uvm/uvm-1.1d.tar.gz",
                inputs / "uvm-1.1d.tar.gz", UVM_SHA)
    source = checkout("https://github.com/openhwfoundation/cvfpu-uvm.git", CVFPU_REV,
                      inputs / "cvfpu-uvm")
    helpers = checkout("https://github.com/openhwgroup/core-v-verif.git", HELPERS_REV,
                       source / "modules/core-v-verif", "lib/cv_dv_utils")
    output.mkdir(parents=True, exist_ok=True)
    with tarfile.open(recipes) as archive:
        archive.extractall(output, filter="data")
    with tarfile.open(uvm) as archive:
        archive.extractall(output, filter="data")
    recipe = output / f"eda-benchmarks-{pulp.REVISION}" / "testbenches/cvfpu"
    host = output / "host"
    staged = host / "cvfpu-uvm"
    export_git(source, staged)
    export_git(helpers, staged / "modules/core-v-verif", ("lib/cv_dv_utils",))
    # Apply only the public, pinned benchmark's verification patches.
    subprocess.run(["git", "init", "-q", str(staged)], check=True)
    subprocess.run(["git", "-C", str(staged), "apply",
                    *map(str, sorted((recipe / "patches").glob("*.patch")))], check=True)
    subprocess.run([sys.executable, str(recipe / "generate-shim.py"),
                    str(staged / "ref_model_csim/cpp/include/dpiheader.h"),
                    str(staged / "ref_model_csim/cpp/src/verilator_shim.cpp")], check=True)
    shutil.copytree(output / "uvm-1.1d", host / "uvm", dirs_exist_ok=True)
    if sys.platform == "darwin":
        for name in ("uvm_regex.cc", "uvm_svcmd_dpi.c"):
            source = host / "uvm/src/dpi" / name
            text = source.read_text()
            if text.count("#include <malloc.h>") != 1:
                raise ValueError("Pinned UVM allocation header layout changed")
            source.write_text(text.replace("#include <malloc.h>",
                "// Modified by Naja: standard allocation declarations on macOS.\n#include <stdlib.h>"))
    shutil.copy2(recipe / "uvm-support.cpp", host / "uvm-support.cpp")
    return recipe, host


def simulation_inputs(package, recipe, host, dump, output):
    source = package / "cvfpu/variants/fp32_fma"
    rtl = []
    for line in (source / "filelist.f").read_text().splitlines():
        if not line.strip() or line.startswith(("+define+SYNTHESIS", "+define+TARGET_SYNTHESIS",
                                               "+define+COMMON_CELLS_ASSERTS_OFF")):
            continue
        if line.startswith("+incdir+"):
            line = "+incdir+" + str((source / line[8:]).resolve())
        elif not line.startswith("+"):
            line = str((source / line).resolve())
        rtl.append(line)
    bench = (recipe / "sources.f").read_text().replace("@HOST@", str(host)).splitlines()
    trace = output / "cvfpu_trace.sv"
    trace.write_text(TRACE)
    common = [*bench, "+define+BENCH_FP32_FMA", str(trace)]
    generated = [str(package / "cvfpu/main/src/fpnew_pkg.sv"), str(dump / "netlist.v")]
    if (dump / "naja_primitives.v").exists():
        generated.append(str(dump / "naja_primitives.v"))
    result = {}
    for name, files in (("original", rtl), ("generated", generated)):
        result[name] = output / f"{name}.f"
        result[name].write_text("\n".join([*files, *common]) + "\n")
    return result


def validate_simulation(log):
    if "TEST PASSED" not in log or "NAJA_CVFPU_COMPLETE" not in log or "%Error" in log:
        raise ValueError("Missing successful UVM completion")
    for line in log.splitlines():
        line = line.lstrip()
        if re.match(r"UVM_(ERROR|FATAL)\b", line) and not re.fullmatch(r"UVM_(ERROR|FATAL)\s*:\s*0\s*", line):
            raise ValueError("UVM error or fatal diagnostic")
    coverage = re.findall(r"\[FPU_SB_COVERAGE\].*checked=(\d+) skipped=(\d+)", log)
    if len(coverage) != 1 or int(coverage[0][0]) == 0:
        raise ValueError("Missing nonempty reference-model coverage")
    trace = re.findall(r"^NAJA_CVFPU_(?:STATE|INPUT|OUTPUT) .+$", log, re.M)
    outputs = sum(line.startswith("NAJA_CVFPU_OUTPUT ") for line in trace)
    inputs = sum(line.startswith("NAJA_CVFPU_INPUT ") for line in trace)
    states = sum(line.startswith("NAJA_CVFPU_STATE ") for line in trace)
    if not states or min(inputs, outputs) < int(coverage[0][0]):
        raise ValueError("Incomplete DUT output trace")
    return dict(checked=int(coverage[0][0]), skipped=int(coverage[0][1]), outputs=outputs), trace


def compare_results(first, second):
    if first[0] != second[0] or first[1] != second[1]:
        raise ValueError("RTL/netlist coverage or cycle/transaction traces differ")


def execute(command, cwd, log_path, timeout, env=None):
    command = list(map(str, command))
    start = time.monotonic()
    record = dict(command=command, cwd=str(cwd), log=str(log_path))
    with log_path.open("w") as log:
        try:
            result = pulp.run_command(command, timeout, cwd=cwd, stdout=log,
                                      stderr=subprocess.STDOUT, env=env)
            record.update(returncode=result.returncode,
                          status="passed" if result.returncode == 0 else "failed")
        except subprocess.TimeoutExpired:
            record["status"] = "timeout"
    record["seconds"] = round(time.monotonic() - start, 3)
    return record


def select_cases(tests, seeds, shard_index, shard_count):
    """Partition the ordered matrix without duplicating or dropping cases."""
    return [(test, seed) for index, (test, seed) in enumerate(
        (test, seed) for test in tests for seed in range(1, seeds + 1))
        if index % shard_count == shard_index]


def run_case(test, seed, binaries, out, timeout, env):
    case = dict(test=test, seed=seed, results={}, status="running")
    directory = out / f"{test}-seed{seed}"
    directory.mkdir(parents=True, exist_ok=True)
    traces = []
    try:
        for name, binary in binaries.items():
            log = directory / f"{name}.log"
            result = execute([binary, "+UVM_NO_RELNOTES", "+UVM_VERBOSITY=UVM_LOW",
                f"+UVM_TESTNAME={test}", "+NB_TXNS=10", f"+verilator+seed+{seed}"],
                directory, log, timeout, env)
            case["results"][name] = result
            if result["status"] != "passed":
                raise ValueError(f"{name} failed")
            coverage, trace = validate_simulation(log.read_text())
            result["coverage"] = coverage
            result["trace_sha256"] = hashlib.sha256("\n".join(trace).encode()).hexdigest()
            traces.append((coverage, trace))
        compare_results(*traces)
        case["status"] = "passed"
    except Exception as error:
        case.update(status="failed", error=str(error))
    return case


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, help="Verified prepared PULP RTL directory")
    parser.add_argument("--inputs", type=Path, default=pulp.ROOT / "build/cvfpu-sources")
    parser.add_argument("--output", type=Path, default=pulp.ROOT / "build/cvfpu-simulation")
    parser.add_argument("--najaeda-path", type=Path, default=pulp.ROOT / "build/test/najaeda")
    parser.add_argument("--verilator", default="verilator")
    parser.add_argument("--z3", default="z3")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=1200, help="Per build/run wall-clock limit")
    parser.add_argument("--seeds", type=int, default=50, help="Seeds 1..N for each UVM class")
    parser.add_argument("--test", action="append", choices=TESTS)
    parser.add_argument("--phase", choices=("all", "build", "run"), default="all",
                        help="Build once, then distribute the output directory to run jobs")
    parser.add_argument("--workers", type=int, default=1, help="Concurrent RTL/netlist pairs")
    parser.add_argument("--shard-index", type=int, default=0, help="Zero-based matrix partition")
    parser.add_argument("--shard-count", type=int, default=1)
    args = parser.parse_args()
    if min(args.jobs, args.timeout, args.seeds, args.workers, args.shard_count) < 1 or args.seeds > 50:
        parser.error("jobs/timeout/workers/shard-count must be positive; seeds must be in 1..50")
    if not 0 <= args.shard_index < args.shard_count:
        parser.error("shard-index must be in 0..shard-count-1")
    if args.phase != "run" and args.package is None:
        parser.error("--package is required for build/all")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    build_manifest = out / "build-summary.json"
    if args.phase != "run":
        build_manifest.unlink(missing_ok=True)
    summary = dict(status="running", benchmark_revision=pulp.REVISION,
                   cvfpu_uvm_revision=CVFPU_REV, helpers_revision=HELPERS_REV,
                   tests=list(dict.fromkeys(args.test or TESTS)), seeds=args.seeds,
                   transactions_per_case=10,
                   results={}, cases=[])
    summary.update(workers=args.workers, shard_index=args.shard_index, shard_count=args.shard_count)
    selected = select_cases(summary["tests"], args.seeds, args.shard_index, args.shard_count)
    if not selected:
        parser.error("This shard contains no cases")
    summary["expected_cases"] = len(selected)
    def save():
        temporary = out / "summary.json.tmp"
        temporary.write_text(json.dumps(summary, indent=2) + "\n")
        temporary.replace(out / "summary.json")
    save()
    try:
        if args.phase == "run":
            previous = json.loads(build_manifest.read_text())
            expected = dict(status="built", benchmark_revision=pulp.REVISION,
                            cvfpu_uvm_revision=CVFPU_REV, helpers_revision=HELPERS_REV,
                            expected_verilator_revision=VERILATOR_REV)
            if any(previous.get(key) != value for key, value in expected.items()):
                raise ValueError("Missing successful build or verification revision mismatch")
            for key in ("naja_producer", "verilator", "solver", "expected_verilator_revision", "results"):
                summary[key] = previous[key]
        z3 = shutil.which(args.z3)
        if not z3:
            raise ValueError("Z3 executable is required")
        z3 = str(Path(z3).resolve())
        solver = subprocess.check_output([z3, "--version"], text=True).strip()
        if not re.search(r"\b4\.13\.3\b", solver):
            raise ValueError("Use the upstream-pinned Z3 4.13.3")
        env = dict(os.environ, VERILATOR_SOLVER=shlex.quote(z3) + " --in")
        if args.phase != "run":
            verilator = shutil.which(args.verilator)
            if not verilator:
                raise ValueError("Verilator and Z3 executables are required")
            verilator = str(Path(verilator).resolve())
            version = subprocess.check_output([verilator, "--version"], text=True).strip()
            summary.update(verilator=version, solver=solver, expected_verilator_revision=VERILATOR_REV)
            if "5.053" not in version or VERILATOR_REV[:7] not in version or "4.13.3" not in solver:
                raise ValueError("Use the upstream-pinned Verilator 5.053 and Z3 4.13.3")
            package = pulp.validate_package(args.package.resolve())
            producer = subprocess.check_output([sys.executable, "-c",
                "import json; from najaeda import naja; print(json.dumps(dict("
                "version=naja.getVersion(), git_hash=naja.getGitHash())))"], text=True,
                env=dict(env, PYTHONPATH=str(args.najaeda_path.resolve())))
            summary["naja_producer"] = json.loads(producer)
            recipe, host = prepare_sources(args.inputs.resolve(), out / "sources")
            dump = out / "dump"
            loaded = pulp.run_target(package, "cvfpu.fp32_fma", "cvfpu_fp32_fma_synth", dump,
                                     args.najaeda_path.resolve(), args.timeout)
            summary["results"]["dump"] = loaded
            save()
            if loaded["status"] != "passed":
                raise ValueError("CVFPU elaboration failed; see dump diagnostics")
            lists = simulation_inputs(package, recipe, host, dump, out)
            model = host / "cvfpu-uvm/ref_model_csim/cpp"
            flags = subprocess.check_output(["pkg-config", "--cflags", "mpfr", "gmp"], text=True).strip()
            libs = subprocess.check_output(["pkg-config", "--libs", "mpfr", "gmp"], text=True).strip()
            cpp = [host / "uvm-support.cpp", *[model / "src" / name for name in
                   ("dpi_wrapper.cpp", "operations.cpp", "memory.cpp", "verilator_shim.cpp")]]
            includes = f"-I{host / 'uvm/src/dpi'} -I{model / 'include'} {flags}"
            for name, flist in lists.items():
                obj = out / f"{name}-obj"
                (obj / "Vtop").unlink(missing_ok=True)
                command = [verilator, "--binary", "--timing", "--assert", "--vpi",
                           "--timescale", "1ns/1ps", "--top-module", "top", "-Wno-fatal",
                           "-j", args.jobs, "--Mdir", obj, "-CFLAGS", includes,
                           "-LDFLAGS", libs, "-f", flist, *cpp]
                result = execute(command, out, out / f"{name}-build.log", args.timeout, env)
                summary["results"][f"{name}_build"] = result
                save()
                if result["status"] != "passed":
                    raise ValueError(f"{name} simulation build failed")
            built = dict(summary, status="built")
            build_manifest.write_text(json.dumps(built, indent=2) + "\n")
            if args.phase == "build":
                summary["status"] = "built"
                save()
                return 0
        binaries = {name: out / f"{name}-obj/Vtop" for name in ("original", "generated")}
        if not all(binary.is_file() for binary in binaries.values()):
            raise ValueError("Missing built simulators in output directory")
        started = time.monotonic()
        completed = {}
        with ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = {pool.submit(run_case, test, seed, binaries, out, args.timeout, env): (test, seed)
                       for test, seed in selected}
            for future in as_completed(futures):
                key = futures[future]
                completed[key] = future.result()
                summary["cases"] = [completed[key] for key in selected if key in completed]
                save()
                print(f"{len(completed)}/{len(selected)}: {key[0]} seed {key[1]} "
                      f"{completed[key]['status']}", flush=True)
        summary["simulation_seconds"] = round(time.monotonic() - started, 3)
        failures = [case for case in summary["cases"] if case["status"] != "passed"]
        if failures:
            raise ValueError(f"{len(failures)} RTL/netlist pairs failed; see per-case diagnostics")
        summary["totals"] = dict(cases=len(summary["cases"]),
            checked=sum(case["results"]["original"]["coverage"]["checked"] for case in summary["cases"]),
            skipped=sum(case["results"]["original"]["coverage"]["skipped"] for case in summary["cases"]))
        summary["status"] = "passed"
        save()
        return 0
    except Exception as error:
        summary.update(status="failed", error=str(error))
        save()
        raise


if __name__ == "__main__":
    sys.exit(main())
