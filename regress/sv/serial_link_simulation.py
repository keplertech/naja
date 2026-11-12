#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Run the pinned upstream one-channel Serial Link matrix on RTL then Naja."""

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

import pulp_benchmarks as pulp

SERIAL_REV = "164d327b6ebe72c8d72c93e9d7c7169fa41d9b16"
VERILATOR_REV = "ea338be98e1e838d3518809ce8899f85a009963c"
TOP = "naja_serial_link_dut"
CASES = list(itertools.product((0, 100), repeat=4))


def checked_revision(path, revision):
    actual = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()
    if actual != revision:
        raise ValueError(f"Revision mismatch for {path}: {actual}")


def execute(command, cwd, log_path, timeout, env=None):
    start = time.monotonic()
    command = list(map(str, command))
    with log_path.open("w") as log:
        result = pulp.run_command(command, timeout, cwd=cwd, env=env,
                                  stdout=log, stderr=subprocess.STDOUT)
    record = dict(command=command, seconds=round(time.monotonic() - start, 3),
                  returncode=result.returncode, log=str(log_path), status="passed")
    if result.returncode:
        raise RuntimeError(f"Command failed; see {log_path}")
    return record


def monitor():
    blocks = ["\n// Added by Naja: observe the same pre-NBA handshakes as axi_chan_compare.\n",
              'initial begin #1s; $fatal(1, "NAJA_SLINK watchdog expired"); end\n']
    for endpoint in (1, 2):
        blocks.append(f"always @(posedge clk_{endpoint}) begin\n")
        for port in ("in", "out"):
            req, rsp = f"axi_{port}_req_{endpoint}", f"axi_{port}_rsp_{endpoint}"
            for channel in ("aw", "w", "b", "ar", "r"):
                owner = rsp if channel in ("b", "r") else req
                peer = req if channel in ("b", "r") else rsp
                last = f"{owner}.{channel}.last" if channel in ("w", "r") else "1'b1"
                blocks.append(f'  if ({owner}.{channel}_valid && {peer}.{channel}_ready)\n'
                    f'    $display("NAJA_SLINK_TX ep={endpoint} port={port} ch={channel} time=%0t data=%h last=%b", '
                    f'$time, {owner}.{channel}, {last});\n')
        blocks.append("end\n")
    blocks.append('final $display("NAJA_SLINK_COMPLETE");\n')
    return "".join(blocks)


def constant_delay_fast_path(source):
    pattern = ("      rand_success = std::randomize(cycles) with {\n"
               "        cycles >= min;\n        cycles <= max;\n      };")
    if source.count(pattern) != 4:
        raise ValueError("Pinned AXI rand_wait layout changed")
    replacement = ("      // Modified by Naja: equal bounds have exactly one legal delay.\n"
                   "      if (min == max) begin\n"
                   "        cycles = min;\n        rand_success = 1;\n"
                   "      end else begin\n" + pattern + "\n      end")
    return source.replace(pattern, replacement)


