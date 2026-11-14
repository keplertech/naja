# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import sys
from types import ModuleType
import unittest
from unittest.mock import patch

import najaeda

EXPORTS = [
    "show",
    "diagnosis_response",
    "handle_request",
    "style_response",
    "export_svg",
    "export_svgs",
    "export_png",
    "RenderError",
    "__version__",
]
# Re-exported only when the installed naja-schematic has them.
OPTIONAL_EXPORTS = ["diff_svg", "diff_png"]


class NajaedaSchematicTest(unittest.TestCase):
    def setUp(self):
        self.previous_module = sys.modules.pop("najaeda.schematic", None)
        self.previous_attribute = najaeda.__dict__.pop("schematic", None)

    def tearDown(self):
        sys.modules.pop("najaeda.schematic", None)
        najaeda.__dict__.pop("schematic", None)
        if self.previous_module is not None:
            sys.modules["najaeda.schematic"] = self.previous_module
        if self.previous_attribute is not None:
            najaeda.schematic = self.previous_attribute

    def test_missing_viewer(self):
        with patch.dict(sys.modules, {"naja_schematic": None}):
            with self.assertRaisesRegex(ImportError, r"najaeda\[schematic\]") as error:
                importlib.import_module("najaeda.schematic")
        self.assertIsInstance(error.exception.__cause__, ImportError)

    def test_public_api(self):
        try:
            import naja_schematic
        except ImportError:
            self.skipTest("naja-schematic is not installed")
        schematic = importlib.import_module("najaeda.schematic")
        for name in EXPORTS:
            with self.subTest(name=name):
                self.assertIs(getattr(schematic, name), getattr(naja_schematic, name))
        for name in OPTIONAL_EXPORTS:
            with self.subTest(name=name):
                self.assertEqual(hasattr(schematic, name), hasattr(naja_schematic, name))
                self.assertEqual(name in schematic.__all__, hasattr(naja_schematic, name))

    def _stub_viewer(self, names):
        viewer = ModuleType("naja_schematic")
        for name in names:
            setattr(viewer, name, object())
        return viewer

    def test_public_api_with_stub_viewer(self):
        viewer = self._stub_viewer(EXPORTS)
        with patch.dict(sys.modules, {"naja_schematic": viewer}):
            schematic = importlib.import_module("najaeda.schematic")
            self.assertEqual(schematic.__all__, EXPORTS)
            for name in EXPORTS:
                with self.subTest(name=name):
                    self.assertIs(getattr(schematic, name), getattr(viewer, name))
            for name in OPTIONAL_EXPORTS:
                self.assertFalse(hasattr(schematic, name))

    def test_diff_exports_with_a_viewer_that_has_them(self):
        viewer = self._stub_viewer(EXPORTS + OPTIONAL_EXPORTS)
        with patch.dict(sys.modules, {"naja_schematic": viewer}):
            schematic = importlib.import_module("najaeda.schematic")
            self.assertEqual(schematic.__all__, EXPORTS + OPTIONAL_EXPORTS)
            for name in OPTIONAL_EXPORTS:
                with self.subTest(name=name):
                    self.assertIs(getattr(schematic, name), getattr(viewer, name))

    def test_older_viewer_gets_the_install_hint(self):
        # A naja-schematic before 0.3.0: no headless export.
        viewer = self._stub_viewer(["show", "diagnosis_response", "handle_request", "__version__"])
        with patch.dict(sys.modules, {"naja_schematic": viewer}):
            with self.assertRaisesRegex(ImportError, r"najaeda\[schematic\]"):
                importlib.import_module("najaeda.schematic")


if __name__ == "__main__":
    unittest.main()
