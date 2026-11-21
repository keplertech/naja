# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import naja

class SNLEquiTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    self.db = naja.NLDB.create(universe)
    lib = naja.NLLibrary.create(self.db)
    self.primitives = naja.NLLibrary.createPrimitives(self.db)
    self.top = naja.SNLDesign.create(lib)
    self.top_out = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Output, "OUT")
    self.model = naja.SNLDesign.create(lib, "model")
    self.submodel = naja.SNLDesign.createPrimitive(self.primitives, "submodel")
    self.i0 = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "I0")
    self.i1 = naja.SNLBusTerm.create(self.model, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    self.o = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Output, "O")
    self.i0sub = naja.SNLScalarTerm.create(self.submodel, naja.SNLTerm.Direction.Input, "I0")
    self.i1sub = naja.SNLBusTerm.create(self.submodel, naja.SNLTerm.Direction.Input, 4, 0, "I1")
    self.osub = naja.SNLScalarTerm.create(self.submodel, naja.SNLTerm.Direction.Output, "O")
    
  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()
    
  def test(self):
    ins2 = naja.SNLInstance.create(self.model, self.submodel, "ins2")
    ins1 = naja.SNLInstance.create(self.top, self.model, "ins1")
    
    path0 = naja.SNLPath()
    print(path0)
    self.assertIsNotNone(path0)
    self.assertTrue(path0.empty())
    self.assertEqual(0, path0.size())
    self.assertEqual(naja.SNLPath(), path0.getHeadPath())

    instTerms = tuple(ins1.getInstTerms())
    i0Net = naja.SNLScalarNet.create(self.top, "I0")
    self.top_out.setNet(i0Net)
    instTerms[0].setNet(i0Net)  
    i0Netsub = naja.SNLScalarNet.create(self.model, "I0")
    subinstTerms = tuple(ins2.getInstTerms())
    subinstTerms[0].setNet(i0Netsub)  
    self.i0.setNet(i0Netsub)  
    #print(instTerms[0])  
    path1 = naja.SNLPath(path0, ins1)
    path2 = naja.SNLPath(path1, ins2)
    netcomponentoccurrence1 = naja.SNLOccurrence(path1, subinstTerms[0])

    #insttermoccurrence1 = naja.SNLInstTermOccurrence(path0, instTerms[0])

    equi0 = naja.SNLEquipotential(netcomponentoccurrence1)
    topTerms = [t for t in equi0.getTerms()]
    self.assertListEqual([self.top_out], topTerms)
    insttermoccurrences = [i for i in equi0.getInstTermOccurrences()]
    self.assertEqual(1, len(insttermoccurrences))
    self.assertListEqual([netcomponentoccurrence1], insttermoccurrences)

    equi1 = naja.SNLEquipotential(self.top_out)
    self.assertEqual(equi0, equi1)
    with self.assertRaises(TypeError):
      hash(equi0)
    topTerms = [t for t in equi1.getTerms()]
    self.assertListEqual([self.top_out], topTerms)
    insttermoccurrences = [i for i in equi1.getInstTermOccurrences()]
    self.assertEqual(1, len(insttermoccurrences))
    self.assertListEqual([netcomponentoccurrence1], insttermoccurrences)

  def testConstant0TopTerm(self):
    i0Net = naja.SNLScalarNet.create(self.top, "I0")
    i0Net.setType(naja.SNLNet.Type.Assign0)
    self.top_out.setNet(i0Net)
    equi = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi.isConst0())

  def testConstant1TopTerm(self):
    i0Net = naja.SNLScalarNet.create(self.top, "I0")
    i0Net.setType(naja.SNLNet.Type.Assign1)
    self.top_out.setNet(i0Net)
    equi = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi.isConst1())

  def testConstantXAndZTopTerm(self):
    net = naja.SNLScalarNet.create(self.top, "four_state")
    self.top_out.setNet(net)
    net.setType(naja.SNLNet.Type.AssignX)
    equi_x = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi_x.isConstX())
    self.assertFalse(equi_x.isConstZ())
    net.setType(naja.SNLNet.Type.AssignZ)
    equi_z = naja.SNLEquipotential(self.top_out)
    self.assertTrue(equi_z.isConstZ())
    self.assertFalse(equi_z.isConstX())

  def testTraverseAssigns(self):
    with tempfile.NamedTemporaryFile("w", suffix=".v", delete=False) as source:
      source.write(
        "module assign_top(input i, output o); "
        "wire n; assign n = i; assign o = n; endmodule\n")
      source_path = source.name
    try:
      top = self.db.loadGateLevelVerilog([source_path])
      input_term = top.getScalarTerm("i")
      output_term = top.getScalarTerm("o")

      standard = naja.SNLEquipotential(input_term)
      self.assertListEqual([input_term], list(standard.getTerms()))
      assign_occurrences = list(standard.getInstTermOccurrences())
      self.assertEqual(1, len(assign_occurrences))
      self.assertEqual(
        standard,
        naja.SNLEquipotential(
          input_term, mode=naja.SNLEquipotential.Mode.Standard))

      traversed = naja.SNLEquipotential(
        input_term, mode=naja.SNLEquipotential.Mode.TraverseAssigns)
      self.assertCountEqual(
        [input_term, output_term], list(traversed.getTerms()))
      self.assertListEqual([], list(traversed.getInstTermOccurrences()))
      self.assertEqual(
        traversed,
        naja.SNLEquipotential(
          output_term, naja.SNLEquipotential.Mode.TraverseAssigns))
      self.assertEqual(
        traversed,
        naja.SNLEquipotential(
          assign_occurrences[0],
          mode=naja.SNLEquipotential.Mode.TraverseAssigns))
      self.checkGraph(input_term, 1, 0)
      for start in (input_term, output_term, assign_occurrences[0]):
        graph_equi = self.checkGraph(
          start, 3, 2, naja.SNLEquipotential.Mode.TraverseAssigns)
        nodes, edges = graph_equi.getGraph()
        for kind, crossing, source, target in edges:
          self.assertEqual(naja.SNLEquipotential.EdgeKind.Assign, kind)
          self.assertEqual(nodes[source][0].getPath(), nodes[target][0].getPath())
          self.assertEqual(naja.SNLTerm.Direction.Input,
                           crossing.getInstTerm().getBitTerm().getDirection())
      with self.assertRaisesRegex(
          RuntimeError, "invalid SNLEquipotential.Mode value"):
        naja.SNLEquipotential(input_term, mode=99)
      with self.assertRaisesRegex(
          RuntimeError,
          "SNLEquipotential mode must be an SNLEquipotential.Mode"):
        naja.SNLEquipotential(input_term, mode="TraverseAssigns")
    finally:
      os.remove(source_path)

  def checkGraph(self, start, node_count, edge_count, mode=None):
    if mode is None:
      mode = naja.SNLEquipotential.Mode.Standard
    equi = naja.SNLEquipotential(start, mode=mode, keep_graph=True)
    plain = naja.SNLEquipotential(start, mode=mode)
    self.assertIsNone(plain.getGraph())
    self.assertEqual(plain, equi)
    nodes, edges = equi.getGraph()
    self.assertIsInstance(equi.getGraph(), tuple)
    self.assertEqual(node_count, len(nodes))
    self.assertEqual(edge_count, len(edges))
    leaves = [leaf for _, leaves, _ in nodes for leaf in leaves]
    terms = [term for _, _, terms in nodes for term in terms]
    self.assertCountEqual(list(equi.getInstTermOccurrences()), leaves)
    self.assertCountEqual(list(equi.getTerms()), terms)
    self.assertEqual(len(leaves), len(set(str(leaf) for leaf in leaves)))
    self.assertEqual(len(edges), len(set(str(edge[1]) for edge in edges)))
    for net, leaves, terms in nodes:
      self.assertIsInstance(net, naja.SNLOccurrence)
      self.assertIsInstance(leaves, list)
      self.assertIsInstance(terms, list)
      for leaf in leaves:
        self.assertEqual(net, naja.SNLOccurrence(leaf.getPath(), leaf.getNetComponent().getNet()))
      for term in terms:
        self.assertTrue(net.getPath().empty())
        self.assertEqual(net, naja.SNLOccurrence(term.getNet()))
    for kind, crossing, source, target in edges:
      self.assertIn(kind, (naja.SNLEquipotential.EdgeKind.Port,
                           naja.SNLEquipotential.EdgeKind.Assign))
      self.assertEqual(nodes[source][0], naja.SNLOccurrence(crossing.getPath(), crossing.getInstTerm().getNet()))
      self.assertLess(target, len(nodes))
    self.assertEqual(equi.getGraph(), equi.getGraph())
    original_graph = equi.getGraph()
    returned_nodes, returned_edges = equi.getGraph()
    for _, returned_leaves, returned_terms in returned_nodes:
      returned_leaves.clear()
      returned_terms.clear()
    returned_nodes.clear()
    returned_edges.clear()
    self.assertEqual(original_graph, equi.getGraph())
    return equi

  def testGraphUnconnected(self):
    equi = self.checkGraph(self.top_out, 0, 0)
    self.assertEqual(([], []), equi.getGraph())
    self.assertIsNone(naja.SNLEquipotential(self.top_out, keep_graph=False).getGraph())

  def testGraphTwoLevelsAndStartPoints(self):
    inner = naja.SNLDesign.create(self.model.getLibrary(), "inner")
    inner_port = naja.SNLScalarTerm.create(inner, naja.SNLTerm.Direction.Input, "p")
    upper = naja.SNLInstance.create(self.top, self.model, "upper")
    lower = naja.SNLInstance.create(self.model, inner, "lower")
    leaf = naja.SNLInstance.create(inner, self.submodel, "leaf")
    outer_net = naja.SNLScalarNet.create(self.top, "outer")
    middle_net = naja.SNLScalarNet.create(self.model, "middle")
    inner_net = naja.SNLScalarNet.create(inner, "inner")
    self.top_out.setNet(outer_net)
    upper.getInstTerm(self.i0).setNet(outer_net)
    self.i0.setNet(middle_net)
    lower.getInstTerm(inner_port).setNet(middle_net)
    inner_port.setNet(inner_net)
    leaf.getInstTerm(self.i0sub).setNet(inner_net)
    equi = self.checkGraph(self.top_out, 3, 2)
    nodes, edges = equi.getGraph()
    self.assertEqual([0, 1], [edge[2] for edge in edges])
    self.assertEqual([1, 2], [edge[3] for edge in edges])
    self.assertEqual([0, 1, 2], [node[0].getPath().size() for node in nodes])
    leaf_occ = naja.SNLOccurrence(
      naja.SNLPath(naja.SNLPath(upper), lower), leaf.getInstTerm(self.i0sub))
    for start in (leaf_occ, upper.getInstTerm(self.i0)):
      other = self.checkGraph(start, 3, 2)
      self.assertEqual(equi, other)
      self.assertCountEqual([n[0] for n in nodes], [n[0] for n in other.getGraph()[0]])
      self.assertCountEqual([e[1] for e in edges], [e[1] for e in other.getGraph()[1]])
    with tempfile.TemporaryDirectory() as directory:
      filename = os.path.join(directory, "graph.dot")
      equi.dumpDotFile(filename)
      with open(filename) as dot:
        contents = dot.read()
      self.assertIn("n0 -> n1", contents)
      self.assertIn("Port:", contents)
      self.assertIn("Leaf:", contents)
      with self.assertRaisesRegex(RuntimeError, "Cannot open"):
        equi.dumpDotFile(os.path.join(directory, "missing", "graph.dot"))

  def testGraphFeedthroughAndCycle(self):
    inside = naja.SNLScalarNet.create(self.model, "inside")
    self.i0.setNet(inside)
    self.o.setNet(inside)
    inst = naja.SNLInstance.create(self.top, self.model, "u")
    left = naja.SNLScalarNet.create(self.top, "left")
    right = naja.SNLScalarNet.create(self.top, "right")
    inst.getInstTerm(self.i0).setNet(left)
    inst.getInstTerm(self.o).setNet(right)
    self.checkGraph(inst.getInstTerm(self.i0), 3, 2)
    inst.getInstTerm(self.o).setNet(left)
    equi = self.checkGraph(inst.getInstTerm(self.i0), 2, 2)
    self.assertEqual(equi.getGraph()[1][0][2:], equi.getGraph()[1][1][2:])

  def testGraphBusAndSeparateInnerNets(self):
    a, b = self.i1.getBusTermBit(4), self.i1.getBusTermBit(1)
    na = naja.SNLScalarNet.create(self.model, "na")
    nb = naja.SNLScalarNet.create(self.model, "nb")
    a.setNet(na)
    b.setNet(nb)
    la = naja.SNLInstance.create(self.model, self.submodel, "la")
    lb = naja.SNLInstance.create(self.model, self.submodel, "lb")
    la.getInstTerm(self.i0sub).setNet(na)
    lb.getInstTerm(self.i0sub).setNet(nb)
    inst = naja.SNLInstance.create(self.top, self.model, "u")
    outer = naja.SNLScalarNet.create(self.top, "outer")
    inst.getInstTerm(a).setNet(outer)
    inst.getInstTerm(b).setNet(outer)
    equi = self.checkGraph(inst.getInstTerm(a), 3, 2)
    nodes, edges = equi.getGraph()
    for _, crossing, _, target in edges:
      bit = crossing.getInstTerm().getBitTerm()
      self.assertIsInstance(bit, naja.SNLBusTermBit)
      expected = la if bit == a else lb
      self.assertEqual(expected.getInstTerm(self.i0sub), nodes[target][1][0].getInstTerm())

  def testGraphSiblingsAndInternalNet(self):
    child = naja.SNLInstance.create(self.model, self.submodel, "child")
    inside = naja.SNLScalarNet.create(self.model, "inside")
    child.getInstTerm(self.i0sub).setNet(inside)
    a = naja.SNLInstance.create(self.top, self.model, "a")
    b = naja.SNLInstance.create(self.top, self.model, "b")
    start = naja.SNLOccurrence(naja.SNLPath(a), child.getInstTerm(self.i0sub))
    self.checkGraph(start, 1, 0)
    self.i0.setNet(inside)
    outer = naja.SNLScalarNet.create(self.top, "outer")
    a.getInstTerm(self.i0).setNet(outer)
    b.getInstTerm(self.i0).setNet(outer)
    equi = self.checkGraph(start, 3, 2)
    self.assertEqual(2, len(list(equi.getInstTermOccurrences())))

  def testErrors(self):
    ins = naja.SNLInstance.create(self.model, self.submodel, "ins")
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(0)
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(ins)
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(naja.SNLOccurrence(ins))
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(naja.SNLPath())
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(naja.SNLOccurrence())
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential(-1, -1, -1)
    with self.assertRaises(RuntimeError) as context: naja.SNLEquipotential()
    
if __name__ == '__main__':
  unittest.main()
