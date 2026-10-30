#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Prepare a pinned PULP Git revision and load/dump its RTL in isolated processes."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tarfile
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
REPOSITORY = "https://github.com/pulp-platform/eda-benchmarks.git"
# Update this single pin when advancing the local runner and nightly inputs.
REVISION = "b4bd97823aa93737dda0bf1f3b2bd5c6dcb47ecc"
PACKAGE = f"pulp-benchmarks-{REVISION}"
MANIFEST = "naja-pulp-source.json"
DEFAULT_CACHE = ROOT / "build/pulp-benchmark-sources"
# Keep this small enough for pull requests; the complete survey is opt-in.
SMOKE = ("cv32e40p.default", "cvfpu.fp32_fma", "ibex.minimal", "spi_host.default",
         "nvdla.top", "snitch_cluster.minimal")
# Case-specific Slang compatibility settings for the pinned source suite.
SLANG_ARGS = {
    "nvdla.top": ["--timescale=1ns/1ps"],
    "cheshire.default": ["--allow-use-before-declare"],
    "redmule.default": ["--relax-enum-conversions"],
}


def package_hashes(package):
    hashes = {}
    for path in sorted(package.rglob("*")):
        if path.is_symlink():
            raise ValueError(f"Prepared package must be self-contained: {path}")
        if path.is_file() and path != package / MANIFEST:
            with path.open("rb") as source:
                hashes[path.relative_to(package).as_posix()] = hashlib.file_digest(source, "sha256").hexdigest()
    return hashes


def write_manifest(package):
    manifest = dict(repository=REPOSITORY, revision=REVISION, simulation=False,
                    files=package_hashes(package))
    manifest["SPDX-FileCopyrightText"] = "2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>"
    manifest["SPDX-License-Identifier"] = "Apache-2.0"
    (package / MANIFEST).write_text(json.dumps(manifest, indent=2) + "\n")


def validate_package(package):
    if not (package / MANIFEST).is_file():
        raise ValueError(f"Missing pinned-source manifest for {REVISION}: {package}")
    manifest = json.loads((package / MANIFEST).read_text())
    if (manifest.get("repository") != REPOSITORY or manifest.get("revision") != REVISION
            or manifest.get("simulation") is not False):
        raise ValueError(f"Prepared package does not match pinned RTL revision {REVISION}")
    if manifest.get("files") != package_hashes(package):
        raise ValueError(f"Prepared package SHA-256 manifest mismatch: {package}")
    targets(package)
    return package


def prepare(cache=DEFAULT_CACHE):
    cache = cache.resolve()
    package = cache / PACKAGE
    if package.exists():
        return validate_package(package)
    if sys.platform != "linux":
        raise RuntimeError("Preparing PULP sources requires Linux and upstream build prerequisites; "
                           "use --archive or --package with a package prepared from the pinned revision")
    cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pulp-source-", dir=cache) as temporary:
        checkout = Path(temporary) / "checkout"
        subprocess.run(["git", "init", str(checkout)], check=True)
        subprocess.run(["git", "-C", str(checkout), "fetch", "--depth", "1", REPOSITORY, REVISION], check=True)
        subprocess.run(["git", "-C", str(checkout), "checkout", "--detach", "FETCH_HEAD"], check=True)
        actual = subprocess.check_output(["git", "-C", str(checkout), "rev-parse", "HEAD"], text=True).strip()
        if actual != REVISION:
            raise ValueError(f"Benchmark checkout mismatch: expected {REVISION}, got {actual}")
        subprocess.run(["bash", "setup.sh"], cwd=checkout, check=True,
                       env=dict(os.environ, SIMULATION="0", KEEP_BUILD_TOOLS="0",
                                PYTHON=sys.executable, JOBS=str(os.cpu_count() or 1)))
        prepared = checkout / "benchmarks"
        write_manifest(prepared)
        validate_package(prepared)
        try:
            prepared.rename(package)
        except OSError:
            if not package.exists():
                raise
            validate_package(package)
    return package


def unpack(archive, destination):
    with tarfile.open(archive) as source:
        source.extractall(destination, filter="data")
    return validate_package(destination / PACKAGE)


def targets(package):
    pairs = re.findall(r"^([\w]+\.[\w]+)\.TOP := (\w+)\s*$",
                       (package / "targets.mk").read_text(), re.MULTILINE)
    if not pairs or len(dict(pairs)) != len(pairs):
        raise ValueError("Missing or duplicate benchmark tops in targets.mk")
    return dict(pairs)


def validate_diagnostics(diagnostics):
    if ("summary.status=success" not in diagnostics
            or 'severity="error"' in diagnostics
            or 'severity="fatal"' in diagnostics
            or 'stage="naja_unsupported_' in diagnostics):
        raise RuntimeError("Incomplete or unsupported lowering; see diagnostics.log")


