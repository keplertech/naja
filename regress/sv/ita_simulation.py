#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Run the pinned upstream ITA golden-vector testbench on RTL and a Naja dump."""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tarfile
import time
import urllib.request

import pulp_benchmarks as pulp

ITA_REV = "ba96519becce195d64e85eb9a5302e8a1d5487e7"
ITA_URL = f"https://codeload.github.com/pulp-platform/ITA/tar.gz/{ITA_REV}"
ITA_SHA = "e0da67361496e0309943a18ba6f87bd6838ccf6c2ff1270e8979a1432bdef4fc"
MEM_REV = "3a3de73632a06826b1bd9c65a0a2e92b32016845"
MEM_URL = f"https://raw.githubusercontent.com/pulp-platform/tech_cells_generic/{MEM_REV}/src/rtl/tc_sram.sv"
MEM_SHA = "e815e5d70aa530f8bcd99dd68c8791c7f8b46295e5fdfd75651eb2807f79abe9"


def fetch(url, path, checksum):
    if not path.exists():
        with urllib.request.urlopen(url, timeout=60) as response:
            path.write_bytes(response.read())
    if hashlib.sha256(path.read_bytes()).hexdigest() != checksum:
        raise ValueError(f"Checksum mismatch: {path}")
    return path


def adapt_testbench(source, vectors):
    # Verilator 5.052 generates invalid C++ for fscanf into packed array slices.
    pattern = r'ret_code = \$fscanf\(([^,]+), ("[^"]+"), ([^;]+)\);'
    def scalar_scan(match):
        fd, fmt, target = match.groups()
        return ("begin\n      integer scanned_value;\n"
                f"      ret_code = $fscanf({fd}, {fmt}, scanned_value);\n"
                '      if (ret_code != 1) $fatal(1, "ITA vector read failed");\n'
                f"      {target} = scanned_value;\n    end")
    source, count = re.subn(pattern, scalar_scan, source)
    if count != 10:
        raise ValueError("Upstream testbench scan layout changed")
    source = source.replace('"../../simvectors/data_S"', json.dumps(str(vectors / "data_S")))
    # Preserve upstream's continue-on-mismatch checker, but reject its log below.
    # A complete trace also distinguishes baseline failures from lowering drift.
    check = "if (requant_oup !== exp_res) begin"
    if source.count(check) != 1:
        raise ValueError("Upstream scoreboard layout changed")
    source = source.replace(check,
        '$display("NAJA_ITA_OUTPUT phase=%0d data=%h time=%0t", phase, requant_oup, $time);\n'
        '        ' + check)
    if source.count("$finish();") != 1:
        raise ValueError("Upstream testbench completion layout changed")
    return source.replace("$finish();", '$display("NAJA_ITA_COMPLETE");\n    $finish();')


def model_weight_read_delay(source, delay_ps):
    """Model latch-memory read propagation without replacing its elaborated logic.

    Verilator's zero-delay gated latch scheduling can expose a transient old
    write-data value to a same-edge reader. Delay only the read port, equally
    on RTL and dump; delaying cascaded clock gates changes their enable timing.
    """
    if not 0 <= delay_ps <= 100:
        raise ValueError("Weight read delay must be between 0 and 100 ps")
    name = "ita_register_file_1w_multi_port_read_we"
    matches = list(re.finditer(r"module " + name + r"\b.*?endmodule", source, re.S))
    if len(matches) != 1:
        raise ValueError("Expected exactly one ITA weight buffer module")
    match = matches[0]
    module = match.group()
    header, body = module.split(");", 1)
    port = re.search(r"output\s+(?:logic\s+)?((?:\[[^]]+\]\s*)+)ReadData\b", header)
    shape = re.sub(r"\s+", "", port[1]) if port else None
    if shape not in ("[N_READ-1:0][DATA_WIDTH-1:0]", "[8191:0]"):
        raise ValueError("ITA weight buffer read port shape changed")
    if "naja_sim_read_data" in module:
        raise ValueError("ITA weight read delay already applied")
    # The pinned benchmark adds a Verilator-only read delay. Replace it with
    # this experiment's setting so RTL and dump use exactly the same delay.
    body = re.sub(r"\n`ifdef VERILATOR\n\s*#1ps\n`endif\n", "\n", body)
    if delay_ps == 0:
        return source[:match.start()] + header + ");" + body + source[match.end():]
    body = re.sub(r"\bReadData\b", "naja_sim_read_data", body)
    prefix = ("\n  timeunit 1ns; timeprecision 1ps;\n"
              f"  wire {port[1]} naja_sim_read_data;\n"
              f"  assign #({delay_ps}ps) ReadData = naja_sim_read_data;\n")
    return source[:match.start()] + header + ");" + prefix + body + source[match.end():]


