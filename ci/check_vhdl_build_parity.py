#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Compare raw VHDL intent, live netlists and snapshots from two source builds.

Each package directory must contain najaeda/ with its matching compiled module.
Python interpreters must match the respective extension's ABI. No identity,
metadata or snapshot differences are normalized away. Only Verilog banner
timestamps use the existing determinism test's permitted normalization.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import warnings

ROOT = Path(__file__).resolve().parents[1]


def worker(source, output, retain, snapshot):
    sys.path.insert(0, str(ROOT / 'test/najaeda'))
    from najaeda import naja
    from test_najaeda_determinism import dump_state
    from test_najaeda_vhdl_intent import PACKAGE, SOURCE

    output.mkdir()
    if snapshot:
        db = naja.NLDB.loadNajaIF(str(snapshot))
        if naja.intentAvailable():
            raise RuntimeError('Cold snapshots must not expose retained intent')
    else:
        source.write_text(PACKAGE + SOURCE, encoding='utf-8')
        universe = naja.NLUniverse.create()
        db = naja.NLDB.create(universe)
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', RuntimeWarning)
            db.loadVHDL(str(source), top='top', keep_ast_link=retain, diagnostics_report_path=None)
    def save(name, value):
        (output / name).write_text(json.dumps(value, sort_keys=True, indent=2), encoding='utf-8')
    save('identity.json', {'version': naja.getVersion(), 'git_hash': naja.getGitHash()})
    save('api.json', {name: sorted(n for n in dir(obj) if not n.startswith('_'))
                     for name, obj in [('module', naja), ('NLDB', naja.NLDB), ('SNLDesign', naja.SNLDesign)]})
    intent = []
    for library in db.getLibraries():
        for design in library.getSNLDesigns():
            for obj in [design, *design.getInstances(), *design.getTerms(), *design.getNets()]:
                intent.append([obj.getNLID().toTuple(), naja.intentDescribe(obj),
                               naja.intentTypeOf(obj), naja.intentParametersOf(obj)])
    save('intent.json', intent)
    dump_state(output / 'artifacts')
    naja.NLUniverse.get().destroy()


def artifact_bytes(directory):
    # Keep normalization identical to the existing determinism contract.
    result = {}
    for path in sorted(directory.rglob('*')):
        if not path.is_file():
            continue
        data = path.read_bytes()
        if path.suffix == '.v':
            lines = data.splitlines(keepends=True)
            if len(lines) > 1 and lines[0].rstrip(b'\r\n') == b'/' * 80 and lines[1].startswith(b'// '):
                lines[1] = b'// <timestamp>\n'
            data = b''.join(lines)
        result[path.relative_to(directory).as_posix()] = data
    return result


def compare(first, second):
    left, right = artifact_bytes(first), artifact_bytes(second)
    differences = sorted(name for name in left.keys() | right.keys() if left.get(name) != right.get(name))
    if differences:
        raise RuntimeError(f'{first.name} versus {second.name}: differences in {differences}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for build in ('reference', 'candidate'):
        parser.add_argument(f'--{build}-python', type=Path)
        parser.add_argument(f'--{build}-package', type=Path)
        parser.add_argument(f'--{build}-runtime-dir', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--retain', action='store_true', help=argparse.SUPPRESS)
    parser.add_argument('--snapshot', type=Path, help=argparse.SUPPRESS)
    parser.add_argument('--source', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.worker:
        worker(args.source, args.output, args.retain, args.snapshot)
        return
    args.output.mkdir(parents=True, exist_ok=False)
    def run(build, name, retain=False, snapshot=None):
        python = getattr(args, f'{build}_python')
        package = getattr(args, f'{build}_package')
        if not python or not package:
            parser.error(f'--{build}-python and --{build}-package are required')
        environment = os.environ.copy()
        environment['PYTHONPATH'] = str(package.resolve())
        runtime = getattr(args, f'{build}_runtime_dir')
        if runtime:
            for key in ('DYLD_LIBRARY_PATH', 'LD_LIBRARY_PATH'):
                environment[key] = str(runtime.resolve()) + (os.pathsep + environment[key] if environment.get(key) else '')
        output = args.output / name
        command = [str(python.resolve()), str(Path(__file__).resolve()), '--worker',
                   '--source', str(args.output / 'input.vhd'), '--output', str(output)]
        if retain:
            command.append('--retain')
        if snapshot:
            command.extend(['--snapshot', str(snapshot)])
        result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=120)
        if result.returncode:
            raise RuntimeError(f'{name} failed:\n{result.stdout}{result.stderr}')
        return output
    outputs = {build: {retain: run(build, f'{build}-{retain}', retain=retain) for retain in (False, True)}
               for build in ('reference', 'candidate')}
    for retain in (False, True):
        compare(outputs['reference'][retain], outputs['candidate'][retain])
    for build in outputs:
        compare(outputs[build][False] / 'artifacts', outputs[build][True] / 'artifacts')
    for producer, consumer in [('reference', 'candidate'), ('candidate', 'reference')]:
        for retain in (False, True):
            original = outputs[producer][retain] / 'artifacts'
            loaded = run(consumer, f'{consumer}-loads-{producer}-{retain}',
                         snapshot=original / 'snapshot')
            compare(original, loaded / 'artifacts')
    print('PASS: raw API, producer identity, VHDL intent, live state, Verilog, DOT, all snapshot bytes,')
    print('      retention on/off, and four fresh cross-build snapshot reloads agree.')
    print((outputs['reference'][True] / 'identity.json').read_text())


if __name__ == '__main__':
    main()
