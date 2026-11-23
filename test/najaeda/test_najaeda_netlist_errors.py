
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler
import os
import tempfile
import warnings
from pathlib import Path

from najaeda import netlist
from najaeda import naja

class NajaNetlistTestErrors(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_loader_progress_forwarding(self):
        sources = {
            "v": "module top(input a, output y); assign y=a; endmodule",
            "sv": "module top(input logic a, output logic y); assign y=a; endmodule",
            "vhd": ("entity top is port(a: in bit; y: out bit); end; "
                    "architecture rtl of top is begin y <= a; end;"),
            "lib": ('library(synthetic) {cell(buffer) {pin(A) {direction: input;} '
                    'pin(Y) {direction: output; function: "A";} }}'),
        }
        with tempfile.TemporaryDirectory() as directory:
            for suffix, text in sources.items():
                Path(directory, "source." + suffix).write_text(text)
            loaders = [
                ("v", netlist.load_gate_level_verilog),
                ("sv", netlist.load_system_verilog),
                ("vhd", netlist.load_vhdl),
                ("lib", netlist.load_liberty),
            ]
            for suffix, load in loaders:
                with self.subTest(loader=suffix):
                    netlist.reset()
                    stages = []
                    load(str(Path(directory, "source." + suffix)),
                         progress=lambda **event: stages.append(event["stage"]))
                    self.assertEqual("done", stages[-1])
                    netlist.reset()
                    with self.assertRaises(naja.NajaCancelled):
                        load(str(Path(directory, "source." + suffix)),
                             progress=lambda **event: False)
            netlist.reset()
            netlist.load_gate_level_verilog(str(Path(directory, "source.v")))
            snapshot = str(Path(directory, "snapshot"))
            netlist.dump_naja_if(snapshot)
            netlist.reset()
            stages = []
            netlist.load_naja_if(snapshot, progress=lambda **event: stages.append(event["stage"]))
            self.assertEqual("done", stages[-1])

    def test_gate_level_verilog_names(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "input.v"
            source.write_text("module top(input a, output y); assign y = a; endmodule\n")
            with warnings.catch_warnings(record=True) as caught:
                warnings.simplefilter("error", FutureWarning)
                config = netlist.GateLevelVerilogConfig(keep_assigns=False)
                dump_config = netlist.GateLevelVerilogDumpConfig(
                    dumpAssignsAsInstances=True)
                top = netlist.load_gate_level_verilog(str(source), config=config)
                self.assertEqual(top.get_name(), "top")
                top.dump_gate_level_verilog(
                    str(Path(directory) / "new.v"), config=dump_config)
            self.assertEqual(caught, [])

    def test_gate_level_verilog_validation(self):
        with self.assertRaisesRegex(TypeError, "GateLevelVerilogConfig.keep_assigns"):
            netlist.GateLevelVerilogConfig(keep_assigns=1)
        with self.assertRaisesRegex(TypeError, "load_gate_level_verilog config"):
            netlist.load_gate_level_verilog([], config="invalid")
        with self.assertRaises(ValueError):
            netlist.GateLevelVerilogDumpConfig(rtlInfoDumpMode="invalid")

    def test_missing_model(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        self.assertRaises(Exception, top.create_child_instance, 'Module0', 'mod')

    def test_missing_verilog(self):
        self.assertRaises(TypeError, netlist.load_gate_level_verilog)
        with self.assertRaisesRegex(
                ValueError, "No Verilog input files were provided"):
            netlist.load_gate_level_verilog([])
        self.assertRaises(TypeError, netlist.load_system_verilog)
        with self.assertRaisesRegex(
                ValueError,
                "pass at least one file or set SystemVerilogConfig.flist"):
            netlist.load_system_verilog([])

    def test_loader_input_errors_identify_argument_and_path(self):
        with self.assertRaisesRegex(
                TypeError, r"Verilog files\[1\].*got int"):
            netlist.load_gate_level_verilog(["valid.v", 3])

        missing = os.path.join(
            tempfile.gettempdir(), "najaeda-file-that-does-not-exist.v")
        with self.assertRaises(FileNotFoundError) as context:
            netlist.load_gate_level_verilog(missing)
        message = str(context.exception)
        self.assertIn(repr(missing), message)
        self.assertIn("files[0]", message)
        self.assertIn("resolved to", message)

        with self.assertRaisesRegex(
                TypeError, r"Liberty files must be a path string or list.*got int"):
            netlist.load_liberty(7)

    def test_verilog_config_validation_errors(self):
        cases = [
            (
                {"keep_assigns": 1},
                TypeError,
                r"GateLevelVerilogConfig\.keep_assigns must be a bool \(got int\)",
            ),
            (
                {"conflicting_design_name_policy": 1},
                TypeError,
                r"conflicting_design_name_policy must be a str \(got int\)",
            ),
            (
                {"conflicting_design_name_policy": "merge"},
                ValueError,
                r"Invalid conflicting_design_name_policy: 'merge'",
            ),
            (
                {"allow_unknown_designs": "true"},
                TypeError,
                r"allow_unknown_designs must be a bool or None \(got str\)",
            ),
        ]
        for kwargs, exception_type, message in cases:
            with self.subTest(kwargs=kwargs):
                with self.assertRaisesRegex(exception_type, message):
                    netlist.GateLevelVerilogConfig(**kwargs)

    def test_loader_config_and_path_validation_errors(self):
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.diagnostics_report_path "
                r"must be a path string or None \(got int\)"):
            netlist.SystemVerilogConfig(diagnostics_report_path=1)

        with self.assertRaisesRegex(
                ValueError, r"Verilog files\[0\] must not be empty"):
            netlist.load_gate_level_verilog([" "])

        with self.assertRaisesRegex(
                TypeError,
                r"load_gate_level_verilog config must be a GateLevelVerilogConfig or None \(got str\)"):
            netlist.load_gate_level_verilog([], config="invalid")

        with self.assertRaisesRegex(
                TypeError,
                r"load_system_verilog config must be a "
                r"SystemVerilogConfig or None \(got str\)"):
            netlist.load_system_verilog([], config="invalid")

        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(
                    ValueError, r"Verilog input path is not a file"):
                netlist.load_gate_level_verilog(directory)

            config = netlist.SystemVerilogConfig(flist=directory)
            with self.assertRaisesRegex(
                    ValueError, r"SystemVerilogConfig\.flist is not a file"):
                netlist.load_system_verilog([], config=config)

    def test_width_mismatch(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        topTerm = top.create_input_term('Top')
        topNet = top.create_bus_net('net', 1, 0)
        self.assertRaises(Exception, topTerm.connect_upper_net, topNet)

    def test_empty_liberty(self):
        with self.assertRaisesRegex(
                ValueError, "No Liberty input files were provided"):
            netlist.load_liberty([])
            
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