def prepare(recipes, out, fast_fixed_delays=False):
    checked_revision(recipes, pulp.REVISION)
    subprocess.run(["git", "-C", str(recipes), "diff", "--quiet", "HEAD", "--",
                    "bender/serial_link", "patches/serial_link"], check=True)
    upstream = recipes / "deps/serial_link"
    checked_revision(upstream, SERIAL_REV)
    host = out / "host"
    host.mkdir(exist_ok=True)
    main = host / "main"
    if main.exists():
        raise ValueError("Use a fresh output directory; staged sources already exist")
    main.mkdir()
    archive = out / "serial-link.tar"
    with archive.open("wb") as stream:
        subprocess.run(["git", "-C", str(upstream), "archive", "HEAD"], stdout=stream, check=True)
    import tarfile
    with tarfile.open(archive) as stream:
        stream.extractall(main, filter="data")
    archive.unlink()
    recipe = recipes / "bender/serial_link"
    for filename in ("Bender.yml", "Bender.lock"):
        shutil.copy2(recipe / filename, host / filename)
    patches = [*sorted((recipes / "patches/serial_link/main").glob("*.patch")),
               *sorted((recipes / "patches/serial_link/simulation/main").glob("*.patch"))]
    subprocess.run(["git", "-C", str(main), "apply", *map(str, patches)], check=True)
    lock = (host / "Bender.lock").read_bytes()
    sources = subprocess.check_output(["bender", "script", "verilator", "-t", "simulation",
                                       "-t", "slink_test"], cwd=host, text=True)
    if (host / "Bender.lock").read_bytes() != lock:
        raise ValueError("Bender changed the pinned dependency lock")
    # Verify actual dependency commits rather than trusting the lock alone.
    for name, revision in re.findall(r"^  (\w+):\n    revision: ([0-9a-f]{40})$", lock.decode(), re.M):
        dependency = Path(subprocess.check_output(["bender", "path", name], cwd=host, text=True).strip())
        checked_revision(dependency, revision)
    if fast_fixed_delays:
        axi = Path(subprocess.check_output(["bender", "path", "axi"], cwd=host, text=True).strip())
        path = axi / "src/axi_test.sv"
        original = path.read_text()
        adapted = constant_delay_fast_path(original)
        path.write_text(adapted)
        import difflib
        (out / "axi-fixed-delay.patch").write_text("".join(difflib.unified_diff(
            original.splitlines(keepends=True), adapted.splitlines(keepends=True),
            fromfile="a/src/axi_test.sv", tofile="b/src/axi_test.sv")))
    tb_path = main / "test/tb_axi_slink.sv"
    tb = tb_path.read_text()
    tb_path.write_text(tb.replace("endmodule : tb_axi_slink", monitor() + "endmodule : tb_axi_slink"))
    original = out / "original.f"
    original.write_text(sources)
    # Match the upstream testbench's 3-bit IDs and isolation shell, rather than
    # the synthesis benchmark wrapper's different 4-bit interface.
    types = tb[tb.index("  localparam int unsigned AxiIdWidth"):tb.index("  typedef logic [NumLanes")]
    aliases = "typedef axi_resp_t axi_rsp_t; typedef apb_resp_t apb_rsp_t;\n"
    aliases += "\n".join(f"typedef axi_{ch}_chan_t {ch}_chan_t;" for ch in ("aw", "w", "b", "ar", "r"))
    aliases += "\ntypedef cfg_addr_t apb_addr_t; typedef cfg_data_t apb_data_t; typedef cfg_strb_t apb_strb_t;\n"
    package = ('`include "axi/typedef.svh"\n`include "apb/typedef.svh"\n'
               "package naja_serial_link_types;\n" + types + aliases + "endpackage\n")
    shell = (main / "src/slink_isolate.sv").read_text()
    end = shell.index(") (", shell.index("module slink_isolate"))
    ports_body = shell[end + 2:]
    wrapper = out / "dut.sv"
    wrapper.write_text(package + f"module {TOP}\n  import naja_serial_link_types::*;\n"
                       "  import slink_reg_pkg::*;\n" + ports_body)
    # All synthesizable dependencies come from this same simulation checkout.
    synth = subprocess.check_output(["bender", "script", "verilator", "-t", "synthesis"],
                                    cwd=host, text=True)
    (out / "elaborate.f").write_text(synth + "\n" + str(wrapper) + "\n")
    generated_tb = out / "generated-tb.sv"
    pattern = r"slink_isolate\s*#\(.*?\)\s*(i_serial_link_[12])\s*\("
    generated, count = re.subn(pattern, TOP + r" \1 (", tb_path.read_text(), flags=re.S)
    if count != 2:
        raise ValueError("Upstream DUT instantiation layout changed")
    generated_tb.write_text(generated)
    return sources, generated_tb


def transaction_records(log):
    records = re.findall(r"^NAJA_SLINK_TX ep=(\d) port=(in|out) ch=(aw|w|b|ar|r) time=(\d+) data=([0-9a-f]+) last=([01])$", log, re.M)
    if len(records) != len(re.findall(r"^NAJA_SLINK_TX\b", log, re.M)):
        raise ValueError("Malformed or unknown-valued AXI trace record")
    return records


def transaction_streams(records):
    streams = {}
    for ep, port, channel, timestamp, data, last in records:
        streams.setdefault(f"{ep}/{port}/{channel}", []).append((timestamp, data))
    return streams


