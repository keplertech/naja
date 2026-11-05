# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

import unittest
import sys

import naja


class NLClockTests(unittest.TestCase):
    def setUp(self):
        self.universe = naja.NLUniverse.create()
        self.db = naja.NLDB.create(self.universe)
        library = naja.NLLibrary.create(self.db)
        self.top = naja.SNLDesign.create(library, "top")
        self.clk = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Input, "clk")
        self.clk2 = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Input, "clk2")
        self.model = naja.SNLDesign.create(library, "model")
        self.model_clk = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "clk")
        self.instance = naja.SNLInstance.create(self.top, self.model, "inst")

    def tearDown(self):
        if naja.NLUniverse.get() is not None:
            naja.NLUniverse.get().destroy()

    def test_primary_clock(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        self.assertEqual("sys", clock.getName())
        self.assertEqual("primary", clock.getKind())
        self.assertEqual(self.top.getName(), clock.getDesign().getName())
        self.assertEqual(10.0, clock.getPeriod())
        self.assertAlmostEqual(0.1, clock.getFrequency())
        self.assertEqual("ns", clock.getTimeUnit())
        self.assertEqual(0.0, clock.getRiseAt())
        self.assertEqual(5.0, clock.getFallAt())
        self.assertIsNone(clock.getMaster())
        self.assertIsNone(clock.getMasterSource())
        self.assertEqual(1, clock.getDivideBy())
        self.assertEqual(1, clock.getMultiplyBy())
        self.assertFalse(clock.isInverted())
        self.assertEqual("sys", clock.getRootClock().getName())
        self.assertTrue(clock.isSynchronousWith(clock))
        sources = clock.getSources()
        self.assertEqual(1, len(sources))
        self.assertEqual("clk", sources[0].getName())

    def test_set_waveform(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        clock.setWaveform(2.0, 7.0)
        self.assertEqual(2.0, clock.getRiseAt())
        self.assertEqual(7.0, clock.getFallAt())
        with self.assertRaises(RuntimeError):
            clock.setWaveform(5.0, 2.0)
        self.assertEqual(7.0, clock.getFallAt())

    def test_generated_clock(self):
        master = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        divided = naja.NLClock.createGenerated(
            self.top, "div2", master, self.clk, 2, 1, False, [self.clk2])
        self.assertEqual("generated", divided.getKind())
        self.assertEqual(master.getName(), divided.getMaster().getName())
        self.assertEqual("clk", divided.getMasterSource().getName())
        self.assertEqual(20.0, divided.getPeriod())
        self.assertAlmostEqual(0.05, divided.getFrequency())
        self.assertEqual(2, divided.getDivideBy())
        self.assertEqual("sys", divided.getRootClock().getName())
        self.assertTrue(divided.isSynchronousWith(master))

    def test_unrelated_clocks_are_not_synchronous(self):
        sys_clock = naja.NLClock.createPrimary(self.top, "sys", 20.0, "ns", [self.clk])
        other = naja.NLClock.createPrimary(self.top, "other", 20.0, "ns", [self.clk2])
        self.assertFalse(sys_clock.isSynchronousWith(other))

    def test_generated_waveforms(self):
        master = naja.NLClock.createPrimary(self.top, "root", 12.0, "ps", [self.clk])
        master.setWaveform(3.0, 7.0)
        for divide, multiply, period, high in (
                (1, 1, 12, 4), (2, 1, 24, 12), (3, 1, 36, 12),
                (6, 1, 72, 36), (1, 2, 6, 2), (3, 2, 18, 6), (2, 3, 8, 4)):
            for invert in (False, True):
                with self.subTest(divide=divide, multiply=multiply, invert=invert):
                    clock = naja.NLClock.createGenerated(
                        self.top, f"g{divide}_{multiply}_{invert}", master,
                        self.clk, divide, multiply, invert, [self.clk2, self.clk])
                    self.assertEqual(period, clock.getPeriod())
                    self.assertEqual("ps", clock.getTimeUnit())
                    self.assertEqual(3 + high if invert else 3, clock.getRiseAt())
                    self.assertEqual(3 + period if invert else 3 + high, clock.getFallAt())
                    self.assertEqual([self.clk2, self.clk], clock.getSources())
                    self.assertEqual(master, clock.getMaster())
                    with self.assertRaisesRegex(RuntimeError, "only supported on primary clocks"):
                        clock.setWaveform(0, period / 2)
        self.assertEqual((3, 7), (master.getRiseAt(), master.getFallAt()))

    def test_generated_chain_tracks_master_waveform(self):
        root = naja.NLClock.createPrimary(self.top, "root", 12.0, "ns", [self.clk])
        root.setWaveform(3, 7)
        inverted = naja.NLClock.createGenerated(
            self.top, "inverted", root, self.clk, 1, 1, True, [self.clk2])
        divided = naja.NLClock.createGenerated(
            self.top, "divided", inverted, self.clk2, 3, 1, False, [self.clk])
        multiplied = naja.NLClock.createGenerated(
            self.top, "multiplied", divided, self.clk, 1, 2, True, [self.clk2])
        for rise, fall, expected in (
                (3, 7, [(7, 15), (7, 31), (19, 25)]),
                (1, 9, [(9, 13), (9, 21), (15, 27)]),
                (0, 12, [(12, 12), (12, 12), (12, 30)])):
            root.setWaveform(rise, fall)
            for clock, period, edges in zip(
                    (inverted, divided, multiplied), (12, 36, 18), expected):
                self.assertEqual(period, clock.getPeriod())
                self.assertAlmostEqual(1 / period, clock.getFrequency())
                self.assertEqual(edges, (clock.getRiseAt(), clock.getFallAt()))
                self.assertEqual("ns", clock.getTimeUnit())
                self.assertEqual(root, clock.getRootClock())
                self.assertTrue(clock.isSynchronousWith(root))

    def test_even_divider_tracks_master_phase(self):
        root = naja.NLClock.createPrimary(self.top, "root", 12.0, "ns", [self.clk])
        divided = naja.NLClock.createGenerated(
            self.top, "divided", root, self.clk, 2, 1, False, [self.clk2])
        for rise, fall in ((2, 5), (4, 11)):
            root.setWaveform(rise, fall)
            self.assertEqual((rise, rise + 12), (divided.getRiseAt(), divided.getFallAt()))

    def test_derived_range_errors_are_python_exceptions(self):
        period = sys.float_info.max
        root = naja.NLClock.createPrimary(self.top, "root", period, "ns", [self.clk])
        with self.assertRaisesRegex(RuntimeError, "derived period and waveform must be finite"):
            naja.NLClock.createGenerated(
                self.top, "overflow", root, self.clk, 2, 1, False, [self.clk2])
        self.assertEqual([root], list(self.top.getClocks()))
        inverted = naja.NLClock.createGenerated(
            self.top, "inverted", root, self.clk, 1, 1, True, [self.clk2])
        root.setWaveform(period / 2, period)
        for getter in (inverted.getPeriod, inverted.getFrequency,
                       inverted.getRiseAt, inverted.getFallAt):
            with self.assertRaisesRegex(RuntimeError, "derived period and waveform must be finite"):
                getter()
        root.setWaveform(0, period / 2)
        self.assertEqual(period, inverted.getFallAt())

    def test_multiple_sources(self):
        clock = naja.NLClock.createPrimary(
            self.top, "sys", 10.0, "ns", [self.clk, self.clk2])
        self.assertEqual([self.clk, self.clk2], clock.getSources())

    def test_primary_binding_errors(self):
        valid = (self.top, "sys", 10.0, "ns", [self.clk])
        cases = [
            ((), "malformed NLClock primary creation method"),
            ((self.top, "sys", "bad", "ns", [self.clk]),
             "malformed NLClock primary creation method"),
            ((None, *valid[1:]), "accepts SNLDesign as first argument"),
            ((*valid[:4], [self.clk, None]), "sources must be SNL terminals"),
        ]
        for args, message in cases:
            with self.subTest(args=args):
                with self.assertRaisesRegex(RuntimeError, message):
                    naja.NLClock.createPrimary(*args)
                self.assertEqual([], list(self.top.getClocks()))

    def test_generated_binding_errors(self):
        master = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        valid = (self.top, "derived", master, self.clk, 2, 1, False, [self.clk2])
        cases = [
            ((), "malformed NLClock generated creation method"),
            ((*valid[:4], "bad", *valid[5:]),
             "malformed NLClock generated creation method"),
            ((None, *valid[1:]), "accepts SNLDesign as first argument"),
            ((*valid[:2], None, *valid[3:]), "expects a master NLClock"),
            ((*valid[:3], None, *valid[4:]), "expects an SNL terminal as master source"),
            ((*valid[:7], (self.clk2,)), "sources must be a list of terminals"),
            ((*valid[:7], [self.clk2, None]), "sources must be SNL terminals"),
        ]
        for args, message in cases:
            with self.subTest(args=args):
                with self.assertRaisesRegex(RuntimeError, message):
                    naja.NLClock.createGenerated(*args)
                self.assertEqual([master], list(self.top.getClocks()))

    def test_waveform_binding_errors(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        for args in ((), (1.0,), (1.0, 2.0, 3.0), ("bad", 2.0)):
            with self.subTest(args=args):
                with self.assertRaisesRegex(RuntimeError, "expects \\(rise_at, fall_at\\)"):
                    clock.setWaveform(*args)
                self.assertEqual((0.0, 5.0), (clock.getRiseAt(), clock.getFallAt()))

    def test_synchronous_binding_errors(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        for other in (None, self.clk, "sys"):
            with self.subTest(other=other):
                with self.assertRaisesRegex(RuntimeError, "expects an NLClock"):
                    clock.isSynchronousWith(other)

    def test_rejects_instance_terminal_source(self):
        inst_term = self.instance.getInstTerm(self.model_clk)
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [inst_term])

    def test_rejects_invalid_arguments(self):
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 0.0, "ns", [self.clk])
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [])
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", "clk")

    def test_design_and_db_expose_clocks(self):
        sys_clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        design_clocks = list(self.top.getClocks())
        self.assertEqual(1, len(design_clocks))
        self.assertEqual("sys", design_clocks[0].getName())
        db_clocks = list(self.db.getClocks())
        self.assertEqual(1, len(db_clocks))
        self.assertEqual(sys_clock.getName(), db_clocks[0].getName())

    def test_destroy_removes_clock(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        clock.destroy()
        self.assertEqual(0, sum(1 for _ in self.top.getClocks()))
        self.assertEqual(0, sum(1 for _ in self.db.getClocks()))


if __name__ == "__main__":
    unittest.main()
