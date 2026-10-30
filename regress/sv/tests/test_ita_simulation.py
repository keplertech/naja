# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
import threading

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import ita_simulation as ita


class ITASimulationTest(unittest.TestCase):
    def test_parallel_pipelines_overlap_and_keep_failures(self):
        barrier = threading.Barrier(2)
        def pipeline(name, flist):
            barrier.wait(timeout=5)
            return {name: {"status": "failed" if name == "generated" else "passed"}}
        results = dict(ita.run_pipelines(
            [("original", "rtl.f"), ("generated", "dump.f")], pipeline, True))
        self.assertEqual("passed", results["original"]["original"]["status"])
        self.assertEqual("failed", results["generated"]["generated"]["status"])

    def test_serial_pipelines_preserve_order(self):
        visited = []
        def pipeline(name, flist):
            visited.append(name)
            return flist
        self.assertEqual([("original", "rtl.f"), ("generated", "dump.f")],
                         list(ita.run_pipelines([("original", "rtl.f"), ("generated", "dump.f")], pipeline)))
        self.assertEqual(["original", "generated"], visited)

    def test_scoreboard_requires_all_outputs_and_no_mismatch(self):
        lines = [f'256 outputs were checked in phase {p}.'
                 for p in (0, 1, 2, 3, 3, 4, 5, 6)]
        trace = [f'NAJA_ITA_OUTPUT phase={phase} data=00 time={i}'
                 for phase in (0, 1, 2, 3, 3, 4, 5, 6) for i in range(256)]
        valid = '\n'.join(lines + trace + ['NAJA_ITA_COMPLETE'])
        ita.validate_simulation(valid)
        for bad in (valid.replace(lines[-1], ''), valid.replace('NAJA_ITA_COMPLETE', ''),
                    valid + '\nWrong value', valid.replace('256', '255', 1)):
            with self.subTest(log=bad), self.assertRaises(ValueError):
                ita.validate_simulation(bad)

    def test_memory_adapter_rejects_changed_shape_or_existing_behavior(self):
        declaration = ('module tc_sram_blackbox(input clk_i, input rst_ni, '
                       'input [1:0] req_i, input [1:0] we_i, input [15:0] addr_i, '
                       'input [831:0] wdata_i, input [103:0] be_i, '
                       'output [831:0] rdata_o);\nendmodule //tc_sram_blackbox')
        adapted = ita.attach_memory(declaration)
        self.assertIn('.NumWords(256)', adapted)
        for bad in (declaration * 2, declaration.replace('[831:0]', '[415:0]'),
                    declaration.replace('endmodule', "assign rdata_o = '0;\nendmodule")):
            with self.subTest(netlist=bad), self.assertRaises(ValueError):
                ita.attach_memory(bad)

    def test_read_delay_preserves_logic_and_rejects_unexpected_models(self):
        name = "ita_register_file_1w_multi_port_read_we"
        for shape in ("[8191:0]", "[N_READ-1:0][DATA_WIDTH-1:0]"):
            source = (f"module {name}(output logic {shape} ReadData);\n"
                      "assign ReadData = storage;\nendmodule")
            self.assertEqual(source, ita.model_weight_read_delay(source, 0))
            result = ita.model_weight_read_delay(source, 1)
            self.assertIn("assign #(1ps) ReadData = naja_sim_read_data;", result)
            self.assertIn("assign naja_sim_read_data = storage;", result)
            for bad in (source * 2, source.replace(shape, "[31:0]"), result):
                with self.subTest(source=bad), self.assertRaises(ValueError):
                    ita.model_weight_read_delay(bad, 1)
        for delay in (-1, 101):
            with self.assertRaises(ValueError):
                ita.model_weight_read_delay(source, delay)

    def test_download_cache_is_checksum_checked(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'download'
            path.write_bytes(b'pinned source')
            sha = hashlib.sha256(path.read_bytes()).hexdigest()
            self.assertEqual(path, ita.fetch('unused', path, sha))
            path.write_bytes(b'changed source')
            with self.assertRaisesRegex(ValueError, 'Checksum mismatch'):
                ita.fetch('unused', path, sha)

    def test_upstream_read_delay_is_replaced_by_explicit_setting(self):
        source = ("module ita_register_file_1w_multi_port_read_we(output logic [8191:0] ReadData);\n"
                  "assign\n`ifdef VERILATOR\n  #1ps\n`endif\n ReadData = storage;\nendmodule")
        for delay in (0, 1, 3):
            with self.subTest(delay=delay):
                result = ita.model_weight_read_delay(source, delay)
                self.assertNotIn("`ifdef VERILATOR", result)
                self.assertNotIn("#1ps", result)
                self.assertEqual(int(delay != 0), result.count("assign #("))
                if delay:
                    self.assertIn(f"assign #({delay}ps) ReadData = naja_sim_read_data;", result)

    def test_adapter_preserves_golden_checker_and_checks_scan_results(self):
        source = '\n'.join(['ret_code = $fscanf(fd, "%d", inp[i]);'] * 10)
        source += ('\n"../../simvectors/data_S"\n'
                   'if (requant_oup !== exp_res) begin\n'
                   '$display("[TB] ITA: Wrong value"); end\n$finish();')
        result = ita.adapt_testbench(source, Path('/tmp/vectors'))
        self.assertEqual(10, result.count('if (ret_code != 1) $fatal'))
        self.assertIn('if (requant_oup !== exp_res)', result)
        self.assertIn('Wrong value', result)
        self.assertIn('NAJA_ITA_OUTPUT', result)
        self.assertIn('NAJA_ITA_COMPLETE', result)
        with self.assertRaisesRegex(ValueError, 'scan layout'):
            ita.adapt_testbench(source.replace('ret_code', 'changed'), Path('/tmp'))


if __name__ == '__main__':
    unittest.main()
