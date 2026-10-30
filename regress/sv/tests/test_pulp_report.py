# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import sys
from pathlib import Path
import unittest
import json
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pulp_report import render, collect_artifacts


class PulpReportTest(unittest.TestCase):
    def test_pinned_source_commit_is_identified(self):
        summary = dict(revision='abc123', selected=[], inventory=[], results=[])
        self.assertIn('commit abc123', render([('Survey', summary)]))

    def test_collect_parallel_artifacts_preserves_modes_and_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name, suffix, mode, status in (
                    ('pulp-lint-a', 'summary.json', 'load + dump + lint', 'failed'),
                    ('pulp-elab-b', 'elaboration/summary.json', 'elaboration', 'passed')):
                path = root / name / suffix
                path.parent.mkdir(parents=True)
                path.write_text(json.dumps(dict(release='0.1.0', mode=mode,
                    selected=[name], inventory=[name], results=[dict(target=name, status=status)])))
            path = root / 'pulp-ita/ita/summary.json'
            path.parent.mkdir(parents=True)
            path.write_text(json.dumps(dict(status='failed', results={})))
            suites, functional = collect_artifacts(root)
            self.assertEqual(2, len(suites))
            self.assertEqual('failed', functional['status'])
            report = render(suites, functional)
            self.assertIn('❌ failed', report)
            self.assertIn('elaboration', report)
            self.assertIn('Overall: **failed**', report)
            self.assertEqual(([], None), collect_artifacts(root / 'missing'))

    def test_partial_run_and_measurements(self):
        summary = dict(release='0.1.0', selected=['a', 'b', 'c'], inventory=['a', 'b', 'c', 'd'],
                       results=[dict(target='a', status='passed', seconds=2, lint_seconds=3,
                                     design_stats=dict(top='top', top_instances=1234, load_seconds=1)),
                                dict(target='b', status='timeout', seconds=10, error='<bad>|\ninput')])
        report = render([('Smoke', summary), ('Elaboration', None)])
        self.assertIn('1/3 passed', report)
        self.assertIn('| 3.000 | 5.000 |', report)
        self.assertIn('1,234', report)
        self.assertIn('❌ timeout', report)
        self.assertIn('⏸️ not run', report)
        self.assertIn('Not run / report unavailable', report)
        self.assertIn('Release variants not selected\n\nd', report)
        self.assertIn('&lt;bad&gt;&#124; input', report)

    def test_trace_pass_does_not_hide_scoreboard_failure(self):
        report = render([], dict(status='failed', weight_read_delay_ps=1, results={'original_simulation': dict(status='failed', seconds=1)},
                                 rtl_netlist_comparison=dict(status='passed', transactions=2048)))
        self.assertIn('Weight-buffer read propagation delay: 1 ps.', report)
        self.assertIn('Overall: **failed**', report)
        self.assertIn('comparison: **passed**', report)
        self.assertIn('2,048', report)


if __name__ == '__main__':
    unittest.main()