def validate(log, case):
    if (log.count("TEST PASSED") != 1 or "NAJA_SLINK_COMPLETE" not in log
            or "[SYS] Links are ready" not in log
            or re.search(r"%Error|Assertion failed|mismatch|queue is empty", log, re.I)):
        raise ValueError("Upstream checks or completion failed")
    records = transaction_records(log)
    counts = Counter((ep, port, ch) for ep, port, ch, _, _, last in records if last == "1")
    beats = Counter((ep, port, ch) for ep, port, ch, _, _, _ in records)
    expected = Counter()
    # Preserve the pinned TB: it calls run(NumWrites, NumReads), whereas its
    # pinned axi_rand_master declares run(n_reads, n_writes).
    writes1, reads1, writes2, reads2 = case
    for ep, port, reads, writes in ((1, "in", reads1, writes1), (2, "out", reads1, writes1),
                                    (2, "in", reads2, writes2), (1, "out", reads2, writes2)):
        for channel in ("aw", "w", "b", "ar", "r"):
            if amount := (reads if channel in ("ar", "r") else writes):
                expected[str(ep), port, channel] = amount
    if counts != expected:
        raise ValueError(f"Accepted AXI counts differ: {counts} != {expected}")
    # Global event ordering across unrelated clocks is unspecified. Preserve
    # each interface/channel stream, including every exact handshake timestamp.
    streams = transaction_streams(records)
    return dict(counts={"/".join(key): value for key, value in sorted(counts.items())},
                beats={"/".join(key): value for key, value in sorted(beats.items())},
                accepted=len(records), trace_sha256=hashlib.sha256(
                    json.dumps(streams, sort_keys=True).encode()).hexdigest()), streams


def compare_traces(original, generated):
    if original.keys() != generated.keys():
        raise ValueError("Accepted AXI channel coverage differs")
    for channel, expected in original.items():
        actual = generated[channel]
        for index, pair in enumerate(itertools.zip_longest(expected, actual)):
            if pair[0] != pair[1]:
                raise ValueError(f"AXI {channel} transaction {index}: "
                                 f"RTL {pair[0]} != netlist {pair[1]} (time, payload)")


