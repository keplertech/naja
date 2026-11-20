
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import naja
from najaeda.primitives import utils


class NajaNetlistTestPrimitives(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_bit_terms_scalar_and_none(self):
        term = object()
        self.assertEqual([], utils._bit_terms(None))
        self.assertEqual([term], utils._bit_terms(term))

    def test_parameterized_term_roles(self):
        from itertools import product
        top = netlist.create_top("Top")
        db = naja.NLUniverse.get().getTopDB()
        library = naja.NLLibrary.createPrimitives(db, "PRIMITIVES")
        model = naja.SNLDesign.createPrimitive(library, "TEST_DFF")
        pins = {name: naja.SNLScalarTerm.create(
            model, naja.SNLTerm.Direction.Output if name == "O" else
            naja.SNLTerm.Direction.Input, name) for name in ("CK", "R", "L", "I", "O")}
        roles, levels = naja.SNLTermRole, naja.SNLActiveLevel
        names = ["reset_present", "reset_synchronous", "reset_sets_value", "enable_present"]
        parameters = [naja.SNLParameter.createBinary(model, name, 1, 0) for name in names]
        pins["CK"].setRole(roles.Clock)
        pins["R"].setRole(roles.AsyncReset, levels.Low)
        pins["L"].setRole(roles.Enable)
        pins["I"].setRole(roles.DataInput)
        pins["O"].setRole(roles.DataOutput)
        entries = []
        for init, sync, typ, load in product((0, 1), repeat=4):
            reset = ((roles.SyncSet if typ else roles.SyncReset) if sync else
                     (roles.AsyncSet if typ else roles.AsyncReset)) if init else roles.Other
            entries.append({"values": [init, sync, typ, load], "roles": {
                pins["R"]: (reset, levels.High) if init else None,
                pins["L"]: (roles.Enable, levels.NA) if load else None}})
        model.setRolesFromParameters(names, entries)
        for index, entry in enumerate(entries):
            child = top.create_child_instance("TEST_DFF", f"ff{index}")
            instance = netlist.get_snl_instance_from_id_list(child.pathIDs)
            for parameter, value in zip(parameters, entry["values"]):
                naja.SNLInstParameter.create(instance, parameter, str(value))
            reset, enable = child.get_term("R"), child.get_term("L")
            expected = entry["roles"][pins["R"]]
            expected_role = expected[0] if expected else roles.Other
            self.assertEqual(expected_role, reset.get_role())
            self.assertEqual(expected_role == roles.AsyncReset, reset.is_async_reset())
            self.assertEqual(expected_role == roles.SyncReset, reset.is_sync_reset())
            self.assertEqual(expected_role == roles.AsyncSet, reset.is_async_set())
            self.assertEqual(expected_role == roles.SyncSet, reset.is_sync_set())
            self.assertEqual(expected_role in (roles.AsyncReset, roles.SyncReset), reset.is_reset())
            self.assertEqual(levels.High if expected else levels.NA, reset.get_reset_active_level())
            self.assertEqual(bool(entry["values"][3]), enable.is_enable())
            self.assertTrue(child.get_term("CK").is_clock())
            self.assertTrue(child.get_term("I").is_data_input())
            self.assertTrue(child.get_term("O").is_data_output())
            self.assertTrue(child.get_term("O").is_data())
        # Top-level terms query their static roles, while buses require a bit.
        top_design = naja.NLUniverse.get().getTopDesign()
        clock = naja.SNLScalarTerm.create(top_design, naja.SNLTerm.Direction.Input, "clock")
        self.assertEqual(roles.Other, top.get_term("clock").get_role())
        bus = naja.SNLBusTerm.create(top_design, naja.SNLTerm.Direction.Input, 1, 0, "bus")
        with self.assertRaisesRegex(ValueError, "scalar term or a bus bit"):
            top.get_term("bus").is_clock()
        self.assertEqual(roles.Other, top.get_term("bus").get_bit(0).get_role())

    def test_yosys_primitives(self):
        netlist.load_primitives('yosys')
        top = netlist.create_top('Top')

        and2_ins = top.create_child_instance('$_AND_', 'and2_ins')
        or2_ins = top.create_child_instance('$_OR_', 'or2_ins')
        self.assertIsNotNone(and2_ins)
        self.assertIsNotNone(or2_ins)

        library = naja.NLUniverse.get().getTopDB().getLibrary("yosys")
        dff = library.getSNLDesign("$_DFF_P_")
        self.assertTrue(dff.getScalarTerm("C").isClock())
        self.assertTrue(dff.getScalarTerm("D").isDataInput())
        self.assertTrue(dff.getScalarTerm("Q").isDataOutput())
        self.assertEqual(
            [dff.getScalarTerm("D")],
            list(dff.getClockRelatedInputs(dff.getScalarTerm("C"))),
        )

        async_reset = library.getSNLDesign("$_DFFE_PN0N_")
        self.assertTrue(async_reset.getScalarTerm("E").isEnable())
        self.assertTrue(async_reset.getScalarTerm("R").isAsyncReset())
        self.assertEqual(
            naja.SNLActiveLevel.Low,
            async_reset.getScalarTerm("R").getResetActiveLevel(),
        )

        sync_set = library.getSNLDesign("$_SDFFCE_PP1P_")
        self.assertTrue(sync_set.getScalarTerm("E").isEnable())
        self.assertTrue(sync_set.getScalarTerm("R").isSyncSet())
        self.assertEqual(
            naja.SNLActiveLevel.High,
            sync_set.getScalarTerm("R").getResetActiveLevel(),
        )

    def test_primitive_rejects_child_instance(self):
        top = netlist.create_top("Top")
        db = naja.NLUniverse.get().getTopDB()
        primitive_library = naja.NLLibrary.createPrimitives(db, "PRIMITIVES")
        naja.SNLDesign.createPrimitive(primitive_library, "primitive")
        naja.SNLDesign.create(
            naja.NLUniverse.get().getTopDesign().getLibrary(), "child")

        primitive = top.create_child_instance("primitive", "primitive")
        with self.assertRaisesRegex(
            RuntimeError, "Cannot create SNLInstance in primitive design"):
            primitive.create_child_instance("child", "nested")
        self.assertEqual(0, primitive.count_child_instances())

    def test_gate_family_predicates(self):
        top = netlist.create_top('Top')
        primitives = naja.NLLibrary.createPrimitives(
            naja.NLUniverse.get().getTopDB(), "gate_primitives")

        def create_gate(name, mask):
            prim = naja.SNLDesign.createPrimitive(primitives, name)
            naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "A")
            naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "B")
            naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
            prim.setTruthTable(mask)
            return top.create_child_instance(name, f"{name.lower()}_ins")

        and2_ins = create_gate("AND2", 0x8)
        nand2_ins = create_gate("NAND2", 0x7)
        or2_ins = create_gate("OR2", 0xE)
        nor2_ins = create_gate("NOR2", 0x1)
        xor2_ins = create_gate("XOR2", 0x6)
        xnor2_ins = create_gate("XNOR2", 0x9)

        self.assertTrue(and2_ins.is_and())
        self.assertFalse(and2_ins.is_nand())
        self.assertTrue(nand2_ins.is_nand())
        self.assertTrue(or2_ins.is_or())
        self.assertTrue(nor2_ins.is_nor())
        self.assertTrue(xor2_ins.is_xor())
        self.assertTrue(xnor2_ins.is_xnor())

    def test_xilinx_sequential_instance_models(self):
        netlist.create_top("Top")
        netlist.load_primitives("xilinx")
        library = naja.NLUniverse.get().getTopDB().getLibrary("xilinx")
        raw_top = naja.NLUniverse.get().getTopDesign()
        for name in ("FDCE", "FDPE", "FDRE", "FDSE"):
            primitive = library.getSNLDesign(name)
            instance = naja.SNLInstance.create(raw_top, primitive, name.lower())
            self.assertTrue(primitive.hasSequentialModel())
            self.assertFalse(primitive.hasSequentialModelFromParameters())
            model = instance.getSequentialModel()
            self.assertEqual(primitive.getSequentialModel(), model)
            self.assertEqual("flip_flop", model["kind"])
            self.assertEqual(("term", primitive.getScalarTerm("C")), model["clocked_on"])
            state = model["states"][0]
            self.assertEqual({"parameter": "INIT", "bit": 0}, state["init_from"])
            self.assertEqual([1 if name == "FDPE" else 0],
                             instance.getSequentialInitialValues())
            override = naja.SNLInstParameter.create(instance, primitive.getParameter("INIT"), "1'b1")
            self.assertEqual([1], instance.getSequentialInitialValues())
            override.setValue("1'bx")
            self.assertEqual([None], instance.getSequentialInitialValues())
            if name == "FDCE":
                self.assertEqual(("term", primitive.getScalarTerm("CLR")), state["clear"])
            elif name == "FDPE":
                self.assertEqual(("term", primitive.getScalarTerm("PRE")), state["preset"])
            else:
                self.assertIsNone(state["clear"])
                self.assertIsNone(state["preset"])


    def test_xilinx_primitives(self):
        top = netlist.create_top('Top')
        i = top.create_input_term("I")
        o = top.create_output_term("O")
        self.assertIsNotNone(top)
        netlist.load_primitives('xilinx')
        library = naja.NLUniverse.get().getTopDB().getLibrary("xilinx")

        fdce = library.getSNLDesign("FDCE")
        self.assertTrue(fdce.getScalarTerm("C").isClock())
        self.assertTrue(fdce.getScalarTerm("D").isDataInput())
        self.assertTrue(fdce.getScalarTerm("Q").isDataOutput())
        self.assertTrue(fdce.getScalarTerm("CE").isEnable())
        self.assertTrue(fdce.getScalarTerm("CLR").isAsyncReset())
        self.assertEqual(
            naja.SNLActiveLevel.High,
            fdce.getScalarTerm("CLR").getResetActiveLevel(),
        )

        fdse = library.getSNLDesign("FDSE")
        self.assertTrue(fdse.getScalarTerm("S").isSyncSet())

        ram32m = library.getSNLDesign("RAM32M")
        self.assertTrue(ram32m.getScalarTerm("WCLK").isClock())
        self.assertEqual(
            naja.SNLTermRole.MemoryWriteEnable,
            ram32m.getScalarTerm("WE").getRole(),
        )
        self.assertEqual(
            naja.SNLTermRole.MemoryReadData,
            ram32m.getBusTerm("DOA").getBusTermBit(0).getRole(),
        )

        lut2_ins0 = top.create_child_instance('LUT2', 'ins0')
        lut2_ins1 = top.create_child_instance('LUT2', 'ins1')
        self.assertIsNotNone(lut2_ins0)
        self.assertIsNotNone(lut2_ins1)

        net1 = top.create_net("net1") 
        net2 = top.create_net("net2")

        i.connect_lower_net(net1)
        lut2_ins0.get_term("I0").connect_upper_net(net1)
        lut2_ins0.get_term("O").connect_upper_net(net2)
        lut2_ins1.get_term("I0").connect_upper_net(net2)
        o.connect_lower_net(net2)

        top.dump_full_dot('./test_xilinx_primitives.dot')
        with self.assertRaises(Exception) as context: top.dump_full_dot(-1)
        top.dump_context_dot('./test_xilinx_primitives_context.dot')
        with self.assertRaises(Exception) as context: top.dump_context_dot(-1)

        lut2_ins1.get_term("I0").get_equipotential().dump_dot('./test_xilinx_primitives_lut2_ins1.dot')

        leaf_drivers_count = 0
        for leaf_driver in lut2_ins1.get_term("I0").get_equipotential().get_leaf_drivers():
            leaf_drivers_count += 1
        top_readers_count = 0
        for top_reader in lut2_ins1.get_term("I0").get_equipotential().get_top_readers():
            top_readers_count += 1
        top_drivers_count = 0
        for top_driver in lut2_ins0.get_term("I0").get_equipotential().get_top_drivers():
            top_drivers_count +=1
        
        self.assertEqual(i.get_equipotential(), lut2_ins0.get_term("I0").get_equipotential())

        self.assertEqual(1, leaf_drivers_count)
        self.assertEqual(1, top_readers_count)
        self.assertEqual(1, top_drivers_count)
        self.assertEqual(lut2_ins0.get_design(), top)

        leaf_count = 0
        for leaf in top.get_leaf_children():
            leaf_count += 1
        
        self.assertEqual(2, leaf_count)

        child_instance = top.get_child_instance_by_id(0)
        self.assertIsNotNone(child_instance)
        child_instance.delete()
        lut2_ins1.delete()

    def test_errors(self):
        with self.assertRaises(Exception) as context:
            netlist.load_primitives('unknown')

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
