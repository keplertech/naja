# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import tempfile
import os
from pathlib import Path
import naja

class SNLDesignTruthTablesTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    self.designs = naja.NLLibrary.create(db)
    self.primitives = naja.NLLibrary.createPrimitives(db)

  def tearDown(self):
    del self.designs
    del self.primitives
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testGatePolarity(self):
    formats = Path(os.environ.get("FORMATS_PATH", Path(__file__).resolve().parents[2] / "formats"))
    db = self.primitives.getDB()
    db.loadLibertyPrimitives([str(formats / "liberty/benchmarks/tests/gate_polarity.lib")])
    library = db.getLibrary("gate_polarity")
    for name, kind, pin in (("cell0", "nand", "P"), ("cell1", "and", "P"),
                            ("cell2", "nor", "R"), ("cell3", "or", "P")):
      with self.subTest(cell=name):
        cell = library.getSNLDesign(name)
        self.assertEqual((kind, [cell.getScalarTerm(pin)]), cell.getGatePolarity())
        for predicate in ("isAnd", "isNand", "isOr", "isNor", "isXor", "isXnor"):
          self.assertFalse(getattr(cell, predicate)())
    prim = naja.SNLDesign.createPrimitive(self.primitives, "gate")
    first = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "P")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "Q")
    self.assertIsNone(prim.getGatePolarity())
    prim.setTruthTable(0b1011)  # !(!P*Q): row bit 0 is P.
    self.assertEqual(("nand", [first]), prim.getGatePolarity())

  def testGatePolarityFromParameter(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "parameter_gate")
    inputs = [naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, name)
              for name in ("P", "Q")]
    output = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    init = naja.SNLParameter.createBinary(prim, "INIT", 4, 8)
    prim.setTruthTableFromParameter(output, inputs, init)
    self.assertIsNone(prim.getGatePolarity())

  def testStandardGates(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "LOGIC0")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b0)
    self.assertEqual(prim.getTruthTable()[1], 0b0)
    self.assertTrue(prim.isConst0())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "LOGIC1")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b1)
    self.assertTrue(prim.isConst1())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "BUF")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "X")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b10)
    self.assertTrue(prim.isBuf())
    self.assertIsNone(prim.getGatePolarity())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "INV")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "X")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0b01)
    self.assertTrue(prim.isInv())
    self.assertIsNone(prim.getGatePolarity())

    prim = naja.SNLDesign.createPrimitive(self.primitives, "MUX2")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "S")
    naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
    prim.setTruthTable(0xCA)
    self.assertTrue(prim.isMux())
    self.assertFalse(prim.isXor())

  def testGateFamilyPredicates(self):
    def create_gate(name, mask):
      prim = naja.SNLDesign.createPrimitive(self.primitives, name)
      naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "A")
      naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "B")
      naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "Y")
      prim.setTruthTable(mask)
      return prim

    and2 = create_gate("AND2", 0x8)
    self.assertTrue(and2.isAnd())
    self.assertFalse(and2.isXor())

    nand2 = create_gate("NAND2", 0x7)
    self.assertTrue(nand2.isNand())
    self.assertFalse(nand2.isAnd())

    or2 = create_gate("OR2", 0xE)
    self.assertTrue(or2.isOr())
    self.assertFalse(or2.isNor())

    nor2 = create_gate("NOR2", 0x1)
    self.assertTrue(nor2.isNor())
    self.assertFalse(nor2.isOr())

    xor2 = create_gate("XOR2", 0x6)
    self.assertTrue(xor2.isXor())
    self.assertFalse(xor2.isXnor())

    xnor2 = create_gate("XNOR2", 0x9)
    self.assertTrue(xnor2.isXnor())
    self.assertFalse(xnor2.isXor())
    self.assertFalse(xnor2.isMux())

  def testFullAdderPredicates(self):
    with tempfile.TemporaryDirectory() as directory:
      source = Path(directory) / "add.sv"
      source.write_text("module top(input [1:0] a, output logic [1:0] y); "
                        "always_comb begin y = a; y++; end endmodule")
      top = self.designs.getDB().loadSystemVerilog([str(source)])
    instances = [instance for instance in top.getInstances()
                 if instance.getModel().getName() == "naja_fa"]
    self.assertEqual(2, len(instances))
    for instance in instances:
      model = instance.getModel()
      for name in ("isConst0", "isConst1", "isConst", "isInv", "isBuf",
                   "isAnd", "isNand", "isOr", "isNor", "isXor", "isXnor", "isMux"):
        self.assertFalse(getattr(model, name)())
      self.assertIsNone(model.getGatePolarity())
      with self.assertRaisesRegex(RuntimeError, "FA has two outputs"):
        model.getTruthTable()
      self.assertFalse(model.isConst0())

  def testMultiOutputTruthTableErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "MULTI")
    i = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Input, "I")
    o0 = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "O0")
    o1 = naja.SNLScalarTerm.create(prim, naja.SNLTerm.Direction.Output, "O1")
    init = naja.SNLParameter.createBinary(prim, "INIT", 2, 2)
    prim.setTruthTableFromParameter(o0, [i], init)
    prim.setTruthTableFromParameter(o1, [i], init)
    with self.assertRaisesRegex(RuntimeError, "design has per-output truth tables"):
      prim.getTruthTable()
    for name in ("isConst0", "isConst1", "isConst", "isInv", "isBuf",
                 "isAnd", "isNand", "isOr", "isNor", "isXor", "isXnor", "isMux"):
      with self.subTest(predicate=name):
        self.assertFalse(getattr(prim, name)())
    self.assertIsNone(prim.getGatePolarity())
    self.assertEqual([1, 2], prim.getTruthTableByOutputID(o0.getID()))

  def testPrimitiveTruthTableErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "AND2")
    with self.assertRaises(RuntimeError) as context: prim.setTruthTable("ERROR")

  def testTruthTableFromParameter(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "LUT2")
    i0 = naja.SNLScalarTerm.create(
        prim, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(
        prim, naja.SNLTerm.Direction.Input, "I1")
    output = naja.SNLScalarTerm.create(
        prim, naja.SNLTerm.Direction.Output, "O")
    init = naja.SNLParameter.createBinary(prim, "INIT", 4, 0)

    prim.setTruthTableFromParameter(output, [i0, i1], init)

    self.assertEqual([2, 0], prim.getTruthTable())

  def testTruthTableFromParameterErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "LUT2_ERRORS")
    i0 = naja.SNLScalarTerm.create(
        prim, naja.SNLTerm.Direction.Input, "I0")
    output = naja.SNLScalarTerm.create(
        prim, naja.SNLTerm.Direction.Output, "O")
    init = naja.SNLParameter.createBinary(prim, "INIT", 2, 0)

    with self.assertRaisesRegex(
        RuntimeError,
        "malformed SNLDesign.setTruthTableFromParameter method"):
      prim.setTruthTableFromParameter()

    invalid_arguments = (
      (prim, [i0], init),
      (output, (i0,), init),
      (output, [i0], prim),
    )
    for arguments in invalid_arguments:
      with self.assertRaisesRegex(
          RuntimeError,
          "setTruthTableFromParameter expects output, inputs, parameter"):
        prim.setTruthTableFromParameter(*arguments)

    with self.assertRaisesRegex(
        RuntimeError, "inputs must be SNLBitTerm objects"):
      prim.setTruthTableFromParameter(output, [prim], init)

    with self.assertRaisesRegex(
        RuntimeError, "require between 1 and 6 inputs"):
      prim.setTruthTableFromParameter(output, [], init)
   
if __name__ == '__main__':
  unittest.main()