def original_identity(out):
    files = [out / "original.f", out / "original-obj/Vtb_axi_slink",
             out / "host/Bender.yml", out / "host/Bender.lock"]
    for root in (out / "host/main", out / "host/.bender/git/checkouts"):
        files.extend(path for path in root.rglob("*")
                     if path.is_file() and ".git" not in path.parts)
    return {str(path.relative_to(out)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(files)}


def run_case(case, flavor, out, timeout, env):
    name = "r%d_w%d_r%d_w%d" % case
    directory = out / name
    directory.mkdir(exist_ok=True)
    command = [out / (flavor + "-obj/Vtb_axi_slink"), "+verilator+seed+1"]
    command += [f"+NUM_{kind}_{ep}={value}" for (kind, ep), value in zip(
        (("READS", 1), ("WRITES", 1), ("READS", 2), ("WRITES", 2)), case)]
    log = directory / (flavor + ".log")
    start = time.monotonic()
    try:
        record = execute(command, directory, log, timeout, env)
        coverage, trace = validate(log.read_text(), case)
        record.update(coverage)
        return name, record, trace
    except Exception as error:
        record = dict(status="failed", error=str(error), log=str(log),
                      seconds=round(time.monotonic() - start, 3), command=list(map(str, command)))
        text = log.read_text() if log.exists() else ""
        if "NAJA_SLINK watchdog expired" in text:
            record["error"] = "Simulation watchdog expired"
        try:
            transactions = transaction_records(text)
            record["partial_accepted"] = len(transactions)
            return name, record, transaction_streams(transactions)
        except ValueError as trace_error:
            record["trace_error"] = str(trace_error)
            return name, record, None


def worker(out):
    from najaeda import naja, netlist
    producer = dict(version=naja.getVersion(), git_hash=naja.getGitHash())
    top = netlist.load_system_verilog([], config=netlist.SystemVerilogConfig(
        top=TOP, flist=str(out / "elaborate.f"), blackbox_unknown_modules=False,
        diagnostics_report_path=str(out / "diagnostics.log")))
    pulp.validate_diagnostics((out / "diagnostics.log").read_text())
    # The testbench still compiles upstream helper modules. Namespace the SNL
    # models through the API so their concrete ports cannot shadow those RTL
    # parameterized modules; instance model references follow the rename.
    def namespace(library):
        for design in library.getSNLDesigns():
            if design.getName() != TOP:
                design.setName("naja_slink_" + design.getName())
        for child in library.getLibraries():
            namespace(child)
    for library in naja.NLUniverse.get().getTopDesign().getDB().getLibraries():
        namespace(library)
    # Keep NLDB0 memories and their behavioral models; no memory-to-flop pass.
    top.dump_gate_level_verilog(str(out / "netlist.v"),
        config=netlist.GateLevelVerilogDumpConfig(verilatorSplitPackedSignals=True))
    primitives = (out / "naja_primitives.v").read_text()
    text = (out / "netlist.v").read_text()
    memories = len(re.findall(r"\bnaja_mem\s*#\(", text))
    if memories and "reg [WIDTH-1:0] mem [0:DEPTH-1]" not in primitives:
        raise ValueError("Missing behavioral memory model")
    (out / "producer.json").write_text(json.dumps(dict(producer=producer, memories=memories), indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--recipes", type=Path, help="Pinned benchmark Git checkout with deps/serial_link initialized")
    parser.add_argument("--output", type=Path, required=True, help="Fresh output directory")
    parser.add_argument("--najaeda-path", type=Path)
    parser.add_argument("--verilator", default="verilator")
    parser.add_argument("--z3", default="z3")
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--timeout", type=int, default=3600)
    parser.add_argument("--rtl-only", action="store_true")
    parser.add_argument("--constant-delay-fast-path", action="store_true",
                        help="Skip SMT for equal AXI delay bounds; preserves delays but changes RNG advancement")
    parser.add_argument("--reuse-rtl", action="store_true",
                        help="Revalidate completed RTL logs and unchanged source/binary hashes before netlist work")
    parser.add_argument("--worker", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    out = args.output.resolve()
    if args.worker:
        worker(out)
        return 0
    if not args.recipes or (not args.rtl_only and not args.najaeda_path):
        parser.error("--recipes and (unless --rtl-only) --najaeda-path are required")
    if min(args.jobs, args.workers, args.timeout) < 1:
        parser.error("jobs, workers and timeout must be positive")
    solver = shutil.which(args.z3)
    if not solver:
        parser.error("Z3 executable not found")
    out.mkdir(parents=True, exist_ok=True)
    previous = json.loads((out / "summary.json").read_text()) if args.reuse_rtl else None
    if previous and previous["status"] == "running":
        parser.error("Wait for the prior run to finish before reusing RTL")
    if not args.reuse_rtl and (out / "host/main").exists():
        parser.error("Use a fresh output directory or --reuse-rtl")
    start = time.monotonic()
    summary = dict(status="running", benchmark_revision=pulp.REVISION, serial_link_revision=SERIAL_REV,
                   seed=1, constant_delay_fast_path=args.constant_delay_fast_path, results={}, cases={})
    def save():
        summary["seconds"] = round(time.monotonic() - start, 3)
        (out / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    save()
    env = dict(os.environ, VERILATOR_SOLVER=str(Path(solver).resolve()) + " --in")
    try:
        summary["verilator"] = subprocess.check_output([args.verilator, "--version"], text=True).strip()
        summary["z3"] = subprocess.check_output([args.z3, "--version"], text=True).strip()
        if VERILATOR_REV[:7] not in summary["verilator"]:
            raise ValueError("Use the benchmark-pinned Verilator ea338be")
        if args.reuse_rtl:
            for key in ("benchmark_revision", "serial_link_revision", "seed", "verilator", "z3", "constant_delay_fast_path"):
                if previous.get(key, False) != summary[key]:
                    raise ValueError(f"RTL reuse identity mismatch: {key}")
            checked_revision(args.recipes.resolve(), pulp.REVISION)
            checked_revision(args.recipes.resolve() / "deps/serial_link", SERIAL_REV)
            if original_identity(out) != json.loads((out / "original-identity.json").read_text()):
                raise ValueError("RTL reuse source or binary hashes differ")
            sources = (out / "original.f").read_text()
            generated_tb = out / "generated-tb.sv"
        else:
            sources, generated_tb = prepare(args.recipes.resolve(), out, args.constant_delay_fast_path)
        traces = {}
        for flavor in ("original",) if args.rtl_only else ("original", "generated"):
            if flavor == "original" and args.reuse_rtl:
                summary["results"]["original_build"] = previous["results"]["original_build"]
                missing = [case for case in CASES if not
                           (out / ("r%d_w%d_r%d_w%d" % case) / "original.log").exists()]
                with ThreadPoolExecutor(max_workers=args.workers) as pool:
                    for name, record, trace in pool.map(
                            lambda case: run_case(case, "original", out, args.timeout, env), missing):
                        print(f"original: {name} {record['status']} ({record['seconds']}s)", flush=True)
                for case in CASES:
                    name = "r%d_w%d_r%d_w%d" % case
                    log_path = out / name / "original.log"
                    log = log_path.read_text()
                    coverage, trace = validate(log, case)
                    elapsed = re.findall(r"walltime ([0-9.]+) s;", log)
                    if len(elapsed) != 1:
                        raise ValueError(f"Missing normal simulator report: {name}")
                    summary["cases"][name] = dict(original=dict(coverage, status="passed", log=str(log_path),
                        seconds=float(elapsed[0]), revalidated=True,
                        log_sha256=hashlib.sha256(log_path.read_bytes()).hexdigest()))
                    traces[name] = trace
                summary["original_passed"] = len(CASES)
                summary["original_phase_seconds"] = (previous.get("original_phase_seconds", previous["seconds"])
                                                       + round(time.monotonic() - start, 3))
                save()
                continue
            if flavor == "generated":
                summary["results"]["elaboration"] = execute([sys.executable, __file__, "--worker", "--output", out],
                    out, out / "elaboration.log", args.timeout,
                    dict(env, PYTHONPATH=str(args.najaeda_path.resolve())))
                summary.update(json.loads((out / "producer.json").read_text()))
                # Keep upstream verification, isolation is already in the dump.
                exclude = re.compile(r"/main/src/|/main/test/")
                files = [line for line in sources.splitlines()
                         if line.startswith("+") or not exclude.search(line)]
                files += [str(out / "host/main/src/regs/slink_reg_pkg.sv"), str(out / "netlist.v"),
                          str(out / "naja_primitives.v"), str(generated_tb)]
                (out / "generated.f").write_text("\n".join(files) + "\n")
            obj = out / (flavor + "-obj")
            summary["results"][flavor + "_build"] = execute([args.verilator, "--binary", "--timing", "--assert",
                "--timescale", "1ns/1ps", "--top-module", "tb_axi_slink", "-Wno-fatal", "-j", args.jobs,
                "--Mdir", obj, "-f", out / (flavor + ".f")], out, out / (flavor + "-build.log"), args.timeout, env)
            if flavor == "original":
                (out / "original-identity.json").write_text(json.dumps(original_identity(out), indent=2) + "\n")
            save()
            # Complete and validate the entire RTL matrix before elaboration.
            failures = []
            with ThreadPoolExecutor(max_workers=args.workers) as pool:
                for name, record, trace in pool.map(
                        lambda case: run_case(case, flavor, out, args.timeout, env), CASES):
                    summary["cases"].setdefault(name, {})[flavor] = record
                    if record["status"] == "passed":
                        if flavor == "original":
                            traces[name] = trace
                    if flavor == "generated" and trace is not None:
                        try:
                            compare_traces(traces[name], trace)
                        except ValueError as error:
                            record.update(status="failed", comparison_error=str(error))
                    if record["status"] != "passed":
                        failures.append(name)
                    save()
                    print(f"{flavor}: {name} {record['status']} ({record['seconds']}s)", flush=True)
            if failures:
                raise ValueError(f"{flavor}: {len(failures)} failed cases: {failures}")
            summary[flavor + "_passed"] = len(CASES)
            if flavor == "original":
                summary["original_phase_seconds"] = round(time.monotonic() - start, 3)
            save()
        summary["status"] = "rtl_passed" if args.rtl_only else "passed"
        save()
        return 0
    except Exception as error:
        summary.update(status="failed", error=str(error))
        save()
        raise


if __name__ == "__main__":
    sys.exit(main())
