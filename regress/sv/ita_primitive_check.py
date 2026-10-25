#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Compare ITA latch scheduling on RTL and a Naja dump in two simulators.

Zero-delay golden failures are recorded, not treated as passes. Exit success
means RTL/netlist behavior matches and every 1 ps read-delay control passes.
The nonblocking primitive candidate exists only in the output directory.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess

from pulp_benchmarks import ROOT, validate_diagnostics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/ita-primitive-check')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parent
    primitives = out / 'naja_primitives.v'
    from najaeda import netlist

    try:
        top = netlist.load_system_verilog([str(source / 'ita_latch_dut.sv')],
            config=netlist.SystemVerilogConfig(top='ita_latch_dut',
                diagnostics_report_path=str(out / 'diagnostics.log'),
                blackbox_unknown_modules=False))
        validate_diagnostics((out / 'diagnostics.log').read_text())
        top.dump_gate_level_verilog(str(out / 'netlist.v'))
    finally:
        netlist.reset()
    candidate = primitives.read_text()
    if candidate.count('if (E) Q = D;') != 1:
        raise ValueError('Latch primitive changed; review the NBA candidate')
    (out / 'primitives-nba.v').write_text(candidate.replace('if (E) Q = D;', 'if (E) Q <= D;'))
    summary = dict(versions={}, results=[])
    for tool, flag in (('iverilog', '-V'), ('verilator', '--version')):
        version = subprocess.run([tool, flag], capture_output=True, text=True, check=True, timeout=10)
        summary['versions'][tool] = version.stdout.splitlines()[0]
    for model, files in (
            ('rtl', [source / 'ita_latch_dut.sv']),
            ('netlist', [out / 'netlist.v', primitives]),
            ('netlist-nba', [out / 'netlist.v', out / 'primitives-nba.v'])):
        for simulator in ('iverilog', 'verilator'):
            for delay in (0, 1):
                name = f'{model}-{simulator}-{delay}'
                obj = out / name
                sources = list(map(str, [source / 'ita_latch_repro.sv', *files]))
                if simulator == 'iverilog':
                    command = ['iverilog', '-g2012', '-s', 'ita_latch_repro',
                               f'-Pita_latch_repro.READ_DELAY_PS={delay}', '-o', str(obj), *sources]
                    run = ['vvp', str(obj)]
                else:
                    command = ['verilator', '--binary', '--timing', '-j', '2',
                               '--top-module', 'ita_latch_repro', f'-GREAD_DELAY_PS={delay}',
                               '-Wno-fatal', '--Mdir', str(obj), *sources]
                    run = [str(obj / 'Vita_latch_repro')]
                with (out / f'{name}-build.log').open('w') as log:
                    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=120)
                result = subprocess.run(run, capture_output=True, text=True, timeout=10)
                log = result.stdout + result.stderr
                (out / f'{name}-run.log').write_text(log)
                observed = re.search(r'captured=([0-9a-fx]+) read_data=([0-9a-fx]+)', log)
                if not observed:
                    raise ValueError(f'Missing observation from {name}')
                record = dict(model=model, simulator=simulator, delay_ps=delay,
                              golden_status='passed' if result.returncode == 0 else 'failed',
                              returncode=result.returncode, observed=list(observed.groups()))
                summary['results'].append(record)
                (out / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
                print(f'{name}: golden {record["golden_status"]}; {observed.group()}', flush=True)
    records = summary['results']
    same = all(r['observed'] == ref['observed'] and r['golden_status'] == ref['golden_status']
               for r in records for ref in records
               if ref['model'] == 'rtl' and r['simulator'] == ref['simulator']
               and r['delay_ps'] == ref['delay_ps'])
    timed = all(r['golden_status'] == 'passed' for r in records if r['delay_ps'] == 1)
    summary.update(comparison_status='matched' if same else 'different',
                   timed_checks_status='passed' if timed else 'failed')
    (out / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    return int(not (same and timed))


if __name__ == '__main__':
    raise SystemExit(main())
