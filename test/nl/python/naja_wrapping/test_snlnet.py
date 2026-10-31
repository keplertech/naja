# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import naja

class SNLNetTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    self.lib = naja.NLLibrary.create(db)
    self.design = naja.SNLDesign.create(self.lib)
    self.model = naja.SNLDesign.create(self.lib, "model")

  def tearDown(self):
    del self.lib
    del self.design
    del self.model
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testGetNetByID(self):
    nets = [
      naja.SNLScalarNet.create(self.design, "scalar"),
      naja.SNLBusNet.create(self.design, 3, -1, "bus"),
      naja.SNLScalarNet.create(self.design, ""),
      naja.SNLBusNet.create(self.design, -2, 2, ""),
    ]
    nets[2].setType(naja.SNLNet.Type.Assign0)
    for net in nets:
      with self.subTest(netID=net.getID()):
        self.assertEqual(net.getNLID().getDesignObjectID(), net.getID())
        self.assertIs(net, self.design.getNetByID(net.getID()))
        self.assertIsNone(self.model.getNetByID(net.getID()))
    self.assertEqual("", nets[2].getName())
    self.assertTrue(nets[2].isConstant0())
    self.assertIsNone(self.design.getNetByID(max(net.getID() for net in nets) + 1))
    for bus in (nets[1], nets[3]):
      for bit in bus.getBits():
        self.assertEqual(bus.getID(), bit.getID())
        self.assertIs(bit, self.design.getNetByID(bus.getID()).getBit(bit.getBit()))

  def testGetNetByIDErrors(self):
    for args in ((), ("invalid",), (None,), (1.5,), (0, 1)):
      with self.subTest(args=args):
        with self.assertRaises(RuntimeError):
          self.design.getNetByID(*args)

  def test0(self):
    self.assertIsNotNone(self.design)

    i0 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLBusTerm.create(self.design, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    o = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Output, "O")

    self.assertEqual(3, sum(1 for t in self.design.getTerms()))
    self.assertEqual(0, sum(1 for n in self.design.getNets()))

    self.assertIsNone(i0.getNet())
    i0Net = naja.SNLScalarNet.create(self.design, "I0")
    self.assertIsNotNone(i0Net)
    self.assertEqual(self.design, i0Net.getDesign())
    self.assertEqual("I0", i0Net.getName())
    self.assertEqual(naja.SNLNet.Type.Standard, i0Net.getType())
    i0.setNet(i0Net)
    self.assertEqual(i0.getNet(), i0Net)
    self.assertEqual(i0Net, i0.getNet())
    self.assertEqual(i0Net, self.design.getNet("I0"))
    self.assertEqual(i0Net, self.design.getScalarNet("I0"))
    self.assertIsNone(self.design.getBusNet("I0"))
    self.assertEqual(1, sum(1 for b in i0Net.getBits()))
    self.assertEqual(1, sum(1 for n in self.design.getNets()))
    self.assertEqual(1, sum(1 for n in self.design.getScalarNets()))
    self.assertEqual(0, sum(1 for n in self.design.getBusNets()))
    self.assertEqual(1, sum(1 for c in i0Net.getComponents())) 
    self.assertEqual(1, sum(1 for b in i0Net.getBitTerms())) 
    self.assertEqual(0, sum(1 for i in i0Net.getInstTerms()))
    c = next(iter(i0Net.getComponents()))
    self.assertEqual(i0, c)
    self.assertIsInstance(c, naja.SNLNetComponent)
    self.assertIsInstance(c, naja.SNLBitTerm)
    self.assertIsInstance(c, naja.SNLScalarTerm)

    i1Net = naja.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertIsInstance(i1Net, naja.SNLBusNet)
    self.assertIsInstance(i1Net, naja.SNLNet)
    self.assertIsInstance(i1Net, naja.SNLDesignObject)
    self.assertIsNotNone(i1Net)
    self.assertEqual("I1", i1Net.getName())
    with self.assertRaises(RuntimeError) as context: naja.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertEqual(4, i1.getMSB())
    self.assertEqual(0, i1.getLSB())
    self.assertEqual(5, i1.getWidth())
    self.assertEqual(4, i1Net.getMSB())
    self.assertEqual(0, i1Net.getLSB())
    self.assertEqual(5, i1Net.getWidth())
    self.assertEqual(i1Net, self.design.getNet("I1"))
    self.assertEqual(i1Net, self.design.getBusNet("I1"))
    self.assertIsNone(self.design.getScalarNet("I1"))
    i1.setNet(i1Net)
    with self.assertRaises(RuntimeError) as context: i1.setNet(self.design)
    self.assertEqual(5, sum(1 for b in i1Net.getBits()))
    self.assertEqual(2, sum(1 for n in self.design.getNets()))
    self.assertEqual(1, sum(1 for n in self.design.getScalarNets()))
    self.assertEqual(1, sum(1 for n in self.design.getBusNets()))
    self.assertEqual(1+5, sum(1 for b in self.design.getBitNets()))
    
    i1Bit4 = i1.getBusTermBit(4)
    i1NetBit4 = i1Net.getBit(4)
    self.assertIsNotNone(i1Bit4)
    self.assertIsNotNone(i1NetBit4)
    self.assertEqual(i1NetBit4, i1Net.getBitAtPosition(0))
    self.assertEqual(i1Bit4.getNet(), i1NetBit4)
    self.assertEqual(1, sum(1 for c in i1NetBit4.getComponents()))
    self.assertEqual(1, sum(1 for b in i1NetBit4.getBitTerms()))
    self.assertEqual(0, sum(1 for i in i1NetBit4.getInstTerms()))
    c = next(iter(i1NetBit4.getComponents()))
    self.assertEqual(i1Bit4, c)
    self.assertIsInstance(c, naja.SNLNetComponent)
    self.assertIsInstance(c, naja.SNLBitTerm)
    self.assertIsInstance(c, naja.SNLBusTermBit)

    #create another design
    design2 = naja.SNLDesign.create(self.lib)
    design2Net = naja.SNLScalarNet.create(design2, "net")
    with self.assertRaises(RuntimeError) as context: i1.setNet(design2Net)

    self.assertIsNone(i1.getBusTermBit(5))
    self.assertIsNone(i1Net.getBit(5))
    del i1Bit4
    del i1NetBit4

  def testComponents(self):
    topI0 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Input, "I0")
    topI1 = naja.SNLBusTerm.create(self.design, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    topO0 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Output, "O0")
    topO1 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Output, "O1")

    i0 = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "i0")
    i1 = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "i1")
    o = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Output, "o")

    ins1 = naja.SNLInstance.create(self.design, self.model, "ins1")
    ins2 = naja.SNLInstance.create(self.design, self.model, "ins2")

    i0Net = naja.SNLScalarNet.create(self.design, "I0")
    topI0.setNet(i0Net)
    i1Net = naja.SNLScalarNet.create(self.design, "I1")
    topI1.getBusTermBit(0).setNet(i1Net)
    o0Net = naja.SNLScalarNet.create(self.design, "O0")
    topO0.setNet(o0Net)
    o1Net = naja.SNLScalarNet.create(self.design, "O1")
    topO1.setNet(o1Net)
    self.assertIsNotNone(self.model.getScalarTerm("i0"))
    ins1I0 = ins1.getInstTerm(self.model.getScalarTerm("i0"))
    self.assertIsNotNone(ins1I0)
    ins1I0.setNet(i0Net)
    del ins1I0
    ins1.getInstTerm(self.model.getScalarTerm("i1")).setNet(i1Net)
    ins1.getInstTerm(self.model.getScalarTerm("o")).setNet(o0Net)
    ins2.getInstTerm(self.model.getScalarTerm("i0")).setNet(i0Net)
    ins2.getInstTerm(self.model.getScalarTerm("i1")).setNet(i1Net)
    ins2.getInstTerm(self.model.getScalarTerm("o")).setNet(o1Net)
    self.assertEqual(3, sum(1 for c in i0Net.getComponents()))
    i0NetList = list(i0Net.getComponents())
    self.assertEqual(3, len(i0NetList))
    self.assertEqual(topI0, i0NetList[0])
    self.assertEqual(ins1.getInstTerm(self.model.getScalarTerm("i0")), i0NetList[1])
    self.assertEqual(ins2.getInstTerm(self.model.getScalarTerm("i0")), i0NetList[2])

    i1NetList = list(i1Net.getComponents())
    self.assertEqual(3, len(i1NetList))
    self.assertEqual(topI1.getBusTermBit(0), i1NetList[0])
    self.assertEqual(topI1, topI1.getBusTermBit(0).getBus())
    self.assertEqual(0, topI1.getBusTermBit(0).getBit())
    self.assertEqual(ins1.getInstTerm(self.model.getScalarTerm("i1")), i1NetList[1])
    self.assertEqual(ins2.getInstTerm(self.model.getScalarTerm("i1")), i1NetList[2])

    self.assertEqual(3, sum(1 for c in i1Net.getComponents()))
    self.assertEqual(2, sum(1 for c in o0Net.getComponents()))
    self.assertEqual(2, sum(1 for c in o1Net.getComponents()))

    #delete topI0 as a bitTerm
    i0NetList[0].destroy()
    self.assertEqual(2, sum(1 for c in i0Net.getComponents()))

    #delete ins0
    ins1.destroy()
    self.assertEqual(1, sum(1 for c in i0Net.getComponents()))

  def testRenameNet(self):
    i0Net = naja.SNLScalarNet.create(self.design, "I0")
    i1Net = naja.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertEqual("I0", i0Net.getName())
    self.assertEqual("I1", i1Net.getName())
    i0Net.setName("I2")
    i1Net.setName("I3")
    self.assertEqual("I2", i0Net.getName())
    self.assertEqual("I3", i1Net.getName())
    self.assertEqual(i0Net, self.design.getNet("I2"))
    self.assertEqual(i1Net, self.design.getNet("I3"))
    self.assertIsNone(self.design.getNet("I0"))
    self.assertIsNone(self.design.getNet("I1"))
    self.assertEqual(i1Net, i1Net.getBit(0).getBus())
    self.assertEqual(0, i1Net.getBit(0).getBit())

  def testNetType(self):
    i0Net = naja.SNLScalarNet.create(self.design, "I0")
    i1Net = naja.SNLBusNet.create(self.design, 4, 0, "I1")
    self.assertEqual(naja.SNLNet.Type.Standard, i0Net.getType())
    i0Net.setType(naja.SNLNet.Type.Assign0)
    self.assertEqual(naja.SNLNet.Type.Assign0, i0Net.getType())
    i0Net.setType(naja.SNLNet.Type.Assign1)
    self.assertEqual(naja.SNLNet.Type.Assign1, i0Net.getType())
    self.assertTrue(i0Net.isConstant1())
    self.assertTrue(i0Net.isConstant())
    i0Net.setType(naja.SNLNet.Type.Supply0)
    self.assertEqual(naja.SNLNet.Type.Supply0, i0Net.getType())
    self.assertTrue(i0Net.isConstant0())
    self.assertTrue(i0Net.isConstant())
    i0Net.setType(naja.SNLNet.Type.Supply1)
    self.assertEqual(naja.SNLNet.Type.Supply1, i0Net.getType())
    i0Net.setType(naja.SNLNet.Type.AssignX)
    self.assertEqual(naja.SNLNet.Type.AssignX, i0Net.getType())
    self.assertEqual("AssignX", i0Net.getTypeAsString())
    self.assertTrue(i0Net.isConstantX())
    self.assertTrue(i0Net.isConstant())
    self.assertFalse(i0Net.isConstant0())
    self.assertFalse(i0Net.isConstant1())
    i0Net.setType(naja.SNLNet.Type.AssignZ)
    self.assertEqual(naja.SNLNet.Type.AssignZ, i0Net.getType())
    self.assertEqual("AssignZ", i0Net.getTypeAsString())
    self.assertTrue(i0Net.isConstantZ())
    self.assertTrue(i0Net.isConstant())
    i0Net.setType(naja.SNLNet.Type.Standard)
    self.assertFalse(i0Net.isConstant0())
    self.assertFalse(i0Net.isConstant1())
    self.assertFalse(i0Net.isConstantX())
    self.assertFalse(i0Net.isConstantZ())
    self.assertFalse(i0Net.isConstant())

  def test_bus_net_resize(self):
    net = naja.SNLBusNet.create(self.design, 3, 0, "net")
    net.setMSB(1)
    self.assertEqual(1, net.getMSB())
    self.assertEqual(0, net.getLSB())
    self.assertEqual(2, net.getWidth())
    net.setLSB(1)
    self.assertEqual(1, net.getMSB())
    self.assertEqual(1, net.getLSB())
    self.assertEqual(1, net.getWidth())
    net.setMSB(1)  # no-op
    net.setLSB(1)  # no-op

  def test_bus_net_resize_invalid(self):
    net = naja.SNLBusNet.create(self.design, 3, 0, "net")
    with self.assertRaises(RuntimeError):
      net.setMSB(-1)
    with self.assertRaises(RuntimeError):
      net.setLSB(4)
    with self.assertRaisesRegex(RuntimeError, r"SNLBusNet\.setMSB\(\) expects an integer argument"):
      net.setMSB("1")
    with self.assertRaisesRegex(RuntimeError, r"SNLBusNet\.setLSB\(\) expects an integer argument"):
      net.setLSB("0")

  def test_bus_net_resize_connected(self):
    net = naja.SNLBusNet.create(self.design, 3, 0, "net")
    term = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.InOut, "t0")
    term.setNet(net.getBit(3))
    with self.assertRaises(RuntimeError):
      net.setMSB(2)

  def testErrors(self):
    self.assertIsNotNone(self.design)
    i0 = naja.SNLScalarTerm.create(self.design, naja.SNLTerm.Direction.Input, "I0")
    #wrong arg type
    with self.assertRaises(RuntimeError) as context: i0.setNet(self.design)
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarNet.create(self.lib, "I1")
    with self.assertRaises(RuntimeError) as context: naja.SNLBusNet.create(self.lib, 4, 0, "I1")
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarNet.create(self.design, 4, 0, "I1")
    with self.assertRaises(RuntimeError) as context: naja.SNLBusNet.create(self.lib, "I1")

    net = naja.SNLScalarNet.create(self.design, "net")
    with self.assertRaises(RuntimeError) as context: net.setType(i0)

  def testNameClash(self):
    i0Net = naja.SNLScalarNet.create(self.design, "I0")
    with self.assertRaises(RuntimeError) as context: naja.SNLScalarNet.create(self.design, "I0")
    with self.assertRaises(RuntimeError) as context: naja.SNLBusNet.create(self.design, 4, 0, "I0")
    
if __name__ == '__main__':
  unittest.main()
