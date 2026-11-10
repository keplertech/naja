# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import os
import json
import subprocess
import tempfile
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import cvfpu_simulation as cvfpu


class CVFPUSimulationTest(unittest.TestCase):
    def successful_log(self):
        return "\n".join((
            "NAJA_CVFPU_STATE time=100 reset=1 iv=1 ir=1 ov=1 or=1 flush=0 busy=1",
            "NAJA_CVFPU_INPUT time=100 operands=00 boxed=7 rounding=0 op=2 mod=0 tag=0 mask=1 aux=0",
            "NAJA_CVFPU_OUTPUT time=100 result=1234 status=00 extension=0 tag=0 mask=1 aux=0",
            "UVM_INFO source(1) @ 100: checker [FPU_SB_COVERAGE] checked=1 skipped=0",
            "UVM_ERROR : 0", "UVM_FATAL : 0", "TEST PASSED", "NAJA_CVFPU_COMPLETE"))

    def test_complete_nonempty_scoreboard_and_trace_required(self):
        coverage, trace = cvfpu.validate_simulation(self.successful_log())
        self.assertEqual(dict(checked=1, skipped=0, outputs=1), coverage)
        self.assertEqual(3, len(trace))
        for bad in (self.successful_log().replace("TEST PASSED", ""),
                    self.successful_log().replace("NAJA_CVFPU_COMPLETE", ""),
                    self.successful_log().replace("checked=1", "checked=0"),
                    self.successful_log().replace("checked=1", "checked=2"),
                    self.successful_log().replace("NAJA_CVFPU_OUTPUT", "OTHER_OUTPUT"),
                    self.successful_log().replace("NAJA_CVFPU_INPUT", "OTHER_INPUT"),
                    self.successful_log().replace("NAJA_CVFPU_STATE", "OTHER_STATE"),
                    self.successful_log() + "\nUVM_ERROR source(2) [MISMATCH] bad result",
                    self.successful_log() + "\n  UVM_ERROR source(2) [MISMATCH] bad result",
                    self.successful_log().replace("UVM_FATAL : 0", "UVM_FATAL : 1"),
                    self.successful_log() + "\n%Error: simulation failed"):
            with self.subTest(log=bad), self.assertRaises(ValueError):
                cvfpu.validate_simulation(bad)

    def test_comparison_preserves_data_flags_inputs_handshakes_and_timing(self):
        baseline = cvfpu.validate_simulation(self.successful_log())
        cvfpu.compare_results(baseline, baseline)
        for old, new in (("time=100", "time=101"), ("result=1234", "result=1235"),
                         ("status=00", "status=01"), ("operands=00", "operands=01"),
                         ("ir=1", "ir=0"), ("skipped=0", "skipped=1")):
            changed = cvfpu.validate_simulation(self.successful_log().replace(old, new))
            with self.subTest(field=old), self.assertRaises(ValueError):
                cvfpu.compare_results(baseline, changed)

    def test_shards_cover_matrix_once_for_smoke_and_full(self):
        for seeds, expected in ((2, 12), (50, 300)):
            shards = [cvfpu.select_cases(cvfpu.TESTS, seeds, index, 4) for index in range(4)]
            cases = [case for shard in shards for case in shard]
            self.assertEqual(expected, len(cases))
            self.assertEqual(expected, len(set(cases)))
            self.assertTrue(all(len(shard) == expected // 4 for shard in shards))
            self.assertEqual({(test, seed) for test in cvfpu.TESTS for seed in range(1, seeds + 1)},
                             set(cases))

    def test_missing_or_invalid_build_replaces_stale_success_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for manifest in (None, {}, dict(status='built', benchmark_revision='wrong')):
                if manifest is not None:
                    (root / 'build-summary.json').write_text(json.dumps(manifest))
                (root / 'summary.json').write_text('{"status": "passed"}')
                result = subprocess.run([sys.executable, cvfpu.__file__, '--phase', 'run',
                                         '--output', str(root)], capture_output=True, text=True)
                self.assertNotEqual(0, result.returncode)
                report = json.loads((root / 'summary.json').read_text())
                self.assertEqual('failed', report['status'])
                self.assertEqual([], report['cases'])

    def test_parallel_pairs_isolate_files_and_preserve_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            binaries = {}
            for name in ('original', 'generated'):
                binary = root / name
                # Exclusive file creation detects accidental cwd sharing between seeds.
                binary.write_text(f'#!{sys.executable}\nfrom pathlib import Path\n'
                                  f'Path({name!r} + ".scratch").open("x").close()\n'
                                  f'print({self.successful_log()!r})\n')
                binary.chmod(0o755)
                binaries[name] = binary
            with ThreadPoolExecutor(max_workers=2) as pool:
                cases = list(pool.map(lambda seed: cvfpu.run_case('test', seed, binaries,
                                      root, 10, os.environ.copy()), (1, 2)))
            self.assertTrue(all(case['status'] == 'passed' for case in cases))
            self.assertNotEqual(cases[0]['results']['original']['cwd'],
                                cases[1]['results']['original']['cwd'])
            binaries['generated'].write_text(f'#!{sys.executable}\nraise SystemExit(1)\n')
            failure = cvfpu.run_case('test', 3, binaries, root, 10, os.environ.copy())
            self.assertEqual('failed', failure['status'])
            self.assertEqual('passed', failure['results']['original']['status'])
            self.assertEqual('failed', failure['results']['generated']['status'])


if __name__ == "__main__":
    unittest.main()