def worker(top_name, artifacts, dump_netlist, frontend_options=None):
    from najaeda import netlist

    start = time.monotonic()
    top = netlist.load_system_verilog([], config=netlist.SystemVerilogConfig(
        top=top_name, flist="filelist.f",
        diagnostics_report_path=str(artifacts / "diagnostics.log"),
        blackbox_unknown_modules=False,
        frontend_options=frontend_options,
    ))
    loaded = time.monotonic()
    if top.get_name() != top_name or not top.count_terms():
        raise RuntimeError("Missing or unexpected top-level interface")
    validate_diagnostics((artifacts / "diagnostics.log").read_text())
    stats = dict(top=top.get_name(), top_terms=top.count_terms(),
                 top_nets=top.count_nets(), top_instances=top.count_child_instances(),
                 top_bit_terms=top.count_bit_terms(), top_bit_nets=top.count_bit_nets(),
                 load_seconds=loaded - start)
    efficiency = top.get_lowering_efficiency_report()
    (artifacts / "lowering-efficiency.json").write_text(json.dumps(efficiency, indent=2) + "\n")
    stats["expanded_mux_bits"] = efficiency["expanded_mux_bits"]
    stats["stats_seconds"] = time.monotonic() - loaded
    (artifacts / "design-stats.json").write_text(json.dumps(stats, indent=2) + "\n")
    if dump_netlist:
        dumping = time.monotonic()
        top.dump_gate_level_verilog(str(artifacts / "netlist.v"), config=netlist.GateLevelVerilogDumpConfig(
            verilatorSplitPackedSignals=True))
        stats["dump_seconds"] = time.monotonic() - dumping
    (artifacts / "design-stats.json").write_text(json.dumps(stats, indent=2) + "\n")
    netlist.reset()


def run_command(command, timeout, **kwargs):
    # Verilator's launcher spawns a child; bound the entire invocation.
    with subprocess.Popen(command, start_new_session=True, **kwargs) as process:
        try:
            return subprocess.CompletedProcess(command, process.wait(timeout=timeout))
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
            raise


