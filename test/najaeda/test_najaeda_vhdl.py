# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import tempfile
import unittest

from najaeda import netlist


class NajaEDAVHDLTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_load_vhdl(self):
        source = """\
entity inverter is port(a : in bit; y : out bit); end;
architecture rtl of inverter is begin y <= not a; end;
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "inverter.vhd"
            path.write_text(source, encoding="utf-8")
            with self.assertWarnsRegex(RuntimeWarning, "VHDL parser is in Beta mode"):
                top = netlist.load_vhdl(path)

        self.assertEqual("inverter", top.get_model_name())
        self.assertEqual(2, top.count_terms())
        self.assertEqual(1, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())

    def test_source_references_across_libraries_and_snapshot(self):
        leaf_source = """\
-- File-local coordinates, independent of library concatenation.
library ieee; use ieee.std_logic_1164.all;
entity leaf is generic(n : positive);
  port(clk, a : in std_logic; y : out std_logic_vector(n-1 downto 0));
end;
library ieee; use ieee.std_logic_1164.all;
architecture rtl of leaf is begin
  lanes: for i in 0 to n-1 generate
    process(clk) begin
      if rising_edge(clk) then y(i) <= not a; end if;
    end process;
  end generate;
end;
"""
        top_source = """\
-- This file must keep its own line numbering.
library ieee; use ieee.std_logic_1164.all;
entity top is port(clk, a : in std_logic; y : out std_logic_vector(1 downto 0)); end;
library ieee; use ieee.std_logic_1164.all;
library cells;
architecture rtl of top is begin
  u: entity cells.leaf generic map(2) port map(clk, a, y);
