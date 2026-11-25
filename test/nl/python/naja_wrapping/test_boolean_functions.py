# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import naja


def named(expression):
    if isinstance(expression, bool):
        return expression
    if expression[0] == "term":
        return ("term", expression[1].getName())
    return (expression[0], *(named(child) for child in expression[1:]))


class BooleanFunctionTest(unittest.TestCase):
    def setUp(self):
        self.db = naja.NLDB.create(naja.NLUniverse.create())
        self.primitives = naja.NLLibrary.createPrimitives(self.db)

    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()

    def primitive(self, name="logic"):
        design = naja.SNLDesign.createPrimitive(self.primitives, name)
        p = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "P")
        # Interleave output and inputs to exercise flat-term dependency mapping.
        y = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "Y")
        q = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "Q")
        return design, p, q, y

    def test_validation(self):
        design, p, q, y = self.primitive()
        expression = ("and", ("term", p), ("term", q))
        self.assertIsNone(y.getFunction())
        with self.assertRaises(RuntimeError):
            y.setFunction(expression)
        design.setTruthTable(8)
        y.setFunction(expression)
        self.assertEqual(expression, y.getFunction())
        other, foreign, _, _ = self.primitive("other")
        for invalid in (("not", expression), ("state", 0), ("term", y),
                        ("term", foreign), ("and", ("term", p)), ("unknown", True), 42):
            with self.subTest(invalid=invalid), self.assertRaises(RuntimeError):
                y.setFunction(invalid)
        self.assertEqual(expression, y.getFunction())
        with self.assertRaises(RuntimeError):
            p.setFunction(expression)
        design.addInputsToClockArcs([p], q)
        self.assertIsNone(y.getFunction())
        with self.assertRaises(RuntimeError):
            y.setFunction(expression)
        other_output = other.getScalarTerm("Y")
        parameter = naja.SNLParameter.createBinary(other, "INIT", 4, 8)
        other.setTruthTableFromParameter(other_output, [foreign, other.getScalarTerm("Q")], parameter)
        with self.assertRaises(RuntimeError):
            other_output.setFunction(("term", foreign))

    def test_python_primitive_and_grouping(self):
        design, p, q, y = self.primitive()
        design.setTruthTable(8)
        y.setFunction(("and", ("and", ("term", p), True), ("term", q)))
        self.assertEqual(("and", ("term", p), True, ("term", q)), y.getFunction())
        y.setFunction("!!(P*Q)")
        self.assertEqual(("not", ("not", ("and", ("term", p), ("term", q)))), y.getFunction())

    def load_fixture(self):
        formats = Path(os.environ.get("FORMATS_PATH", Path(__file__).resolve().parents[2] / "formats"))
        self.db.loadLibertyPrimitives([str(formats / "liberty/benchmarks/tests/written_functions.lib")])
        return self.db.getLibrary("written_functions")

    def test_liberty_exact_trees_and_tables(self):
        library = self.load_fixture()
        t = lambda name: ("term", name)
        p, q, r, s = map(t, "PQRS")
        expected = {
            "bubble": (("not", ("and", ("not", p), q)), 0xb),
            "product": (("and", p, ("not", q)), 0x2),
            "nor": (("not", ("or", p, ("not", q))), 0x4),
            "aoi": (("not", ("or", ("and", p, q), r)), 0x7),
            "oai": (("not", ("and", ("or", p, q), ("or", r, s))), 0x111f),
            "parity": (("xor", p, q, r), 0x96),
            "buffer": (p, 0x2), "inverter": (("not", p), 0x1),
            "postfix": (("or", ("and", p, ("not", q)), r), 0xf2),
            "double": (("not", ("not", p)), 0x2),
            "constant": (("and", p, True), 0x2),
        }
        for name, (tree, mask) in expected.items():
            with self.subTest(cell=name):
                cell = library.getSNLDesign(name)
                self.assertEqual(tree, named(cell.getScalarTerm("Y").getFunction()))
                self.assertEqual(mask, cell.getTruthTable()[1])
        pair = library.getSNLDesign("pair")
        self.assertEqual(("and", p, q), named(pair.getScalarTerm("Y").getFunction()))
        self.assertEqual(("not", ("or", p, q)), named(pair.getScalarTerm("Z").getFunction()))
        self.assertEqual([2, 8], pair.getTruthTableByOutputID(pair.getScalarTerm("Y").getID()))
        self.assertEqual([2, 1], pair.getTruthTableByOutputID(pair.getScalarTerm("Z").getID()))

    def test_snapshot_omits_functions(self):
        library = self.load_fixture()
        # Include a directly constructed primitive, with interleaved pin directions.
        design, p, q, y = self.primitive()
        design.setTruthTable(8)
        y.setFunction(("and", ("term", p), ("term", q)))
        top = naja.SNLDesign.create(naja.NLLibrary.create(self.db), "top")
        naja.SNLInstance.create(top, design, "u")
        naja.NLUniverse.get().setTopDesign(top)
        expected = {}
        for lib in self.db.getLibraries():
            for cell in lib.getSNLDesigns():
                for term in cell.getBitTerms():
                    if term.getFunction() is not None:
                        expected[f"{lib.getName()}/{cell.getName()}/{term.getName()}"] = cell.getTruthTableByOutputID(term.getID())
        with tempfile.TemporaryDirectory() as directory:
            snapshot = str(Path(directory) / "snapshot")
            self.db.dumpNajaIF(snapshot)
            script = '''
import json, sys, naja
assert naja.NLUniverse.get() is None
db = naja.NLDB.loadNajaIF(sys.argv[1])
result = {}
for lib in db.getLibraries():
    for cell in lib.getSNLDesigns():
        for term in cell.getBitTerms():
            assert term.getFunction() is None
            if term.getDirection() == naja.SNLTerm.Direction.Output:
                result[f"{lib.getName()}/{cell.getName()}/{term.getName()}"] = cell.getTruthTableByOutputID(term.getID())
print(json.dumps(result, sort_keys=True))
'''
            environment = dict(os.environ)
            environment["PYTHONPATH"] = str(Path(__file__).parent) + os.pathsep + environment.get("PYTHONPATH", "")
            output = subprocess.check_output([sys.executable, "-c", script, snapshot], env=environment, text=True)
            self.assertEqual(json.loads(json.dumps(expected)), json.loads(output))