def attach_memory(netlist):
    # This adapter is specific to the source-pinned ita.default variant.
    # Reject shape changes instead of silently simulating the wrong memory.
    pattern = r"module tc_sram_blackbox\(.*?endmodule //tc_sram_blackbox"
    matches = list(re.finditer(pattern, netlist, re.S))
    if len(matches) != 1:
        raise ValueError("Expected exactly one ITA SRAM blackbox declaration")
    declaration = matches[0].group()
    expected = ("input clk_i", "input rst_ni", "input [1:0] req_i", "input [1:0] we_i",
                "input [15:0] addr_i", "input [831:0] wdata_i", "input [103:0] be_i",
                "output [831:0] rdata_o")
    if not all(port in declaration for port in expected) or declaration.count(";") != 1:
        raise ValueError("ITA SRAM blackbox shape/body changed")
    model = ("ita_sim_sram #(.NumWords(256), .DataWidth(416), .ByteWidth(8), "
             ".NumPorts(2), .Latency(1)) model (.*);\n")
    return netlist[:matches[0].start()] + declaration.replace(
        "endmodule", model + "endmodule", 1) + netlist[matches[0].end():]


def output_trace(log):
    if "NAJA_ITA_COMPLETE" not in log or "%Error" in log:
        raise ValueError("Simulation did not complete")
    counts = re.findall(r"(\d+) outputs were checked in phase (\d+)\.", log)
    expected = [("256", str(phase)) for phase in (0, 1, 2, 3, 3, 4, 5, 6)]
    if counts != expected:
        raise ValueError(f"Incomplete output coverage: {counts}")
    trace = re.findall(r"NAJA_ITA_OUTPUT phase=(\d+) data=([0-9a-f]+) time=([^\n]+)", log)
    if len(trace) != 2048:
        raise ValueError(f"Incomplete output trace: {len(trace)} transactions")
    return trace


def validate_simulation(log):
    output_trace(log)
    if "Wrong value" in log:
        raise ValueError("Simulation did not pass the upstream scoreboard")


