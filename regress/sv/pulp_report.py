#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Render PULP JSON results as a GitHub Actions job summary."""

import argparse
import html
import json
import os
from pathlib import Path


def cell(value):
    return html.escape(str(value)).replace('|', '&#124;').replace('\n', ' ')


def number(value):
    return '—' if value is None else f'{value:,}'


def seconds(value):
    return '—' if value is None else f'{value:.3f}'


def render(suites, functional=None):
    lines = ['# PULP SV regression', '',
             'Elaboration and structural lint are frontend checks, not functional equivalence proofs.', '']
    inventory, selected = set(), set()
    for label, summary in suites:
        lines += [f'## {cell(label)}', '']
        if summary is None:
            lines += ['**Not run / report unavailable.** Check the job steps for the cause.', '']
            continue
        results = {r['target']: r for r in summary['results']}
        names = summary.get('selected', list(results))
        inventory.update(summary.get('inventory', names))
        selected.update(names)
        passed = sum(results.get(n, {}).get('status') == 'passed' for n in names)
        source = ('commit ' + cell(summary['revision']) if 'revision' in summary
                  else 'release ' + cell(summary['release']))
        lines += [f'**{passed}/{len(names)} passed** · {cell(summary.get("mode", label))} · {source}', '',
                  '| Case | Result | Load (s) | Stats (s) | Dump (s) | Lint (s) | Total (s) |',
                  '|---|---|---:|---:|---:|---:|---:|']
        for name in names:
            result = results.get(name, {})
            stats = result.get('design_stats', {})
            status = result.get('status', 'not run')
            total = (result['seconds'] + result.get('lint_seconds', 0)) if 'seconds' in result else None
            values = [cell(name), ('✅ ' if status == 'passed' else '❌ ' if result else '⏸️ ') + cell(status),
                      *[seconds(stats.get(key + '_seconds')) for key in ('load', 'stats', 'dump')],
                      seconds(result.get('lint_seconds')), seconds(total)]
            lines.append('| ' + ' | '.join(values) + ' |')
        lines += ['', 'Counts from najaeda on the elaborated top model. Instances are immediate children; '
                  'terms and nets count each bus once, while bit counts expand buses. These are not flattened cell counts.', '',
                  '| Case | Top | Instances | Terms | Term bits | Nets | Net bits | Dump (bytes) |',
                  '|---|---|---:|---:|---:|---:|---:|---:|']
        for name in names:
            result = results.get(name, {})
            stats = result.get('design_stats', {})
            values = [cell(name), cell(stats.get('top', result.get('top', '—'))),
                      *[number(stats.get(key)) for key in ('top_instances', 'top_terms', 'top_bit_terms', 'top_nets', 'top_bit_nets')],
                      number(result.get('dump_bytes'))]
            lines.append('| ' + ' | '.join(values) + ' |')
        for name, result in results.items():
            if result.get('error'):
                lines += ['', f'Error ({cell(name)}): {cell(result["error"])}', '']
        lines += ['']
    if inventory - selected:
        lines += ['## Release variants not selected', '', ', '.join(cell(n) for n in sorted(inventory - selected)), '']
    lines += ['## ITA functional validation', '']
    if functional is None:
        lines += ['Not run in this job. The experimental upstream testbench is a separate validation path.', '']
    else:
        lines += [f'Weight-buffer read propagation delay: {number(functional.get("weight_read_delay_ps"))} ps.', '',
                  f'Overall: **{cell(functional["status"])}**', '', '| Stage | Result | Seconds |', '|---|---|---:|']
        for stage, result in functional['results'].items():
            lines.append(f'| {cell(stage)} | {cell(result["status"])} | {seconds(result.get("seconds"))} |')
        comparison = functional.get('rtl_netlist_comparison', {})
        lines += ['', f'RTL/netlist trace comparison: **{cell(comparison.get("status", "not run"))}**; '
                  f'transactions: {number(comparison.get("transactions"))}. '
                  'A matching trace does not override a failed upstream golden scoreboard.', '']
    lines += ['Full JSON results, diagnostics, logs and generated netlists are in this job’s `pulp-sv-regress-*` artifact.', '',
              'Times are measured wall-clock seconds. Total includes worker startup/cleanup and lint; '
              'unavailable measurements are shown as —. Statistics may survive a later dump or lint failure.', '']
    return '\n'.join(lines)


def read(path):
    return json.loads(path.read_text()) if path.is_file() else None


def collect_artifacts(root):
    suites, functional = [], None
    if root.is_dir():
        for artifact in sorted(root.iterdir()):
            if not artifact.is_dir():
                continue
            for suffix in ('summary.json', 'elaboration/summary.json'):
                path = artifact / suffix
                if path.is_file():
                    suites.append((artifact.name, read(path)))
            candidate = artifact / 'ita/summary.json'
            if candidate.is_file():
                if functional is not None:
                    raise ValueError('Multiple ITA functional reports in downloaded artifacts')
                functional = read(candidate)
    return suites, functional


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path('build/pulp-benchmarks'))
    parser.add_argument('--functional', type=Path)
    parser.add_argument('--aggregate-root', type=Path, help='Directory of separately downloaded job artifacts')
    args = parser.parse_args()
    if args.aggregate_root:
        suites, functional = collect_artifacts(args.aggregate_root)
        if not suites:
            suites = [('Parallel job reports', None)]
    else:
        suites = [('Load / dump / lint', read(args.root / 'summary.json')),
                  ('Larger-design elaboration', read(args.root / 'elaboration/summary.json'))]
        functional = read(args.functional) if args.functional else None
    report = render(suites, functional)
    args.root.mkdir(parents=True, exist_ok=True)
    (args.root / 'report.md').write_text(report)
    if os.environ.get('GITHUB_STEP_SUMMARY'):
        with Path(os.environ['GITHUB_STEP_SUMMARY']).open('a') as output:
            output.write(report)
    print(report)


if __name__ == '__main__':
    main()
