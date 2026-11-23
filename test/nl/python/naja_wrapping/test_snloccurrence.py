# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLOccurrenceTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(db)
    self.top = naja.SNLDesign.create(lib)
    self.model = naja.SNLDesign.create(lib)
    self.submodel = naja.SNLDesign.create(lib, "submodel")
    self.i0 = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "I0")
    self.i1 = naja.SNLBusTerm.create(self.model, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    self.o = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Output, "O")
    
  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()
    
  def testBusNetBitObject(self):
    bus = naja.SNLBusNet.create(self.top, 2, 0, "bus")
    pin = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Input, "pin")
    pin.setNet(bus.getBit(1))
    occurrence = naja.SNLOccurrence(bus.getBit(1))
    self.assertIs(type(pin.getNet()), naja.SNLBusNetBit)
    for net in (occurrence.getObject(), pin.getNet()):
      self.assertIs(type(net), naja.SNLBusNetBit)
      self.assertEqual(1, net.getBit())
      self.assertEqual(bus, net.getBus())

  def testTermObjects(self):
    occurrence = naja.SNLOccurrence(self.i1.getBusTermBit(3))
    term = occurrence.getNetComponent()
    self.assertIs(type(term), naja.SNLBusTermBit)
    self.assertEqual(3, term.getBit())
    self.assertEqual(self.i1, term.getBus())
    self.assertIs(term, occurrence.getObject())
    bus_occurrence = naja.SNLOccurrence(self.i1)
    self.assertIs(bus_occurrence.getNetComponent(), self.i1)
    self.assertIs(bus_occurrence.getObject(), self.i1)
    scalar_occurrence = naja.SNLOccurrence(self.i0)
    self.assertIs(scalar_occurrence.getObject(), self.i0)
    self.assertIsNone(naja.SNLOccurrence().getObject())

  def testOtherObjectTypes(self):
    bus = naja.SNLBusNet.create(self.top, 2, 0, "bus")
    scalar = naja.SNLScalarNet.create(self.top, "scalar")
    inst = naja.SNLInstance.create(self.top, self.model, "child")
    for obj in (bus, scalar, inst, inst.getInstTerm(self.i0)):
      occurrence = naja.SNLOccurrence(obj)
      self.assertIs(occurrence.getObject(), obj)

  def testFunctions(self):
    ins2 = naja.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = naja.SNLInstance.create(self.top, self.model, "ins1")
    
    path0 = naja.SNLPath()
    print(path0)
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    self.assertEqual(0, path0.size())
    self.assertEqual(naja.SNLPath(), path0.getHeadPath())
    
    path1 = naja.SNLPath(ins1)
    self.assertIsNotNone(path1)
    self.assertFalse(path1.empty())
    self.assertEqual(1, path1.size())
    self.assertEqual(naja.SNLPath(), path1.getHeadPath())

    path2 = naja.SNLPath(path1, ins2)
    self.assertIsNotNone(path2)
    self.assertFalse(path2.empty())
    self.assertEqual(2, path2.size())
    self.assertEqual(path1, path2.getHeadPath())

    path3 = naja.SNLPath(ins2)
    path4 = naja.SNLPath(ins1, path3)

    occurrence = naja.SNLOccurrence(path1, ins2)
    occurrence2 = naja.SNLOccurrence(ins1)
    occurrence3 = naja.SNLOccurrence()
    self.assertIsInstance(occurrence, naja.SNLOccurrence)
    self.assertTrue(occurrence.isInstanceOccurrence())
    self.assertEqual(ins2, occurrence.getInstance())
    self.assertIsNone(occurrence.getInstTerm())
    self.assertIsNone(occurrence.getNetComponent())
    self.assertIsInstance(hash(occurrence), int)
    self.assertTrue(occurrence2.isInstanceOccurrence())
    self.assertEqual(ins1, occurrence2.getInstance())
    self.assertFalse(occurrence3.isInstanceOccurrence())
    self.assertIsNone(occurrence3.getInstance())
    unbound = naja.SNLOccurrence.__new__(naja.SNLOccurrence)
    with self.assertRaisesRegex(
      RuntimeError,
      r"Attempt to call SNLOccurrence\.__hash__\(\) on an unbound object"):
      hash(unbound)

    instTerms = tuple(ins1.getInstTerms())

    netcomponentoccurrence1 = naja.SNLOccurrence(path0, instTerms[0])
    netcomponentoccurrence2 = naja.SNLOccurrence(instTerms[0])
    self.assertIsInstance(netcomponentoccurrence1, naja.SNLOccurrence)
    self.assertIsInstance(netcomponentoccurrence2, naja.SNLOccurrence)

    insttermoccurrence1 = naja.SNLOccurrence(path0, instTerms[0])
    insttermoccurrence2 = naja.SNLOccurrence(instTerms[0])
    self.assertEqual(insttermoccurrence1, insttermoccurrence2)
    self.assertEqual(hash(insttermoccurrence1), hash(insttermoccurrence2))
    self.assertEqual(1, len({insttermoccurrence1, insttermoccurrence2}))
    self.assertEqual("instterm", {insttermoccurrence1: "instterm"}[insttermoccurrence2])

    instTerm = insttermoccurrence1.getInstTerm()
    self.assertIsNotNone(instTerm)
    self.assertFalse(insttermoccurrence1.isInstanceOccurrence())
    self.assertIsNone(insttermoccurrence1.getInstance())
    
    uniq = naja.SNLUniquifier(insttermoccurrence1.getPath())
    with self.assertRaises(TypeError):
      hash(uniq)
    uniqPath = uniq.getPathUniqCollection()

    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(path1, path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(path1, path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: naja.SNLOccurrence(path1, path1)
    with self.assertRaises(RuntimeError) as context: naja.SNLUniquifier("ERROR")
    with self.assertRaises(RuntimeError) as context: naja.SNLUniquifier()
    with self.assertRaises(RuntimeError) as context: naja.SNLUniquifier("ERROR", "ERROR")
    
if __name__ == '__main__':
  unittest.main()