def execute(command, cwd, log, timeout):
    start = time.monotonic()
    record = dict(command=list(map(str, command)), cwd=str(cwd), log=str(log))
    with log.open("w") as output:
        try:
            process = subprocess.Popen(record["command"], cwd=cwd, stdout=output,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            returncode = process.wait(timeout=timeout)
            record.update(returncode=returncode,
                          status="passed" if returncode == 0 else "failed")
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            record["status"] = "timeout"
    record["seconds"] = round(time.monotonic() - start, 3)
    return record


def run_pipelines(items, run_pipeline, parallel=False):
    """Run independent variants without sharing build directories or reports."""
    if not parallel:
        for name, flist in items:
            yield name, run_pipeline(name, flist)
        return
    with ThreadPoolExecutor(max_workers=2) as executor:
        futures = {executor.submit(run_pipeline, name, flist): name
                   for name, flist in items}
        for future in as_completed(futures):
            yield futures[future], future.result()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=pulp.ROOT / "build/ita-simulation")
    parser.add_argument("--archive", type=Path)
    parser.add_argument("--cache", type=Path, default=pulp.DEFAULT_CACHE,
                        help="Cache prepared PULP packages by the shared Git revision pin")
    parser.add_argument("--najaeda-path", type=Path, default=pulp.ROOT / "build/test/najaeda")
    parser.add_argument("--generator-python", default=sys.executable,
                        help="Python with numpy and onnx installed")
    parser.add_argument("--jobs", type=int, default=4,
                        help="Total compiler-job budget shared by concurrent simulations")
    parser.add_argument("--parallel-simulations", action="store_true",
                        help="Build and simulate RTL and generated netlist concurrently")
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--weight-read-delay-ps", type=int, default=1,
                        help="Latch-memory read propagation delay (0 reproduces the zero-delay race)")
    parser.add_argument("--no-stalls", action="store_true")
    parser.add_argument("--verilator-opt", choices=("default", "0", "1", "2", "3"),
                        default="default")
    parser.add_argument("--hierarchical", action=argparse.BooleanOptionalAction, default=True,
                        help="Compile large generated datapaths as separate Verilator blocks")
    args = parser.parse_args()
    if args.timeout <= 0 or args.jobs <= 0 or not 0 <= args.weight_read_delay_ps <= 100:
        parser.error("timeout/jobs must be positive; weight read delay must be 0..100 ps")
    if args.parallel_simulations and args.jobs < 2:
        parser.error("parallel simulations require at least two compiler jobs")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    summary = dict(ita_revision=ITA_REV, memory_revision=MEM_REV, seed=args.seed,
                   stalls=not args.no_stalls, weight_read_delay_ps=args.weight_read_delay_ps,
                   parallel_simulations=args.parallel_simulations, compiler_jobs=args.jobs,
                   status="running", results={})
    def save():
        (out / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    try:
        ita_archive = fetch(ITA_URL, out / "ita.tar.gz", ITA_SHA)
        with tarfile.open(ita_archive) as archive:
            archive.extractall(out / "upstream", filter="data")
        upstream = out / "upstream" / f"ITA-{ITA_REV}"
        memory = fetch(MEM_URL, out / "tc_sram.sv", MEM_SHA)
        package = (pulp.unpack(args.archive.resolve(), out / "prepared") if args.archive
                   else pulp.prepare(args.cache))
        summary["benchmark_revision"] = pulp.REVISION
        source = package / "ita/variants/default"
        generate = execute([args.generator_python, upstream / "testGenerator.py",
                            "--seed", args.seed, "-H", 1, "-S", 64, "-E", 64,
                            "-P", 64, "-F", 64, "--activation", "identity"],
                           upstream, out / "vectors.log", args.timeout)
        summary["results"]["vectors"] = generate
        save()
        if generate["status"] != "passed":
            summary["status"] = "failed"
            save()
            return 1
        vector_hashes = {str(path.relative_to(upstream)): hashlib.sha256(path.read_bytes()).hexdigest()
                         for path in sorted((upstream / "simvectors").rglob("*.txt"))}
        (out / "vector-sha256.json").write_text(json.dumps(vector_hashes, indent=2) + "\n")
        tb = out / "ita_tb.sv"
        tb.write_text(adapt_testbench((upstream / "src/tb/ita_tb.sv").read_text(),
                                     upstream / "simvectors"))
        clocks = [upstream / f"src/tb/{name}.sv" for name in ("rst_gen", "clk_rst_gen")]
        original = out / "original.f"
        lines = []
        for line in (source / "filelist.f").read_text().splitlines():
            if not line.strip():
                continue
            if line.startswith("+incdir+"):
                line = "+incdir+" + str((source / line[8:]).resolve())
            elif not line.startswith("+"):
                line = str(memory if line.endswith("tc_sram_stubs.sv") else (source / line).resolve())
            if line.endswith("/ita_register_file_1w_multi_port_read_we.sv"):
                buffer = out / "weight_buffer.sv"
                buffer.write_text(model_weight_read_delay(Path(line).read_text(), args.weight_read_delay_ps))
                line = str(buffer)
            lines.append(line)
        original.write_text("\n".join(lines + list(map(str, clocks)) + [str(tb)]) + "\n")
        dump = pulp.run_target(package, "ita.default", "ita", out / "dump",
                               args.najaeda_path.resolve(), args.timeout)
        summary["results"]["dump"] = dump
        save()
        lists = {"original": original}
        if dump["status"] == "passed":
            netlist = out / "generated.v"
            netlist.write_text(model_weight_read_delay(
                attach_memory((out / "dump/netlist.v").read_text()), args.weight_read_delay_ps))
            mem_model = out / "memory.sv"
            mem_model.write_text(memory.read_text().replace("module tc_sram #(", "module ita_sim_sram #("))
            generated = out / "generated.f"
            files = [package / "ita/dependencies/common_cells/src/cf_math_pkg.sv",
                     package / "ita/main/src/ita_package.sv", mem_model, netlist,
                     out / "dump/naja_primitives.v",
                     *clocks, tb]
            generated.write_text("\n".join(map(str, files)) + "\n")
            lists["generated"] = generated
        def run_pipeline(name, flist):
            results = {}
            hierarchical = name == "generated" and args.hierarchical
            obj = out / (f"{name}-hier-obj" if hierarchical else f"{name}-obj")
            # Avoid stale executables after an unsuccessful rebuild.
            (obj / "Vita_tb").unlink(missing_ok=True)
            extra = []
            if hierarchical:
                config = out / "hierarchy.vlt"
                config.write_text('`verilator_config\n' + ''.join(
                    f'hier_block -module "{module}"\n' for module in
                    ("ita_dotp", "ita_controller", "ita_requantizer", "ita_softmax")))
                extra = ["--hierarchical", config]
            optimization = [] if args.verilator_opt == "default" else [f"-O{args.verilator_opt}"]
            # Verilator 5.052 can recurse rebuilding the top makefile when
            # parallel hierarchical jobs update wrapper timestamps underneath it.
            jobs = args.jobs
            if hierarchical:
                jobs = 1
            elif args.parallel_simulations and len(lists) > 1:
                jobs = max(1, args.jobs - 1) if args.hierarchical else max(1, args.jobs // len(lists))
            build = execute(["verilator", "--binary", "--timing", *optimization,
                             "-j", jobs, "--top-module", "ita_tb", "-Wno-fatal",
                             "+define+BIAS=1", f"+define+NO_STALLS={int(args.no_stalls)}",
                             "--Mdir", obj, *extra, "-f", flist], out,
                            out / f"{name}-build.log", args.timeout)
            results[f"{name}_build"] = build
            if build["status"] != "passed":
                return results
            run = execute([obj / "Vita_tb", "+verilator+seed+1"], out,
                          out / f"{name}-run.log", args.timeout)
            if run["status"] == "passed":
                try:
                    validate_simulation((out / f"{name}-run.log").read_text())
                except ValueError as error:
                    run.update(status="failed", error=str(error))
            results[f"{name}_simulation"] = run
            return results
        for name, results in run_pipelines(lists.items(), run_pipeline, args.parallel_simulations):
            summary["results"].update(results)
            save()
        try:
            traces = [output_trace((out / f"{name}-run.log").read_text())
                      for name in ("original", "generated")
                      if f"{name}_simulation" in summary["results"]]
            matched = len(traces) == 2 and traces[0] == traces[1]
            summary["rtl_netlist_comparison"] = dict(
                status="passed" if matched else "failed",
                transactions=len(traces[0]) if traces else 0)
        except ValueError as error:
            summary["rtl_netlist_comparison"] = dict(status="failed", error=str(error))
        failed = any(summary["results"].get(f"{name}_simulation", {}).get("status") != "passed"
                     for name in ("original", "generated"))
        failed = failed or summary["rtl_netlist_comparison"]["status"] != "passed"
        summary["status"] = "failed" if failed else "passed"
        save()
        print(f"ITA validation: {summary['status']}; report: {out / 'summary.json'}")
        return int(failed)
    except Exception as error:
        summary["error"] = str(error)
        summary["status"] = "failed"
        save()
        raise


if __name__ == "__main__":
    sys.exit(main())
