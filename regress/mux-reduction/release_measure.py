# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Run the predeclared Release study against archived, uninstrumented modules.

Cases JSON: list of {name, lang, files, top, optional flist}.
Archives JSON: ordered mapping of label to {path, revision}.
Use absolute paths in both manifests; keep them outside the repository.
"""

import argparse
import csv
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import subprocess
import sys


def input_hashes(cases):
    paths = set()
    for case in cases:
        paths.update(Path(name).resolve() for name in case["files"])
        if case.get("flist"):
            flist = Path(case["flist"]).resolve()
            paths.add(flist)
            for line in flist.read_text().splitlines():
                line = line.strip()
                if not line or line.startswith("//"):
                    continue
                if line.startswith("+incdir+"):
                    for directory in line[len("+incdir+"):].split("+"):
                        directory = Path(directory)
                        if not directory.is_absolute():
                            directory = flist.parent / directory
                        paths.update(p.resolve() for p in directory.rglob("*") if p.is_file())
                elif not line.startswith("+"):
                    path = Path(line)
                    paths.add((path if path.is_absolute() else flist.parent / path).resolve())
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)}


def execution_plan(case_count, labels, blocks, seed):
    orders = list(itertools.permutations(labels))
    if blocks % len(orders):
        raise ValueError("blocks must be a multiple of the number of revision permutations")
    orders *= blocks // len(orders)
    rng = random.Random(seed)
    rng.shuffle(orders)
    plan = []
    for block, order in enumerate(orders, 1):
        cases = list(range(case_count))
        rng.shuffle(cases)
        for case in cases:
            for position, label in enumerate(order, 1):
                plan.append(dict(block=block, position=position, label=label, case=case))
    return plan


def loaded_naja_libraries():
    import ctypes

    dyld = ctypes.CDLL(None)
    dyld._dyld_image_count.restype = ctypes.c_uint32
    dyld._dyld_get_image_name.argtypes = [ctypes.c_uint32]
    dyld._dyld_get_image_name.restype = ctypes.c_char_p
    paths = [Path(os.fsdecode(dyld._dyld_get_image_name(i))).resolve()
             for i in range(dyld._dyld_image_count())]
    return sorted(str(p) for p in paths if p.name.startswith("libnaja") or p.name == "naja.so")


def worker(cases_path, case_index, archive, expected_revision):
    import resource
    import time
    import naja

    archive = Path(archive).resolve()
    producer = naja.getGitHash()
    if not producer or not expected_revision.startswith(producer):
        raise RuntimeError(f"producer mismatch: expected {expected_revision}, loaded {producer}")
    if Path(naja.__file__).resolve().parent != archive:
        raise RuntimeError("Python extension was loaded outside the selected archive")
    case = json.loads(Path(cases_path).read_text())[int(case_index)]
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    start = time.perf_counter_ns()
    if case["lang"] == "sv":
        db.loadSystemVerilog(case["files"], keep_assigns=True, flist=case.get("flist"),
            diagnostics_report_path=None, frontend_options={"slang": ["--top", case["top"]]})
    else:
        db.loadVHDL(case["files"], top=case["top"], diagnostics_report_path=None)
    elapsed = time.perf_counter_ns() - start
    rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss

    stats = dict(instances=0, muxes=0, mux_bits=0, nets=0, bitnets=0)
    visited = set()

    def visit(design):
        key = (design.getLibrary().getID(), design.getID())
        if key in visited:
            return
        visited.add(key)
        stats["nets"] += sum(1 for _ in design.getNets())
        stats["bitnets"] += sum(1 for _ in design.getBitNets())
        for instance in design.getInstances():
            stats["instances"] += 1
            model = instance.getModel()
            if model.getName().startswith("naja_mux2__w"):
                stats["muxes"] += 1
                stats["mux_bits"] += model.getTerm("Y").getWidth()
            if not model.isPrimitive():
                visit(model)

    visit(db.getTopDesign())
    libraries = loaded_naja_libraries()
    if not libraries or any(Path(p).parent != archive for p in libraries):
        raise RuntimeError(f"Naja runtime escaped the selected archive: {libraries}")
    print("RESULT " + json.dumps(dict(revision=producer, load_ms=elapsed / 1e6,
        peak_rss_bytes=rss, libraries=libraries, **stats), sort_keys=True), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--archives", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--blocks", type=int, default=18)
    parser.add_argument("--seed", type=int, default=536535)
    parser.add_argument("--plan-only", action="store_true")
    args = parser.parse_args()
    if sys.platform != "darwin":
        parser.error("this protocol uses macOS byte-valued ru_maxrss and dyld auditing")
    cases = json.loads(args.cases.read_text())
    archives = json.loads(args.archives.read_text())
    if len({case["name"] for case in cases}) != len(cases):
        parser.error("workload names must be unique")
    plan = execution_plan(len(cases), list(archives), args.blocks, args.seed)
    args.output.mkdir(parents=True, exist_ok=True)
    plan_path = args.output / "plan.json"
    if plan_path.exists() and json.loads(plan_path.read_text()) != plan:
        parser.error("output directory already contains a different execution plan")
    plan_path.write_text(json.dumps(plan, indent=2))
    if args.plan_only:
        print(f"{len(plan)} retained samples planned")
        return
    csv_path = args.output / "samples.csv"
    if csv_path.exists():
        parser.error("samples.csv already exists; preserve it and use a new output directory")
    hashes = input_hashes(cases)
    (args.output / "inputs-before.json").write_text(json.dumps(hashes, indent=2))
    fields = ["block", "position", "revision", "label", "workload", "load_ms",
              "peak_rss_bytes", "instances", "muxes", "mux_bits", "nets", "bitnets"]
    reference_stats = {}

    def sample(event, warmup=False):
        label = event["label"]
        archive = archives[label]
        case = cases[event["case"]]
        prefix = "warmup" if warmup else f"block-{event['block']:02d}"
        env = dict(os.environ, PYTHONPATH=str(Path(archive["path"]).resolve()),
                   DYLD_LIBRARY_PATH=str(Path(archive["path"]).resolve()), PYTHONHASHSEED="0")
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()), "--worker",
            str(args.cases.resolve()), str(event["case"]), archive["path"], archive["revision"]],
            cwd=args.output, env=env, text=True, capture_output=True, timeout=120)
        (args.output / f"{prefix}-{case['name']}-{label}.log").write_text(
            result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f"{prefix} {case['name']} {label} failed: {result.returncode}")
        records = [json.loads(line[7:]) for line in result.stdout.splitlines() if line.startswith("RESULT ")]
        if len(records) != 1:
            raise RuntimeError("worker must emit exactly one result")
        record = records[0]
        stats = {k: record[k] for k in fields if k in record and k not in
                 {"revision", "load_ms", "peak_rss_bytes"}}
        key = (case["name"], label)
        if reference_stats.setdefault(key, stats) != stats:
            raise RuntimeError(f"structural counts changed within {key}")
        return {k: v for k, v in dict(**event, **record, workload=case["name"]).items() if k in fields}

    for case in range(len(cases)):
        for label in archives:
            sample(dict(block=0, position=0, label=label, case=case), warmup=True)
    print("Warm-ups passed; beginning balanced retained samples", flush=True)
    with csv_path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        for index, event in enumerate(plan, 1):
            writer.writerow(sample(event))
            stream.flush()
            if index % (len(cases) * len(archives)) == 0:
                print(f"Completed block {event['block']}/{args.blocks}", flush=True)
    after = input_hashes(cases)
    (args.output / "inputs-after.json").write_text(json.dumps(after, indent=2))
    if hashes != after:
        raise RuntimeError("workload inputs changed during sampling")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--worker":
        worker(*sys.argv[2:])
    else:
        main()