def run_target(package, name, top, artifacts, najaeda_path, timeout, lint_runner=None,
               dump_netlist=True):
    artifacts.mkdir(parents=True, exist_ok=True)
    # Never let stale success products from an earlier run satisfy this run.
    for filename in ("design-stats.json", "netlist.v", "najaeda_primitives.v", "naja_primitives.v",
                     "diagnostics.log", "lint.log", "summary.json", "lowering-efficiency.json"):
        (artifacts / filename).unlink(missing_ok=True)
    design, variant = name.split(".")
    source_dir = package / design / "variants" / variant
    source_list = (source_dir / "filelist.f").read_text()
    (artifacts / "filelist.f").write_text(source_list)
    command = [sys.executable, str(Path(__file__).resolve()), "--worker", top,
               "--output", str(artifacts)]
    slang_args = SLANG_ARGS.get(name, [])
    for arg in slang_args:
        command.append("--slang-arg=" + arg)
    if not dump_netlist:
        command.append("--elaboration-only")
    env = dict(os.environ, PYTHONPATH=str(najaeda_path))
    record = dict(target=name, top=top, command=command, cwd=str(source_dir),
                  frontend_options={"slang": slang_args},
                  source_count=sum(line.strip().endswith((".v", ".sv"))
                                   for line in source_list.splitlines()))
    start = time.monotonic()
    with (artifacts / "load-dump.log").open("w") as log:
        try:
            result = run_command(command, cwd=source_dir, env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=timeout)
            record["returncode"] = result.returncode
            record["status"] = "passed" if result.returncode == 0 else "failed"
        except subprocess.TimeoutExpired:
            record["status"] = "timeout"
    record["seconds"] = round(time.monotonic() - start, 3)
    stats = artifacts / "design-stats.json"
    if stats.is_file():
        record["design_stats"] = json.loads(stats.read_text())
    if record["status"] == "passed":
        dump = artifacts / "netlist.v"
        if not stats.is_file() or (dump_netlist and (not dump.is_file() or dump.stat().st_size == 0)):
            record["status"] = "failed"
            record["error"] = "Missing load/dump products"
        else:
            record["design_stats"] = json.loads(stats.read_text())
            if dump_netlist:
                record["dump_bytes"] = dump.stat().st_size
    if record["status"] == "passed" and lint_runner and dump_netlist:
        primitives = artifacts / "naja_primitives.v"
        inputs = [artifacts / "netlist.v"]
        if primitives.is_file():
            inputs.append(primitives)
        flags = ["--lint-only", "--verilate-jobs", "0", "--sv", "--top-module", top,
                 "-Wno-ASCRANGE"]
        if lint_runner == "docker":
            cidfile = artifacts / "lint-container.cid"
            cidfile.unlink(missing_ok=True)
            command = ["docker", "run", "--rm", "--cidfile", str(cidfile), "-v", f"{artifacts}:/work:ro",
                       "--entrypoint", "verilator",
                       "verilator/verilator:v5.046", *flags, *[f"/work/{path.name}" for path in inputs]]
        else:
            command = ["verilator", *flags, *map(str, inputs)]
        record["lint_command"] = command
        start = time.monotonic()
        with (artifacts / "lint.log").open("w") as log:
            try:
                result = run_command(command, stdout=log, stderr=subprocess.STDOUT,
                                     timeout=timeout)
                record["lint_returncode"] = result.returncode
                if result.returncode:
                    record["status"] = "lint_failed"
            except subprocess.TimeoutExpired:
                record["status"] = "lint_timeout"
            except OSError as error:
                record["status"] = "lint_failed"
                record["error"] = str(error)
            finally:
                if lint_runner == "docker" and cidfile.exists():
                    try:
                        subprocess.run(["docker", "rm", "-f", cidfile.read_text().strip()],
                                       stdout=log, stderr=subprocess.STDOUT, timeout=30)
                    except (OSError, subprocess.TimeoutExpired) as error:
                        record["status"] = "lint_failed"
                        record["error"] = f"Container cleanup failed: {error}"
        record["lint_seconds"] = round(time.monotonic() - start, 3)
    (artifacts / "summary.json").write_text(json.dumps(record, indent=2) + "\n")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    inputs = parser.add_mutually_exclusive_group()
    inputs.add_argument("--archive", type=Path, help="Use a prepared archive (pinned source manifest verified)")
    inputs.add_argument("--package", type=Path, help="Use a prepared directory (pinned source manifest verified)")
    parser.add_argument("--cache", type=Path, default=DEFAULT_CACHE,
                        help="Cache prepared packages by pinned Git revision")
    parser.add_argument("--prepare-only", action="store_true", help="Prepare/validate inputs without loading Naja")
    parser.add_argument("--output", type=Path, default=ROOT / "build/pulp-benchmarks")
    parser.add_argument("--najaeda-path", type=Path, default=ROOT / "build/test/najaeda")
    parser.add_argument("--case", action="append", help="Benchmark target, repeatable; defaults to smoke selection")
    parser.add_argument("--all", action="store_true", help="Survey every benchmark target; failures remain failures")
    parser.add_argument("--timeout", type=int, default=600, help="Wall-clock limit per load/dump or lint stage in seconds (default: 600)")
    parser.add_argument("--lint-runner", choices=("local", "docker"),
                        help="Also lint each complete dump with Naja primitive models")
    parser.add_argument("--elaboration-only", action="store_true",
                        help="Load and validate the design without structural Verilog dumping")
    parser.add_argument("--worker", help=argparse.SUPPRESS)
    parser.add_argument("--slang-arg", action="append", default=[], help=argparse.SUPPRESS)
    args = parser.parse_args()
    args.output = args.output.resolve()
    if args.worker:
        worker(args.worker, args.output, not args.elaboration_only, {"slang": args.slang_arg})
        return 0
    if args.timeout <= 0 or (args.all and args.case) or (args.elaboration_only and args.lint_runner):
        parser.error("--timeout must be positive; --all and --case are mutually exclusive")
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pulp-rtl-", dir=args.output) as tmp:
        package = (unpack(args.archive, Path(tmp)) if args.archive else
                   validate_package(args.package.resolve()) if args.package else prepare(args.cache))
        if args.prepare_only:
            print(package)
            return 0
        available = targets(package)
        selected = list(available) if args.all else list(dict.fromkeys(args.case or SMOKE))
        unknown = set(selected) - available.keys()
        if unknown:
            parser.error(f"Unknown cases: {sorted(unknown)}; available: {list(available)}")
        summary = dict(release=REVISION, revision=REVISION, url=REPOSITORY,
                       manifest_sha256=hashlib.sha256((package / MANIFEST).read_bytes()).hexdigest(),
                       selected=selected, inventory=list(available),
                       mode=("elaboration" if args.elaboration_only else
                             "load + dump + lint" if args.lint_runner else "load + dump"),
                       results=[])
        (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
        for name in selected:
            result = run_target(package, name, available[name], args.output / name,
                                args.najaeda_path.resolve(), args.timeout, args.lint_runner,
                                not args.elaboration_only)
            summary["results"].append(result)
            (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
            print(f"{name}: {result['status']} ({result['seconds']:.2f}s)", flush=True)
    return int(any(result["status"] != "passed" for result in summary["results"]))


if __name__ == "__main__":
    sys.exit(main())
