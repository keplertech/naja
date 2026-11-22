# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
from pathlib import Path
import naja


class SNLBundleTermTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    self.db = naja.NLDB.create(universe)

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  @staticmethod
  def _bundle_issue_120_lib():
    return (
      Path(__file__).resolve().parents[4]
      / "test"
      / "nl"
      / "formats"
      / "liberty"
      / "benchmarks"
      / "tests"
      / "bundle_issue_120.lib"
    )

  def _get_issue_120_primitive(self):
    self.db.loadLibertyPrimitives([str(self._bundle_issue_120_lib())])
    primitive_libs = list(self.db.getPrimitiveLibraries())
    self.assertEqual(1, len(primitive_libs))
    primitive = primitive_libs[0].getSNLDesign("cell_def")
    self.assertIsNotNone(primitive)
    return primitive

  def test_bundle_members_and_lookup(self):
    design = self._get_issue_120_primitive()
    d_bundle = design.getBundleTerm("D")
    qn_bundle = design.getBundleTerm("QN")
    d0 = design.getScalarTerm("D0")
    d1 = design.getScalarTerm("D1")
    qn0 = design.getScalarTerm("QN0")
    qn1 = design.getScalarTerm("QN1")

    self.assertIsInstance(d_bundle, naja.SNLBundleTerm)
    occurrence = naja.SNLOccurrence(d_bundle)
    self.assertIs(occurrence.getObject(), d_bundle)
    self.assertIs(occurrence.getNetComponent(), d_bundle)
    self.assertIsInstance(qn_bundle, naja.SNLBundleTerm)
    self.assertEqual(["CK", "SE", "SI", "D", "QN"], [term.getName() for term in design.getTerms()])
    self.assertEqual(["CK", "SE", "SI"], [term.getName() for term in design.getScalarTerms()])
    self.assertEqual(["D", "QN"], [term.getName() for term in design.getBundleTerms()])
    self.assertEqual(d_bundle, design.getBundleTerm("D"))
    self.assertEqual(qn_bundle, design.getBundleTerm("QN"))
    self.assertEqual(d0, d_bundle.getMember(0))
    self.assertEqual(d1, d_bundle.getMember(1))
    self.assertEqual(qn0, qn_bundle.getMember(0))
    self.assertEqual(qn1, qn_bundle.getMember(1))
    self.assertIsNone(d_bundle.getMember(2))
    with self.assertRaises(RuntimeError) as context:
      d_bundle.getMember("0")
    self.assertIn(
      "SNLBundleTerm.getMember() expects an integer argument",
      str(context.exception),
    )
    self.assertEqual(2, d_bundle.getNumMembers())
    self.assertEqual(2, qn_bundle.getNumMembers())
    self.assertEqual(d0.getFlatID(), d_bundle.getFlatID())
    self.assertEqual(qn0.getFlatID(), qn_bundle.getFlatID())
    self.assertEqual(["D0", "D1"], [member.getName() for member in d_bundle.getMembers()])
    self.assertEqual(["QN0", "QN1"], [member.getName() for member in qn_bundle.getMembers()])
    self.assertEqual(
      ["CK", "SE", "SI", "D0", "D1", "QN0", "QN1"],
      [term.getName() for term in design.getBitTerms()],
    )

  def test_bundle_creation_is_not_exposed(self):
    design = self._get_issue_120_primitive()
    bundle = design.getBundleTerm("D")

    self.assertFalse(hasattr(naja.SNLBundleTerm, "create"))
    with self.assertRaises(RuntimeError):
      naja.SNLScalarTerm.create(bundle, naja.SNLTerm.Direction.Input, "D2")
    with self.assertRaises(RuntimeError):
      naja.SNLBusTerm.create(bundle, naja.SNLTerm.Direction.Input, 1, 0, "D3")


if __name__ == "__main__":
  unittest.main()
