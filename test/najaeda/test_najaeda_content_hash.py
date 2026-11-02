# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Content identity, raw/high-level parity and loader/snapshot contracts."""

from pathlib import Path
import tempfile
import unittest

from najaeda import naja, netlist


GOLDEN_V1 = "464123ab0366d80817af5cec91e2d71b6b7bf4cd27a7cf44a9f716246c51df19"


class ContentHashTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def fixture(self):
        universe = naja.NLUniverse.create()
        db = naja.NLDB.create(universe)
        library = naja.NLLibrary.create(db, "logic")
        top = naja.SNLDesign.create(library, "top")
        primitives = naja.NLLibrary.createPrimitives(db, "primitives")
        model = naja.SNLDesign.createPrimitive(primitives, "cell")
        pin = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "i")
        naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Output, "o")
        term = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Input, "a")
        net = naja.SNLScalarNet.create(top, "wire")
        term.setNet(net)
        instance = naja.SNLInstance.create(top, model, "u")
        instance.getInstTerm(pin).setNet(net)
        top.addAttribute(naja.SNLAttribute("keep", "yes"))
        model.setTruthTable(2)
        universe.setTopDesign(top)
        return universe, db, library, top, model, instance, term, net

    def test_golden_and_wrappers(self):
        universe, db, library, top, model, instance, term, net = self.fixture()
        self.assertEqual(1, naja.getContentHashVersion())
        self.assertEqual(["dont_touch", "keep", "keep_hierarchy"], naja.getContentHashAttributeNames())
        self.assertEqual([], naja.getContentHashPropertyNames())
        # The same golden is checked by C++ and was derived with hashlib.
        self.assertEqual(GOLDEN_V1, universe.getContentHash())
        self.assertEqual(universe.getContentHash(), netlist.get_content_hash())
        self.assertEqual(top.getContentHash(), netlist.get_top().get_content_hash())
        child = netlist.get_top().get_child_instance("u")
        self.assertEqual(model.getContentHash(), child.get_content_hash())
        for obj in (universe, db, library, top, model, instance, term, net):
            self.assertRegex(obj.getContentHash(), r"^[0-9a-f]{64}$")
            with self.assertRaises(TypeError):
                obj.getContentHash(0)
        names = naja.getContentHashAttributeNames()
        names.clear()
        self.assertEqual(3, len(naja.getContentHashAttributeNames()))

    def test_fresh_computations_after_edits(self):
        universe, _, _, top, _, instance, term, net = self.fixture()
        for obj in (instance, term, net):
            before = universe.getContentHash(), top.getContentHash(), obj.getContentHash()
            obj.setName(obj.getName() + "_renamed")
            after = universe.getContentHash(), top.getContentHash(), obj.getContentHash()
            for old, new in zip(before, after):
                self.assertNotEqual(old, new)

    def test_primitive_content_excluded(self):
        universe, db, library, top, model, instance, term, net = self.fixture()
        before = universe.getContentHash()
        model.addAttribute(naja.SNLAttribute("dont_touch", "1"))
        extra = naja.NLLibrary.createPrimitives(db, "other_primitives")
        naja.SNLDesign.createPrimitive(extra, "unused")
        naja.SNLDesign.createPrimitive(extra, "also_unused")
        self.assertEqual(before, universe.getContentHash())
        # An instance still names a particular gate.
        model.setName("different_cell")
        self.assertNotEqual(before, universe.getContentHash())

    def test_bus_bit_methods_and_attributes(self):
        universe, _, _, top, _, _, _, _ = self.fixture()
        term = naja.SNLBusTerm.create(top, naja.SNLTerm.Direction.Input, 2, -1, "port")
        net = naja.SNLBusNet.create(top, 2, -1, "bus")
        term.setNet(net)
        for bus in (term, net):
            before = bus.getContentHash(), universe.getContentHash()
            (bus.getBusTermBit(0) if bus is term else bus.getBit(0)).addAttribute(naja.SNLAttribute("keep", "1"))
            self.assertNotEqual(before, (bus.getContentHash(), universe.getContentHash()))
            bits = list(bus.getBits())
            self.assertNotEqual(bits[0].getContentHash(), bits[1].getContentHash())

    def test_verilog_snapshot_and_source_exclusion(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "input.v"
            source.write_text('module cell(input a, output y); endmodule\n'
                              '(* keep="yes" *) module top(input a, output y);\n'
                              'cell u(a,y); endmodule\n', encoding="utf-8")
            netlist.load_gate_level_verilog(str(source))
            universe = naja.NLUniverse.get()
            expected = universe.getContentHash()
            expected_design = universe.getTopDesign().getContentHash()
            netlist.dump_naja_if(str(root / "snapshot"))
            netlist.reset()
            netlist.load_naja_if(str(root / "snapshot"))
            self.assertEqual(expected, netlist.get_content_hash())
            self.assertEqual(expected_design, netlist.get_top().get_content_hash())
            # Manifest identity remains strictly checked by the loader, but is
            # never a hash input: hashing operates entirely on the loaded model.
            (root / "snapshot" / "snl.mf").write_text("unrelated manifest", encoding="utf-8")
            self.assertEqual(expected, netlist.get_content_hash())

    def test_without_universe(self):
        self.assertIsNone(naja.NLUniverse.get())
        with self.assertRaises(RuntimeError):
            netlist.get_content_hash()


if __name__ == "__main__":
    unittest.main()