end;
"""
        with tempfile.TemporaryDirectory() as directory:
            leaf = Path(directory) / "leaf.vhd"
            top_file = Path(directory) / "top.vhd"
            leaf.write_text(leaf_source, encoding="utf-8")
            top_file.write_text(top_source, encoding="utf-8")
            universe = netlist.naja.NLUniverse.create()
            db = netlist.naja.NLDB.create(universe)
            db.loadVHDL(str(leaf), library="cells", diagnostics_report_path=None)
            top = netlist.load_vhdl(top_file, diagnostics_report_path=None)
            self.assertEqual(str(top_file), top.get_source_declaration().range.file)
            self.assertEqual(3, top.get_source_declaration().range.line)
            child = top.get_child_instance("u")
            self.assertEqual(7, child.get_source_declaration().range.line)
            self.assertEqual(str(top_file), child.get_source_declaration().range.file)
            self.assertEqual([], child.get_source_origins())
            self.assertEqual(3, child.get_model_source_declaration().range.line)
            self.assertEqual(str(leaf), child.get_model_source_declaration().range.file)
            port = child.get_term("y").get_source_declaration()
            self.assertEqual(4, port.range.line)
            self.assertEqual("naja-vhdl", port.provider)
            self.assertEqual("vhdl", port.language)
            self.assertEqual("rtl", port.representation)
            model = db.getLibrary("cells").getSNLDesign(child.get_model_name())
            gates = list(model.getInstances())
            self.assertTrue(gates)
            for gate in gates:
                self.assertIsNone(gate.getSourceDeclaration())
                self.assertTrue(gate.getSourceOrigins())
                for origin in gate.getSourceOrigins():
                    self.assertEqual(str(leaf), origin[0][0])
                    self.assertIn(origin[0][1], (9, 10))
            def source_state(model):
                objects = ([model] + list(model.getInstances()) + list(model.getNets()) +
                           list(model.getBitNets()) + list(model.getTerms()) + list(model.getBitTerms()))
                return [(obj.getNLID().toTuple(), obj.getSourceLoc(),
                         obj.getSourceDeclaration(), obj.getSourceOrigins()) for obj in objects]

            expected = source_state(model)
            snapshot = Path(directory) / "snapshot"
            netlist.dump_naja_if(str(snapshot))
            leaf.unlink()
            top_file.unlink()
            netlist.reset()
            netlist.load_naja_if(str(snapshot))
            child = netlist.get_top().get_child_instance("u")
            self.assertEqual(port, child.get_term("y").get_source_declaration())
            model = netlist.naja.NLUniverse.get().getTopDesign().getInstance("u").getModel()
            self.assertEqual(expected, source_state(model))

    def test_source_references_batch_and_cached_logic(self):
        with tempfile.TemporaryDirectory() as directory:
            leaf = Path(directory) / "a_leaf.vhd"
            top_file = Path(directory) / "z_top.vhd"
            leaf.write_bytes(b"-- first unit\r\nentity leaf is port(a : in bit; y : out bit); end;\r\n"
                             b"architecture rtl of leaf is begin y <= not a; end;")
            top_file.write_text("-- second unit\nentity top is port(a : in bit; y : out bit); end;\n"
                                "architecture rtl of top is begin\n"
                                "  u: entity work.leaf port map(a, y);\nend;")
            for files in ([leaf, top_file], [top_file, leaf]):
                top = netlist.load_vhdl(files, diagnostics_report_path=None)
                self.assertEqual(str(top_file), top.get_source_declaration().range.file)
                self.assertEqual(2, top.get_source_declaration().range.line)
                child = top.get_child_instance("u")
                self.assertEqual(4, child.get_source_declaration().range.line)
                self.assertEqual(str(leaf), child.get_model_source_declaration().range.file)
                self.assertEqual(2, child.get_model_source_declaration().range.line)
                netlist.reset()
            top_file.write_text("entity top is port(a, b : in bit; y, z : out bit); end;\n"
                                "architecture rtl of top is begin\n"
                                "  y <= a and b;\n  z <= a and b;\nend;")
            netlist.load_vhdl(top_file, diagnostics_report_path=None)
            model = netlist.naja.NLUniverse.get().getTopDesign()
            gates = [gate for gate in model.getInstances()
                     if gate.getModel().getLibrary().getName() == "and"]
            self.assertEqual(1, len(gates))
            self.assertEqual([3, 4], [ref[0][1] for ref in gates[0].getSourceOrigins()])

    def test_load_vhdl_batch(self):
        with tempfile.TemporaryDirectory() as directory:
            leaf = Path(directory) / "leaf.vhd"
            top_file = Path(directory) / "top.vhd"
            leaf.write_text("entity leaf is port(a : in bit; y : out bit); end; "
                            "architecture rtl of leaf is begin y <= not a; end;")
            top_file.write_text("entity top is port(a : in bit; y : out bit); end; "
                                "architecture rtl of top is begin "
                                "u: entity work.leaf port map(a, y); end;")
            for files in ([leaf, top_file], [top_file, leaf]):
                top = netlist.load_vhdl(files, diagnostics_report_path=None)
                self.assertEqual(top.get_model_name(), "top")
                self.assertEqual(top.count_child_instances(), 1)
                self.assertEqual(next(top.get_child_instances()).get_model_name(), "leaf")
                netlist.reset()
        with self.assertRaises(ValueError):
            netlist.load_vhdl([])
        with self.assertRaises(TypeError):
            netlist.load_vhdl([1])

    def test_hdl_destination_library(self):
        with tempfile.TemporaryDirectory() as directory:
            vhdl = Path(directory) / "leaf.vhd"
            sv = Path(directory) / "leaf.sv"
            vhdl.write_text("entity leaf is port(a : in bit; y : out bit); end; "
                            "architecture rtl of leaf is begin y <= a; end;")
            sv.write_text("module leaf(input a, output y); assign y = a; endmodule")
            for loader, path in ((netlist.load_vhdl, vhdl),
                                 (netlist.load_system_verilog, str(sv))):
                with self.subTest(loader=loader.__name__):
                    for invalid in (None, 1):
                        with self.assertRaises(TypeError):
                            loader(path, library=invalid)
                    with self.assertRaises(ValueError):
                        loader(path, library="")
                    top = loader(path, library="Cells")
                    self.assertEqual(top.get_model_name(), "leaf")
                    self.assertIsNotNone(netlist.naja.NLUniverse.get().getTopDB().getLibrary("Cells"))
                    netlist.reset()

    def test_load_boolean_generic_default(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "boolean.vhd"
            path.write_text("entity top is generic(enabled : boolean := true); "
                            "port(a : in bit; y : out bit); end; "
                            "architecture rtl of top is begin y <= a when enabled else not a; end;")
            top = netlist.load_vhdl(path, library="BooleanCells", diagnostics_report_path=None)
            self.assertEqual(top.get_model_name(), "top")
            self.assertEqual(top.count_terms(), 2)

    def test_load_vhdl_warning_report(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "diagnostic.vhd"
            report = Path(directory) / "reports" / "warnings.log"
            path.write_text("entity e is port(y : out bit); end; "
                            "architecture rtl of e is begin assert false; "
                            "assert false; y <= '1'; end;", encoding="utf-8")
            top = netlist.load_vhdl(path, diagnostics_report_path=report)
            self.assertEqual(top.get_model_name(), "e")
            self.assertEqual(report.read_text().count("[ignored-assertion]"), 2)
            netlist.reset()
            netlist.load_vhdl(path, diagnostics_report_path=None)
            with self.assertRaises(TypeError):
                netlist.load_vhdl(path, diagnostics_report_path=1)
            with self.assertRaises(ValueError):
                netlist.load_vhdl(path, diagnostics_report_path="")

    def test_load_vhdl_hierarchy_with_explicit_top(self):
        source = """\
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture structural of top is begin
  u0: entity work.leaf port map(a, y);
