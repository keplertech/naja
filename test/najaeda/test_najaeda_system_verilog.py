# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import faulthandler
from pathlib import Path

from najaeda import netlist, naja

systemverilog_benchmarks = os.environ.get("SYSTEMVERILOG_BENCHMARKS_PATH")
if not systemverilog_benchmarks:
    verilog_benchmarks = os.environ.get("VERILOG_BENCHMARKS_PATH")
    if verilog_benchmarks:
        systemverilog_benchmarks = str(
            Path(verilog_benchmarks).parent.parent / "systemverilog" / "benchmarks")
najaeda_test_path = os.environ.get("NAJAEDA_TEST_PATH")
if not najaeda_test_path:
    najaeda_test_path = os.getcwd()


class NajaEDASystemVerilogTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_word_level_operator_predicates(self):
        source = """module ops(
            input logic [3:0] a, b, input logic [1:0] n, input logic s,
            output logic [3:0] sum, diff, prod, shl, shr, sra, m, q, r);
          assign sum = a + b; assign diff = a - b; assign prod = a * b;
          assign shl = a << n; assign shr = a >> n;
          assign sra = $signed(a) >>> n;
          assign m = s ? a : b; assign q = a / b; assign r = a % b;
        endmodule
        """
        raw_methods = ("isAddSub", "isSubtract", "isMultiply", "isShift",
                       "isDivMod", "isShiftRight", "isShiftArithmetic")
        high_methods = ("is_add", "is_sub", "is_mul", "is_shift", "is_divmod")
        expected = {
            "sum": {"isAddSub"}, "diff": {"isAddSub", "isSubtract"},
            "prod": {"isMultiply"}, "shl": {"isShift"},
            "shr": {"isShift", "isShiftRight"},
            "sra": {"isShift", "isShiftRight", "isShiftArithmetic"},
            "q": {"isDivMod"}, "r": {"isDivMod"}, "m": set(),
        }

        def check(model, positives=()):
            for method in raw_methods:
                with self.subTest(model=model.getName(), method=method):
                    self.assertIs(getattr(model, method)(), method in positives)

        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
            path = Path(directory) / "operators.sv"
            path.write_text(source, encoding="utf-8")
            top = netlist.load_system_verilog(str(path))
            raw_top = naja.NLUniverse.get().getTopDesign()
            check(raw_top)
            for method in high_methods:
                self.assertIs(getattr(top, method)(), False)

            instances = list(raw_top.getInstances())
            checked = set()
            for output, positives in expected.items():
                output_net = raw_top.getTerm(output).getBusTermBit(0).getNet()
                drivers = [instance for instance in instances
                           if any(term.getDirection() == naja.SNLTerm.Direction.Output
                                  and term.getNet() == output_net
                                  for term in instance.getInstTerms())]
                self.assertEqual(1, len(drivers), output)
                instance = drivers[0]
                model = instance.getModel()
                check(model, positives)
                checked.add(instance.getID())
                child = next(child for child in top.get_child_instances()
                             if child.pathIDs[-1] == instance.getID())
                high_positive = {
                    "sum": "is_add", "diff": "is_sub", "prod": "is_mul",
                    "shl": "is_shift", "shr": "is_shift", "sra": "is_shift",
                    "q": "is_divmod", "r": "is_divmod",
                }.get(output)
                for method in high_methods:
                    self.assertIs(getattr(child, method)(), method == high_positive)
            for instance in instances:
                if instance.getID() not in checked:
                    check(instance.getModel())

        # Exercise unrelated DB0 models and a blackbox loaded through the frontend.
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
            path = Path(directory) / "unrelated.sv"
            path.write_text(
                "module other(input a, b, output y, z);"
                "assign y = a; assign z = a & b;"
                "missing u(.a(a)); endmodule", encoding="utf-8")
            netlist.reset()
            netlist.load_system_verilog(str(path),
                config=netlist.SystemVerilogConfig(blackbox_unknown_modules=True))
            models = [instance.getModel() for instance in
                      naja.NLUniverse.get().getTopDesign().getInstances()]
            self.assertTrue(any(model.isAssign() for model in models))
            self.assertTrue(any(model.isAnd() for model in models))
            self.assertTrue(any(model.isBlackBox() for model in models))
            for model in models:
                check(model)

    def test_source_roles_and_model_definitions(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
            path = os.path.join(directory, "source.sv")
            with open(path, "w", encoding="utf-8") as output:
                output.write("module leaf(input logic a, b, output logic y);\n"
                             "  assign y = a & b;\nendmodule\n"
                             "module top(input logic a, b, output logic y);\n"
                             "  leaf u(.a(a), .b(b), .y(y));\nendmodule\n")
            for keep_ast_link in (False, True):
                netlist.reset()
                top = netlist.load_system_verilog(path, config=netlist.SystemVerilogConfig(
                    keep_ast_link=keep_ast_link))
                self.assertEqual(4, top.get_source_declaration().range.line)
                instance = top.get_child_instance("u")
                self.assertEqual(5, instance.get_source_declaration().range.line)
                self.assertEqual(1, instance.get_model_source_declaration().range.line)
                self.assertEqual([], instance.get_source_origins())
                self.assertEqual("slang", instance.get_source_declaration().provider)
                self.assertEqual("systemverilog", instance.get_source_declaration().language)
                self.assertEqual("rtl", instance.get_source_declaration().representation)
                self.assertEqual(1, instance.get_term("a").get_source_declaration().range.line)
                model = naja.NLUniverse.get().getTopDesign().getInstance("u").getModel()
                gates = list(model.getInstances())
                self.assertTrue(gates)
                for gate in gates:
                    self.assertIsNone(gate.getSourceDeclaration())
                    self.assertTrue(gate.getSourceOrigins())
                    self.assertTrue(any(origin[0][1] == 2 for origin in gate.getSourceOrigins()))
                net_declaration = top.get_net("a").get_source_declaration()
                term_declaration = top.get_term("a").get_source_declaration()
                model_declaration = instance.get_model_source_declaration()
                gate_origins = {gate.getName(): gate.getSourceOrigins() for gate in gates}
                snapshot = os.path.join(directory, "source.snl")
                netlist.dump_naja_if(snapshot)
                netlist.reset()
                netlist.load_naja_if(snapshot)
                loaded = netlist.get_top().get_child_instance("u")
                self.assertEqual("leaf", loaded.get_model_name())
                self.assertEqual(5, loaded.get_source_declaration().range.line)
                self.assertEqual(1, loaded.get_model_source_declaration().range.line)

                top = netlist.get_top()
                self.assertEqual(net_declaration, top.get_net("a").get_source_declaration())
                self.assertEqual(term_declaration, top.get_term("a").get_source_declaration())
                self.assertEqual(model_declaration, loaded.get_model_source_declaration())
                model = naja.NLUniverse.get().getTopDesign().getInstance("u").getModel()
                for gate in model.getInstances():
                    self.assertIsNone(gate.getSourceDeclaration())
                    self.assertEqual(gate_origins[gate.getName()], gate.getSourceOrigins())

                # Verilog exports legacy attributes, not the new source roles.
                # Reloading records declarations in the exported structural input.
                exported = os.path.join(directory, "source.v")
                for dump_metadata in (False, True):
                    top.dump_gate_level_verilog(exported, config=netlist.GateLevelVerilogDumpConfig(
                        dumpRTLInfosAsAttributes=dump_metadata))
                    netlist.reset()
                    reloaded = netlist.load_gate_level_verilog(exported)
                    for declaration in (
                            reloaded.get_source_declaration(),
                            reloaded.get_net("a").get_source_declaration(),
                            reloaded.get_term("a").get_source_declaration(),
                            reloaded.get_child_instance("u").get_source_declaration(),
                            reloaded.get_child_instance("u").get_model_source_declaration()):
                        self.assertEqual(exported, declaration.range.file)
                        self.assertEqual("naja-verilog", declaration.provider)
                        self.assertEqual("verilog", declaration.language)
                        self.assertIsNone(declaration.representation)
                    model = naja.NLUniverse.get().getTopDesign().getInstance("u").getModel()
                    for gate in model.getInstances():
                        self.assertEqual([], gate.getSourceOrigins())
                    netlist.reset()
                    netlist.load_naja_if(snapshot)
                    top = netlist.get_top()

    def test_unsupported_multi_writer_memory_blackboxing(self):
        # Exercise both Python API levels, independent clocks and a shared clock.
        for raw in (False, True):
            for clock_b in ("clk_b", "clk_a"):
                for mode in ("strict", "fallback", "define"):
                    with self.subTest(raw=raw, clock_b=clock_b, mode=mode):
                        netlist.reset()
                        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
                            source = os.path.join(directory, "dp_ram.sv")
                            report = os.path.join(directory, "diagnostics.log")
                            with open(source, "w") as stream:
                                stream.write("""module dp_ram(input logic clk_a, clk_b, we_a, we_b,
              input logic [2:0] addr_a, addr_b,
              input logic [7:0] wd_a, wd_b,
              output logic [7:0] rd_a, rd_b);
`ifndef SYNTHESIS_MEMORY_BLACK_BOXING
  logic [7:0] mem [8];
  always @(posedge clk_a) begin
    if (we_a) mem[addr_a] = wd_a;
    rd_a <= mem[addr_a];
  end
  always @(posedge CLOCK_B) begin
    if (we_b) mem[addr_b] = wd_b;
    rd_b <= mem[addr_b];
  end
`endif
endmodule
module top(input logic clk_a, clk_b, we_a, we_b,
           input logic [2:0] addr_a, addr_b,
           input logic [7:0] wd_a, wd_b,
           output logic [7:0] rd_a, rd_b, output logic alive);
  dp_ram ram(.*);
  assign alive = we_a & we_b;
endmodule
""".replace("CLOCK_B", clock_b))
                            options = dict(
                                diagnostics_report_path=report,
                                blackbox_multi_writer_memories=(mode == "fallback"),
                                defines=(["SYNTHESIS_MEMORY_BLACK_BOXING"]
                                         if mode == "define" else []))
                            if raw:
                                db = naja.NLDB.create(naja.NLUniverse.create())
                                load = lambda: db.loadSystemVerilog([source], **options)
                            else:
                                load = lambda: netlist.load_system_verilog(
                                    source, netlist.SystemVerilogConfig(**options))
                            if mode == "strict":
                                with self.assertRaisesRegex(
                                        RuntimeError, "multiple sequential writers"):
                                    load()
                                continue
                            load()
                            top = naja.NLUniverse.get().getTopDesign()
                            self.assertFalse(top.isBlackBox())
                            ram = top.getInstance("ram")
                            self.assertIsNotNone(ram)
                            model = ram.getModel()
                            self.assertTrue(model.isBlackBox())
                            self.assertEqual(0, len(list(model.getInstances())))
                            self.assertEqual(0 if mode == "fallback" else 10,
                                             len(list(model.getNets())))
                            self.assertEqual(10, len(list(model.getTerms())))
                            for name, width in (("addr_a", 3), ("addr_b", 3),
                                                ("wd_a", 8), ("wd_b", 8),
                                                ("rd_a", 8), ("rd_b", 8)):
                                term = model.getBusTerm(name)
                                self.assertEqual(width, term.getWidth())
                                self.assertEqual(naja.SNLTerm.Direction.Output
                                                 if name.startswith("rd")
                                                 else naja.SNLTerm.Direction.Input,
                                                 term.getDirection())
                            for term in ram.getInstTerms():
                                self.assertIsNotNone(term.getNet())
                            self.assertGreater(len(list(top.getInstances())), 1)
                            with open(report) as stream:
                                diagnostics = stream.read()
                            if mode == "fallback":
                                self.assertIn("multi_writer_memory_blackbox", diagnostics)
                                self.assertIn("Memory 'mem'", diagnostics)
                                self.assertIn("blackboxing entire module 'dp_ram'", diagnostics)
                            else:
                                self.assertNotIn("multi_writer_memory_blackbox", diagnostics)

    def test_multi_clock_memory_inference_and_snapshot(self):
        for raw in (False, True):
            for clock_b in ("clk_b", "clk_a"):
                with self.subTest(raw=raw, clock_b=clock_b):
                    netlist.reset()
                    with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
                        source = os.path.join(directory, "dp_ram.sv")
                        with open(source, "w") as stream:
                            stream.write("""module dp_ram(input logic clk_a, clk_b, we_a, we_b,
              input logic [2:0] addr_a, addr_b,
              input logic [7:0] wd_a, wd_b,
              output logic [7:0] rd_a, rd_b);
  logic [7:0] mem [8];
  always @(posedge clk_a) begin
    if (we_a) mem[addr_a] <= wd_a;
    rd_a <= mem[addr_a];
  end
  always @(posedge CLOCK_B) begin
    if (we_b) mem[addr_b] <= wd_b;
    rd_b <= mem[addr_b];
  end
endmodule
""".replace("CLOCK_B", clock_b))
                        report = os.path.join(directory, "diagnostics.log")
                        if raw:
                            db = naja.NLDB.create(naja.NLUniverse.create())
                            db.loadSystemVerilog([source], diagnostics_report_path=report)
                        else:
                            netlist.load_system_verilog(source, netlist.SystemVerilogConfig(
                                diagnostics_report_path=report))
                        top = naja.NLUniverse.get().getTopDesign()
                        self.assertFalse(top.isBlackBox())
                        snapshot = os.path.join(directory, "snapshot")
                        for restored in (False, True):
                            if restored:
                                top.getLibrary().getDB().dumpNajaIF(snapshot)
                                netlist.reset()
                                naja.NLDB.loadNajaIF(snapshot)
                                top = naja.NLUniverse.get().getTopDesign()
                            memories = [inst for inst in top.getInstances()
                                        if inst.getModel().getName().startswith("naja_mem__")]
                            self.assertEqual(1, len(memories))
                            memory = memories[0]
                            model = memory.getModel()
                            self.assertEqual("1", model.getParameter("MULTI_CLOCK").getValue())
                            self.assertEqual(2, model.getBusTerm("WCLK").getWidth())
                            self.assertEqual(16, model.getBusTerm("WMASK").getWidth())
                            for port in range(2):
                                clock = model.getBusTerm("WCLK").getBusTermBit(port)
                                self.assertTrue(clock.isClock())
                                self.assertEqual(20, len(list(
                                    naja.SNLDesign.getClockRelatedInputs(clock))))
                                self.assertEqual(16, len(list(
                                    naja.SNLDesign.getClockRelatedOutputs(clock))))
                                for bit in range(8):
                                    mask = model.getBusTerm("WMASK").getBusTermBit(port * 8 + bit)
                                    self.assertEqual(naja.SNLTermRole.MemoryWriteEnable,
                                                     mask.getRole())
                            for term in memory.getInstTerms():
                                self.assertIsNotNone(term.getNet())
                            self.assertIsNone(top.getNet("mem"))
                        with open(report) as stream:
                            diagnostics = stream.read()
                        self.assertIn("multi_clock_memory_collision", diagnostics)
                        self.assertNotIn("multi_writer_memory_blackbox", diagnostics)

    def test_multi_writer_memory_option_validation(self):
        self.assertFalse(netlist.SystemVerilogConfig().blackbox_multi_writer_memories)
        with self.assertRaisesRegex(TypeError, "blackbox_multi_writer_memories"):
            netlist.SystemVerilogConfig(blackbox_multi_writer_memories="true")

    def test_memory_sync_reset_role(self):
        for condition, level in (("rst", naja.SNLActiveLevel.High),
                                 ("~rst", naja.SNLActiveLevel.Low)):
            with self.subTest(condition=condition):
                netlist.reset()
                with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
                    source = os.path.join(temp_dir, "memory.sv")
                    with open(source, "w") as stream:
                        stream.write("""module memory(input logic clk, rst,
              input logic [1:0] addr, input logic [7:0] data,
              output logic [7:0] q);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  always_comb begin
    mem_d = mem_q;
    mem_d[addr] = data;
  end
  always_ff @(posedge clk) begin
    if (CONDITION) mem_q <= '{8'h10, 8'h21, 8'h32, 8'h43};
    else mem_q <= mem_d;
  end
  assign q = mem_q[addr];
endmodule
""".replace("CONDITION", condition))
                    netlist.load_system_verilog(source)
                    top = naja.NLUniverse.get().getTopDesign()
                    memories = [inst.getModel() for inst in top.getInstances()
                                if inst.getModel().getName().startswith("naja_mem__")]
                    self.assertEqual(1, len(memories))
                    reset = memories[0].getScalarTerm("RST")
                    self.assertIsNotNone(reset)
                    self.assertEqual(naja.SNLTermRole.SyncReset, reset.getRole())
                    self.assertTrue(reset.isReset())
                    self.assertTrue(reset.isSyncReset())
                    self.assertFalse(reset.isAsyncReset())
                    self.assertEqual(level, reset.getResetActiveLevel())

    def test_system_verilog_config_diagnostics_default(self):
        config = netlist.SystemVerilogConfig()
        self.assertEqual(
            "naja_sv_diagnostics.log", config.diagnostics_report_path)

    def test_system_verilog_default_timescale(self):
        self.assertIsNone(netlist.SystemVerilogConfig().default_timescale)
        for value in (1, False, []):
            with self.assertRaisesRegex(TypeError, "default_timescale must be a str or None"):
                netlist.SystemVerilogConfig(default_timescale=value)
        for value in ("", "  "):
            with self.assertRaisesRegex(ValueError, "default_timescale must not be empty"):
                netlist.SystemVerilogConfig(default_timescale=value)
        config = netlist.SystemVerilogConfig()
        config.default_timescale = 1
        with self.assertRaisesRegex(TypeError, "default_timescale must be a str or None"):
            netlist.load_system_verilog([], config)
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "source.sv")
            flist = os.path.join(directory, "sources.f")
            with open(path, "w", encoding="utf-8") as source:
                source.write("module leaf(output wire y);\n"
                             "localparam integer UNIT = 1ns; assign y = (UNIT == 1000);\n"
                             "endmodule\n"
                             "module top(output wire a, b); timeunit 1ns/1ps;\n"
                             "localparam integer UNIT = 1ns; assign a = (UNIT == 1);\n"
                             "leaf u(b); endmodule\n")
            with open(flist, "w", encoding="utf-8") as output:
                output.write(f'"{Path(path).as_posix()}"\n')
            with self.assertRaisesRegex(naja.SystemVerilogSyntaxError, "time scale"):
                netlist.load_system_verilog(path)
            for use_flist in (False, True):
                netlist.reset()
                top = netlist.load_system_verilog([] if use_flist else path,
                    netlist.SystemVerilogConfig(default_timescale="1ps/1ps", keep_assigns=False,
                        flist=flist if use_flist else None, top="top"))
                self.assertTrue(top.get_term("a").get_lower_net().is_const1())
                self.assertTrue(top.get_child_instance("u").get_term("y").get_lower_net().is_const1())
            netlist.reset()
            with self.assertRaises(naja.SystemVerilogSyntaxError):
                netlist.load_system_verilog(path,
                    netlist.SystemVerilogConfig(default_timescale="invalid"))

    def test_system_verilog_config_single_unit_validation(self):
        self.assertFalse(netlist.SystemVerilogConfig().single_unit)
        for value in ("true", 1, None):
            with self.subTest(value=value):
                with self.assertRaisesRegex(
                        TypeError, r"SystemVerilogConfig\.single_unit must be a bool"):
                    netlist.SystemVerilogConfig(single_unit=value)
        config = netlist.SystemVerilogConfig()
        config.single_unit = "true"
        with self.assertRaisesRegex(
                TypeError, r"SystemVerilogConfig\.single_unit must be a bool"):
            netlist.load_system_verilog([], config)

    def test_load_system_verilog_cross_file_macros(self):
        with tempfile.TemporaryDirectory(prefix="sv inputs ") as directory:
            defs = os.path.join(directory, "defs.sv")
            use = os.path.join(directory, "use.sv")
            flist_path = os.path.join(directory, "sources.f")
            with open(defs, "w", encoding="utf-8") as source:
                source.write("`define CHECK(x) assign x = 1'b1;\n")
            with open(use, "w", encoding="utf-8") as source:
                source.write("module macro_top(output logic o); `CHECK(o) endmodule\n")
            with open(flist_path, "w", encoding="utf-8") as flist:
                flist.write(f'"{Path(defs).as_posix()}"\n"{Path(use).as_posix()}"\n')
            for use_flist in (False, True):
                for single_unit in (False, True):
                    for top_name in (None, "macro_top"):
                        with self.subTest(flist=use_flist, single_unit=single_unit,
                                          top=top_name):
                            netlist.reset()
                            config = netlist.SystemVerilogConfig(
                                single_unit=single_unit, top=top_name,
                                flist=flist_path if use_flist else None,
                                diagnostics_report_path=None,
                                keep_assigns=False)
                            files = [] if use_flist else [defs, use]
                            if single_unit:
                                top = netlist.load_system_verilog(files, config)
                                self.assertEqual("macro_top", top.get_model_name())
                                self.assertTrue(top.get_term("o").get_lower_net().is_const1())
                            else:
                                with self.assertRaisesRegex(
                                        naja.SystemVerilogSyntaxError,
                                        "unknown macro or compiler directive '`CHECK'"):
                                    netlist.load_system_verilog(files, config)

    def test_load_system_verilog(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())
        self.assertEqual(3, top.count_terms())
        self.assertEqual(2, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())
        source_range = top.get_source_range()
        self.assertIsInstance(source_range, netlist.SourceRange)
        self.assertTrue(Path(source_range.file).as_posix().endswith(
            "systemverilog/benchmarks/simple/simple.sv"))
        self.assertGreaterEqual(source_range.line, 1)
        self.assertGreaterEqual(source_range.column, 1)
        self.assertGreaterEqual(source_range.end_line, source_range.line)
        self.assertGreaterEqual(source_range.end_column, 1)

        term_range = top.get_term("a").get_source_range()
        self.assertIsInstance(term_range, netlist.SourceRange)
        self.assertTrue(Path(term_range.file).as_posix().endswith(
            "systemverilog/benchmarks/simple/simple.sv"))

        net_range = top.get_net("y").get_source_range()
        self.assertIsInstance(net_range, netlist.SourceRange)
        self.assertTrue(Path(net_range.file).as_posix().endswith(
            "systemverilog/benchmarks/simple/simple.sv"))

    def test_source_columns_above_16_bits(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
            path = os.path.join(directory, "wide.sv")
            source = " " * 70000 + "module top(input a, output y); assign y = a; endmodule"
            with open(path, "w", encoding="utf-8") as output:
                output.write(source + "\n")
            for keep_ast_link in (False, True):
                with self.subTest(keep_ast_link=keep_ast_link):
                    netlist.reset()
                    top = netlist.load_system_verilog(path, config=netlist.SystemVerilogConfig(
                        keep_ast_link=keep_ast_link))
                    source_range = top.get_source_range()
                    self.assertEqual(70001, source_range.column)
                    self.assertEqual(len(source), source_range.end_column)
                    self.assertEqual(source_range.column,
                                     naja.NLUniverse.get().getTopDesign().getSourceLoc()[2])
                    self.assertGreater(top.get_term("a").get_source_range().column, 65535)
                    self.assertGreater(top.get_net("y").get_source_range().column, 65535)
                    dumped = os.path.join(directory, "wide_export.v")
                    top.dump_gate_level_verilog(dumped, config=netlist.GateLevelVerilogDumpConfig(
                        dumpRTLInfosAsAttributes=True))
                    with open(dumped, encoding="utf-8") as exported:
                        self.assertIn("70001", exported.read())
                    snapshot = os.path.join(directory, "snapshot")
                    netlist.dump_naja_if(snapshot)
                    netlist.reset()
                    netlist.load_naja_if(snapshot)
                    self.assertEqual(source_range, netlist.get_top().get_source_range())

    def test_load_system_verilog_single_arg(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        top = netlist.load_system_verilog(design_file)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_with_ast_link_option(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        top = netlist.load_system_verilog(
            design_file,
            config=netlist.SystemVerilogConfig(keep_ast_link=True),
        )
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_source_range_absent(self):
        top = netlist.create_top("top")
        self.assertIsNone(top.get_source_range())

        concat_net = netlist.Net([], net_concat=[])
        self.assertIsNone(concat_net.get_source_range())

    def test_load_system_verilog_with_ast_json_dump(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        json_path = os.path.join(najaeda_test_path, "simple_elaborated_ast_najaeda.json")
        diagnostics_path = os.path.join(najaeda_test_path, "simple_diagnostics_najaeda.txt")
        top = netlist.load_system_verilog(
            design_files,
            config=netlist.SystemVerilogConfig(
                elaborated_ast_json_path=json_path,
                diagnostics_report_path=diagnostics_path),
        )
        self.assertIsNotNone(top)
        self.assertTrue(os.path.exists(json_path))
        self.assertTrue(os.path.exists(diagnostics_path))

    def test_dump_gate_level_verilog_with_config(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)

        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as dump_dir:
            without_rtl_infos = os.path.join(dump_dir, "simple_default_no_rtl_infos.v")
            top.dump_gate_level_verilog(without_rtl_infos)
            with open(without_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertNotIn("sv_src_file", dumped_text)

            with_rtl_infos = os.path.join(dump_dir, "simple_with_rtl_infos.v")
            top.dump_gate_level_verilog(
                with_rtl_infos,
                config=netlist.GateLevelVerilogDumpConfig(dumpRTLInfosAsAttributes=True),
            )
            with open(with_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertNotIn("sv_src_file", dumped_text)
            self.assertIn('naja_sv_src="', dumped_text)

            with_verbose_rtl_infos = os.path.join(
                dump_dir, "simple_with_verbose_rtl_infos.v"
            )
            top.dump_gate_level_verilog(
                with_verbose_rtl_infos,
                config=netlist.GateLevelVerilogDumpConfig(
                    dumpRTLInfosAsAttributes=True,
                    rtlInfoDumpMode="VerboseAttributes",
                ),
            )
            with open(with_verbose_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertIn("sv_src_file", dumped_text)
            self.assertNotIn('naja_sv_src="', dumped_text)

            with_assign_instances = os.path.join(dump_dir, "simple_with_assign_instances.v")
            top.dump_gate_level_verilog(
                with_assign_instances,
                config=netlist.GateLevelVerilogDumpConfig(dumpAssignsAsInstances=True),
            )
            with open(with_assign_instances, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertIn("assign_module", dumped_text)

    def test_dump_gate_level_verilog_split_packed_signals(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
            source = os.path.join(directory, "packed.sv")
            with open(source, "w", encoding="utf-8") as stream:
                stream.write("module child(input [3:0] a, output [3:0] y); "
                             "assign y = a; endmodule\n"
                             "module top(input [3:0] a, output [3:0] y); "
                             "child i(a,y); endmodule\n")
            top = netlist.load_system_verilog([source], config=netlist.SystemVerilogConfig(top="top"))
            path = os.path.join(directory, "split.v")
            top.dump_gate_level_verilog(path, config=netlist.GateLevelVerilogDumpConfig(verilatorSplitPackedSignals=True))
            with open(path, encoding="utf-8") as stream:
                text = stream.read()
            child_text, top_text = text.split("module top(")
            self.assertIn("split_var", child_text)
            self.assertNotIn("split_var", top_text.split(");", 1)[0])

    def test_dump_gate_level_verilog_config_rejects_invalid_rtl_info_mode(self):
        with self.assertRaises(ValueError) as context:
            netlist.GateLevelVerilogDumpConfig(rtlInfoDumpMode="Invalid")
        self.assertIn("Invalid rtlInfoDumpMode", str(context.exception))

    def test_load_system_verilog_with_flist(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        flist_path = os.path.join(najaeda_test_path, "simple_najaeda.f")
        with open(flist_path, "w", encoding="utf-8") as flist:
            flist.write(f'"{Path(design_file).as_posix()}"\n')
        top = netlist.load_system_verilog(
            [],
            config=netlist.SystemVerilogConfig(flist=flist_path),
        )
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_alias(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_with_top(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            generic_sv = os.path.join(temp_dir, "generic.sv")
            with open(generic_sv, "w", encoding="utf-8") as generic_file:
                generic_file.write(
                    "module generic #(parameter type T = logic) "
                    "(input T i, output logic y);\n"
                    "  assign y = i.foo;\n"
                    "endmodule\n")

            top_sv = os.path.join(temp_dir, "top2.sv")
            with open(top_sv, "w", encoding="utf-8") as top_file:
                top_file.write(
                    "module top2(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n")

            top = netlist.load_system_verilog(
                [generic_sv, top_sv],
                config=netlist.SystemVerilogConfig(top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())

    def test_load_system_verilog_with_non_string_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.top must be a str or None \(got int\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=123),
            )

    def test_load_system_verilog_with_empty_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                ValueError,
                r"SystemVerilogConfig\.top must not be empty"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=""),
            )

    def test_load_system_verilog_with_invalid_ast_link_option_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.keep_ast_link must be a bool \(got str\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(keep_ast_link="true"),
            )

    def test_load_system_verilog_with_invalid_blackbox_unknown_modules_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.blackbox_unknown_modules "
                r"must be a bool \(got str\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(blackbox_unknown_modules="true"),
            )

    def test_load_system_verilog_with_invalid_defines_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        cases = [
            (
                "SYNTHESIS",
                TypeError,
                r"SystemVerilogConfig\.defines must be a list\[str\] or None "
                r"\(got str\)",
            ),
            (
                [1],
                TypeError,
                r"SystemVerilogConfig\.defines\[0\] must be a str \(got int\)",
            ),
            (
                [""],
                ValueError,
                r"SystemVerilogConfig\.defines\[0\] must not be empty",
            ),
            (
                ["HAS SPACE"],
                ValueError,
                r"SystemVerilogConfig\.defines\[0\] must not contain whitespace",
            ),
        ]
        for defines, exception_type, message in cases:
            with self.subTest(defines=defines):
                with self.assertRaisesRegex(exception_type, message):
                    netlist.load_system_verilog(
                        design_files,
                        config=netlist.SystemVerilogConfig(defines=defines),
                    )

    def test_load_system_verilog_configuration_errors_are_actionable(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        cases = [
            (
                {"diagnostics_report_path": ""},
                ValueError,
                r"diagnostics_report_path must not be empty",
            ),
            (
                {"suppress_warnings": [1]},
                TypeError,
                r"suppress_warnings\[0\] must be a str \(got int\)",
            ),
            (
                {"suppress_warnings": ["-Wno-width-trunc"]},
                ValueError,
                r"without a -W/-Wno- prefix; got '-Wno-width-trunc'",
            ),
        ]
        for kwargs, exception_type, message in cases:
            with self.subTest(kwargs=kwargs):
                with self.assertRaisesRegex(exception_type, message):
                    netlist.load_system_verilog(
                        design_files,
                        config=netlist.SystemVerilogConfig(**kwargs),
                    )

        missing_flist = os.path.join(
            najaeda_test_path, "missing-systemverilog-command-file.f")
        with self.assertRaises(FileNotFoundError) as context:
            netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(flist=missing_flist),
            )
        self.assertIn("SystemVerilogConfig.flist", str(context.exception))
        self.assertIn(repr(missing_flist), str(context.exception))

    def test_load_system_verilog_with_flist_top_and_define(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            design_sv = os.path.join(temp_dir, "define_selects_top.sv")
            with open(design_sv, "w", encoding="utf-8") as design_file:
                design_file.write(
                    "`ifdef SYNTHESIS\n"
                    "module synth_top(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n"
                    "`else\n"
                    "module sim_top(input logic a, output logic y);\n"
                    "  assign y = ~a;\n"
                    "endmodule\n"
                    "`endif\n")

            source_flist = os.path.join(temp_dir, "sources.f")
            with open(source_flist, "w", encoding="utf-8") as flist:
                flist.write(f'"{Path(design_sv).as_posix()}"\n')

            top = netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(
                    flist=source_flist,
                    top="synth_top",
                    defines=["SYNTHESIS"]),
            )
            self.assertIsNotNone(top)
            self.assertEqual("synth_top", top.get_model_name())

    def test_load_system_verilog_with_flist_and_top(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            generic_sv = os.path.join(temp_dir, "generic.sv")
            with open(generic_sv, "w", encoding="utf-8") as generic_file:
                generic_file.write(
                    "module generic #(parameter type T = logic) "
                    "(input T i, output logic y);\n"
                    "  assign y = i.foo;\n"
                    "endmodule\n")

            top_sv = os.path.join(temp_dir, "top2.sv")
            with open(top_sv, "w", encoding="utf-8") as top_file:
                top_file.write(
                    "module top2(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n")

            source_flist = os.path.join(temp_dir, "sources.f")
            with open(source_flist, "w", encoding="utf-8") as flist:
                flist.write(f'"{Path(generic_sv).as_posix()}"\n')
                flist.write(f'"{Path(top_sv).as_posix()}"\n')

            top = netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(
                    flist=source_flist,
                    top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())


if __name__ == "__main__":
    faulthandler.enable()
    unittest.main()