end;
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "hierarchy.vhdl"
            path.write_text(source, encoding="utf-8")
            top = netlist.load_vhdl(path, top="TOP")

        self.assertEqual("top", top.get_model_name())
        self.assertEqual(1, top.count_child_instances())
        self.assertEqual("leaf", next(top.get_child_instances()).get_model_name())

    def test_load_vhdl_validates_arguments_before_native_loading(self):
        with self.assertRaisesRegex(TypeError, "VHDL file must be a path string"):
            netlist.load_vhdl(1)
        with self.assertRaisesRegex(ValueError, "VHDL file must not be empty"):
            netlist.load_vhdl(" ")
        with self.assertRaisesRegex(TypeError, "top must be a str or None"):
            netlist.load_vhdl("missing.vhd", top=1)
        with self.assertRaisesRegex(ValueError, "top must not be empty"):
            netlist.load_vhdl("missing.vhd", top=" ")

        missing = Path(tempfile.gettempdir()) / "najaeda-missing-vhdl-file.vhd"
        with self.assertRaises(FileNotFoundError) as context:
            netlist.load_vhdl(missing)
        self.assertIn(repr(str(missing)), str(context.exception))
        self.assertIn("resolved to", str(context.exception))

        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "VHDL input path is not a file"):
                netlist.load_vhdl(directory)

    def test_load_vhdl_rejects_bytes_source_pathlike(self):
        class BytesPath:
            def __fspath__(self):
                return b"source.vhd"

        for source in (BytesPath(), [BytesPath()]):
            with self.subTest(source=source):
                with self.assertRaisesRegex(TypeError, "^VHDL file must be a path string$"):
                    netlist.load_vhdl(source)

    def test_load_vhdl_rejects_bytes_report_pathlike(self):
        class BytesPath:
            def __fspath__(self):
                return b"warnings.log"

        with self.assertRaisesRegex(TypeError, "diagnostics_report_path must be a path string"):
            netlist.load_vhdl("missing.vhd", diagnostics_report_path=BytesPath())


if __name__ == "__main__":
    unittest.main()
