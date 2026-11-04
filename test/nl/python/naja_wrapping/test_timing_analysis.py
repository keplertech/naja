# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import gc
import os
from pathlib import Path
import tempfile
import time
import unittest

import naja


class TimingAnalysisTests(unittest.TestCase):
    def setUp(self):
        self.universe = naja.NLUniverse.create()
        self.db = naja.NLDB.create(self.universe)

    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()

    def load(self, source):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.sv"
            path.write_text(source)
            top = self.db.loadSystemVerilog(
                [str(path)], blackbox_unknown_modules=True,
                diagnostics_report_path=None)
        self.universe.setTopDesign(top)
        return top

    def primary(self, design, name, term):
        return naja.NLClock.createPrimary(
            design, name, 10.0, "ns", [design.getScalarTerm(term)])

    def analyze(self):
        top = self.universe.getTopDesign()
        self.assertIsNone(top.analyzeTiming())
        return top

    def crossing(self):
        top = self.load('''module sample(input a, b, d, output reg q, r);
            always @(posedge a) q <= d;
            always @(posedge b) r <= q;
        endmodule''')
        self.primary(top, "first", "a")
        self.primary(top, "second", "b")
        return top

    def test_top_clock_and_crossing_endpoints(self):
        top = self.crossing()
        self.analyze()
        first, second = top.getClockDomains()
        self.assertEqual({first, second}, set(top.getClockDomains()))
        self.assertEqual("first", first.getNLClock().getName())
        source = first.getSources()[0]
        self.assertEqual(naja.SNLOccurrence(top.getScalarTerm("a")), source)
        self.assertEqual({first}, source.getDrivingClocks())
        endpoints = top.getCaptureEndpoints()
        self.assertEqual(2, len(endpoints))
        crossings = [e for e in endpoints if e.getDrivingClocks() == {first}
                     and e.getReceivingClocks() == {second}]
        self.assertEqual(1, len(crossings))
        self.assertIsInstance(crossings[0], naja.SNLOccurrence)
        self.assertEqual([], top.getTimingDiagnostics()["unclockedClockPins"])

    def test_unknown_pll_clock_uses_its_output(self):
        top = self.load("""module sample(input clk, d, output reg q);
            wire derived;
            pll source(.ref_i(clk), .clk_o(derived));
            always @(posedge derived) q <= d;
        endmodule""")
        self.primary(top, "core", "clk")
        pll = top.getInstance("source")
        declaration = self.primary(pll.getModel(), "pll_clock", "clk_o")
        self.analyze()
        domain = next(c for c in top.getClockDomains() if c.getNLClock() == declaration)
        source = domain.getSources()[0]
        self.assertEqual(pll.getInstTerm(pll.getModel().getScalarTerm("clk_o")), source.getInstTerm())
        self.assertEqual({domain}, source.getDrivingClocks())
        self.assertEqual({domain}, source.getWaveformClocks())
        self.assertEqual({domain}, top.getCaptureEndpoints()[0].getReceivingClocks())
        self.assertEqual([], top.getTimingDiagnostics()["unclockedClockPins"])

    def test_shared_model_clocks_have_distinct_hierarchy(self):
        top = self.load("""module leaf(input c, d, output reg q);
            always @(posedge c) q <= d;
        endmodule
        module branch(input c, d, output q); leaf cell0(c, d, q); endmodule
        module sample(input a, b, d, output q, r);
            branch left(a, d, q); branch right(b, d, r);
        endmodule""")
        model = top.getInstance("left").getModel().getInstance("cell0").getModel()
        declaration = self.primary(model, "shared", "c")
        self.analyze()
        domains = top.getClockDomains()
        self.assertEqual(2, len(domains))
        self.assertEqual([declaration, declaration], [c.getNLClock() for c in domains])
        left, right = [c.getSources()[0] for c in domains]
        self.assertNotEqual(left, right)
        self.assertNotEqual(str(left.getPath()), str(right.getPath()))
        self.assertEqual({frozenset([c]) for c in domains},
                         {e.getReceivingClocks() for e in top.getCaptureEndpoints()})

    def test_generated_declaration_preserves_master(self):
        top = self.load("""module sample(input c, derived, d, output reg q);
            always @(posedge derived) q <= d;
        endmodule""")
        master = self.primary(top, "master", "c")
        generated = naja.NLClock.createGenerated(top, "divided", master,
            top.getScalarTerm("c"), 3, 1, False, [top.getScalarTerm("derived")])
        self.analyze()
        domain = next(c for c in top.getClockDomains() if c.getNLClock() == generated)
        self.assertEqual(master, domain.getNLClock().getMaster())
        self.assertTrue(domain.getNLClock().isSynchronousWith(master))
        self.assertEqual({domain}, top.getCaptureEndpoints()[0].getReceivingClocks())

    def test_divided_clock_diagnostic(self):
        top = self.load("""module sample(input c, d, output reg q, r);
            always @(posedge c) q <= ~q;
            always @(posedge q) r <= d;
        endmodule""")
        self.primary(top, "master", "c")
        self.analyze()
        domain = top.getClockDomains()[0]
        diagnostics = top.getTimingDiagnostics()
        self.assertEqual(1, len(diagnostics["generatedClocks"]))
        generated = diagnostics["generatedClocks"][0]
        self.assertEqual({domain}, generated["clocks"])
        self.assertEqual({domain}, generated["output"].getWaveformClocks())
        self.assertEqual({domain}, generated["output"].getDrivingClocks())
        self.assertEqual([], diagnostics["unclockedClockPins"])
        self.assertTrue(all(e.getReceivingClocks() == {domain} for e in top.getCaptureEndpoints()))

    def test_blackbox_shared_pins_have_no_cycle(self):
        top = self.load("""module sample(input clk, reset_n, d, output reg q, r);
            wire divided;
            opaque left(.c(clk), .r(reset_n));
            opaque right(.c(clk), .r(reset_n));
            pll source(.ref_i(clk), .clk_o(divided));
            always @(posedge clk) q <= d;
            always @(posedge divided) r <= q;
        endmodule""")
        self.primary(top, "root", "clk")
        self.primary(top.getInstance("source").getModel(), "pll", "clk_o")
        self.analyze()
        self.assertEqual(3, top.getTimingCoverageCounts()["blackbox"])
        self.assertEqual([], top.getTimingDiagnostics()["unclockedClockPins"])
        self.assertEqual(2, len(top.getCaptureEndpoints()))

    def test_modeled_cycle_exception(self):
        designs = naja.NLLibrary.create(self.db)
        primitives = naja.NLLibrary.createPrimitives(self.db)
        top = naja.SNLDesign.create(designs, "sample")
        model = naja.SNLDesign.createPrimitive(primitives, "buffer")
        a = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "a")
        b = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Output, "b")
        naja.SNLDesign.addCombinatorialArcs([a], [b])
        left = naja.SNLInstance.create(top, model, "left")
        right = naja.SNLInstance.create(top, model, "right")
        for name, driver, load in (("n0", left, right), ("n1", right, left)):
            net = naja.SNLScalarNet.create(top, name)
            driver.getInstTerm(b).setNet(net)
            load.getInstTerm(a).setNet(net)
        self.universe.setTopDesign(top)
        with self.assertRaises(naja.SNLTimingCycleError) as caught:
            top.analyzeTiming()
        self.assertIsInstance(caught.exception.occurrence, naja.SNLOccurrence)
        self.assertTrue(caught.exception.description)

    def test_coverage_memory_latch_blackbox(self):
        top = self.load('''module sample(input c, d, output q);
            opaque source(.c(c), .q(q));
        endmodule''')
        primitives = naja.NLLibrary.createPrimitives(self.db)
        latch = naja.SNLDesign.createPrimitive(primitives, "latch")
        c = naja.SNLScalarTerm.create(latch, naja.SNLTerm.Direction.Input, "c")
        d = naja.SNLScalarTerm.create(latch, naja.SNLTerm.Direction.Input, "d")
        q = naja.SNLScalarTerm.create(latch, naja.SNLTerm.Direction.Output, "q")
        latch.setSequentialModel(kind="latch", clocked_on="c",
            states=[dict(name="state", next_state="d", clear="d")],
            outputs=[(q, "state")])
        naja.SNLInstance.create(top, latch, "storage")
        # Exercise opaque-memory coverage with an unsupported dependency
        # added to an otherwise valid Liberty memory model.
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "memory.lib"
            path.write_text("""library(synthetic) {
                type(bits) { base_type : array; data_type : bit;
                    bit_width : 1; bit_from : 0; bit_to : 0; downto : true; }
                cell(memory) {
                    memory() { type : ram; address_width : 1; word_width : 1; }
                    pin(clk) { direction : input; clock : true; }
                    bus(a) { bus_type : bits; direction : input; }
                    bus(q) { bus_type : bits; direction : output;
                        memory_read() { address : a; }
                        timing() { related_pin : "clk"; timing_type : rising_edge; }
                    }
                }
            }""")
            self.db.loadLibertyPrimitives([str(path)])
        memory = self.db.getLibrary("synthetic").getSNLDesign("memory")
        # A clock-to-read-data combinational dependency is outside the
        # declared asynchronous address dependency contract.
        naja.SNLDesign.addCombinatorialArcs(
            [memory.getScalarTerm("clk")], [memory.getBusTerm("q").getBusTermBit(0)])
        naja.SNLInstance.create(top, memory, "array")
        self.analyze()
        report = top.getTimingCoverageReport()
        self.assertEqual({"blackbox", "unsupported_latch", "memory"}, {r["reason"] for r in report})
        for row in report:
            self.assertIsInstance(row["occurrence"], naja.SNLOccurrence)
            self.assertTrue(row["model"])
            self.assertTrue(row["detail"])
            self.assertEqual("opaque", row["treatment"])
        self.assertEqual(3, sum(top.getTimingCoverageCounts().values()))

    def test_latch_and_memory_state_endpoints(self):
        top = self.load('''module sample(input c, g, e, d, a, output q, output reg l);
            reg storage [0:1];
            always @(posedge c) if (e) storage[a] <= d;
            assign q = storage[a];
            always_latch if (g) l <= d;
        endmodule''')
        self.primary(top, "clock", "c")
        self.primary(top, "gate", "g")
        self.analyze()
        clocks = {c.getNLClock().getName(): c for c in top.getClockDomains()}
        endpoints = top.getCaptureEndpoints()
        memory_inputs = [e for e in endpoints
                         if e.getInstTerm().getInstance().getModel().hasMemoryInterface()]
        self.assertGreaterEqual(len(memory_inputs), 3)
        self.assertTrue(all(e.getReceivingClocks() == {clocks["clock"]} for e in memory_inputs))
        self.assertTrue(any(e.getReceivingClocks() == {clocks["gate"]} for e in endpoints))
        self.assertIn(("unsupported_latch", "approximated"),
                      {(row["reason"], row["treatment"])
                       for row in top.getTimingCoverageReport()})

    def test_late_declaration_and_rebuild(self):
        top = self.crossing()
        self.analyze()
        endpoint = top.getCaptureEndpoints()[0]
        naja.NLClock.createPrimary(top, "late", 15.0, "ns", [top.getScalarTerm("d")])
        for query in (top.getClockDomains, lambda: endpoint.getDrivingClocks()):
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()
        top.analyzeTiming()
        self.assertEqual(3, len(top.getClockDomains()))
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            endpoint.getReceivingClocks()

    def test_destroy_and_recreate_universe_invalidates(self):
        top = self.crossing()
        self.analyze()
        domain = top.getClockDomains()[0]
        endpoint = top.getCaptureEndpoints()[0]
        bound = endpoint.getPath
        self.universe.destroy()
        naja.NLUniverse.create()
        for query in (domain.getNLClock, bound, lambda: endpoint.getDrivingClocks(),
                      lambda: hash(endpoint), lambda: str(endpoint)):
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()
        with self.assertRaises(RuntimeError):
            top.getCaptureEndpoints()

    def test_rebuild_invalidates_old_views(self):
        top = self.crossing()
        self.analyze()
        domain = top.getClockDomains()[0]
        endpoint = top.getCaptureEndpoints()[0]
        top.analyzeTiming()
        for query in (domain.getNLClock, lambda: endpoint.getReceivingClocks()):
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()
        self.assertEqual(2, len(top.getCaptureEndpoints()))

    def test_edit_refreshes_connectivity(self):
        top = self.crossing()
        self.analyze()
        first, second = top.getClockDomains()
        output = naja.SNLOccurrence(top.getScalarTerm("q"))
        self.assertEqual({first}, output.getDrivingClocks())
        self.assertEqual(frozenset(), output.getReceivingClocks())
        self.assertEqual({second}, naja.SNLOccurrence(top.getScalarTerm("q").getNet()).getReceivingClocks())
        top.getScalarTerm("q").setNet(None)
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            output.getDrivingClocks()
        top.analyzeTiming()
        self.assertEqual(frozenset(), output.getDrivingClocks())

    def test_native_context_outlives_python_design_wrapper(self):
        top = self.crossing()
        self.analyze()
        endpoint = top.getCaptureEndpoints()[0]
        del top
        gc.collect()
        self.assertEqual(1, len(endpoint.getReceivingClocks()))

    def test_missing_analysis_and_hidden_graph(self):
        top = self.crossing()
        occurrence = naja.SNLOccurrence(top.getScalarTerm("q"))
        with self.assertRaisesRegex(RuntimeError, "analyzeTiming"):
            occurrence.getDrivingClocks()
        self.analyze()
        for name in ("TGraph", "TSNLGraphConstructor", "TClockPropagator", "TNode", "TEdge", "TClock"):
            self.assertFalse(hasattr(naja, name))
        self.assertFalse(hasattr(top, "getGraph"))
        with self.assertRaises(TypeError):
            naja.SNLClockDomain()
        with self.assertRaises(RuntimeError):
            naja.SNLOccurrence().getDrivingClocks()
        with self.assertRaises(RuntimeError):
            naja.SNLOccurrence(top.getInstance(next(iter(top.getInstances())).getName())).getDrivingClocks()

    def test_unclocked_pins(self):
        top = self.load("module sample(input c, d, output reg q); always @(posedge c) q <= d; endmodule")
        self.analyze()
        pins = top.getTimingDiagnostics()["unclockedClockPins"]
        self.assertEqual(1, len(pins))
        self.assertIsInstance(pins[0], naja.SNLOccurrence)
        self.assertEqual(frozenset(), pins[0].getWaveformClocks())
        self.assertEqual(frozenset(), top.getCaptureEndpoints()[0].getReceivingClocks())

    def test_control_diagnostics(self):
        top = self.load("""module sample(input c, d, output reg q, r);
            wire gated = c & q;
            always @(posedge c) q <= d;
            always @(posedge gated) r <= d;
        endmodule""")
        self.primary(top, "root", "c")
        self.analyze()
        controls = top.getTimingDiagnostics()["clockControls"]
        self.assertEqual(1, len(controls))
        self.assertIsInstance(controls[0]["clockPin"], naja.SNLOccurrence)
        self.assertIsInstance(controls[0]["output"], naja.SNLOccurrence)
        self.assertEqual(1, len(controls[0]["clocks"]))

    def test_clock_and_model_edits_invalidate(self):
        top = self.crossing()
        declaration = list(top.getClocks())[0]
        for edit in (lambda: declaration.setWaveform(1.0, 6.0),
                     lambda: top.getScalarTerm("d").setName("data"),
                     lambda: top.getScalarTerm("b").setRole(naja.SNLTermRole.Clock)):
            top.analyzeTiming()
            edit()
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                top.getClockDomains()

    def test_terminal_and_net_queries(self):
        top = self.crossing()
        self.analyze()
        first, second = top.getClockDomains()
        output = top.getScalarTerm("q")
        pin = naja.SNLOccurrence(output)
        net = naja.SNLOccurrence(output.getNet())
        self.assertEqual({first}, net.getDrivingClocks())
        self.assertEqual({second}, net.getReceivingClocks())
        self.assertEqual(pin.getDrivingClocks(), net.getDrivingClocks())
        self.assertEqual(frozenset(), pin.getReceivingClocks())
        sources = [c for c in output.getNet().getComponents()
                   if isinstance(c, naja.SNLInstTerm) and c.getDirection() == naja.SNLTerm.Direction.Output]
        self.assertEqual(1, len(sources))
        self.assertEqual({second}, naja.SNLOccurrence(sources[0]).getReceivingClocks())

    def test_state_boundaries(self):
        top = self.load("""module sample(input a, b, c, d, output reg q, r, s);
            always @(posedge a) q <= d;
            always @(posedge b) r <= ~q;
            always @(posedge c) s <= r;
        endmodule""")
        for name in ("a", "b", "c"):
            self.primary(top, name, name)
        self.analyze()
        a, b, c = top.getClockDomains()
        q = naja.SNLOccurrence(top.getScalarTerm("q").getNet())
        r = naja.SNLOccurrence(top.getScalarTerm("r").getNet())
        self.assertEqual({a}, q.getDrivingClocks())
        self.assertEqual({b}, q.getReceivingClocks())
        self.assertEqual({b}, r.getDrivingClocks())
        self.assertEqual({c}, r.getReceivingClocks())

    def test_destroy_design_invalidates_native_context(self):
        top = self.crossing()
        self.analyze()
        domain = top.getClockDomains()[0]
        endpoint = top.getCaptureEndpoints()[0]
        top.destroy()
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            domain.getSources()
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            endpoint.getDrivingClocks()

    def test_clone_does_not_inherit_analysis(self):
        top = self.crossing()
        self.analyze()
        clone = top.clone("copy")
        with self.assertRaisesRegex(RuntimeError, "No timing analysis"):
            clone.getClockDomains()

    def test_caller_occurrence_survives_universe_destruction_safely(self):
        top = self.crossing()
        occurrence = naja.SNLOccurrence(top.getScalarTerm("q"))
        self.analyze()
        self.universe.destroy()
        naja.NLUniverse.create()
        with self.assertRaisesRegex(RuntimeError, "destroyed"):
            occurrence.getDrivingClocks()

    def test_deleted_signal_occurrence_after_reanalysis(self):
        top = self.crossing()
        signal = top.getScalarTerm("q")
        occurrence = naja.SNLOccurrence(signal)
        self.analyze()
        signal.destroy()
        top.analyzeTiming()
        with self.assertRaisesRegex(RuntimeError, "destroyed"):
            occurrence.getReceivingClocks()

    def test_one_hundred_thousand_endpoints(self):
        top = self.load("""module sample(input c, input [99999:0] d,
            output reg [99999:0] q);
            always @(posedge c) q <= d;
        endmodule""")
        self.primary(top, "root", "c")
        self.analyze()
        start = time.perf_counter()
        endpoints = top.getCaptureEndpoints()
        elapsed = time.perf_counter() - start
        self.assertEqual(100_000, len(endpoints))
        domain = top.getClockDomains()[0]
        for endpoint in (endpoints[0], endpoints[-1]):
            self.assertEqual({domain}, endpoint.getReceivingClocks())
            self.assertEqual(frozenset(), endpoint.getDrivingClocks())
        print(f"Timing 100000 endpoint occurrences: {elapsed:.3f}s", flush=True)

    def test_largest_existing_design_timing(self):
        formats = Path(os.environ.get("FORMATS_PATH", Path(__file__).resolve().parents[2] / "formats"))
        fixture = formats / "verilog/benchmarks/large_hier_gates.v"
        top = self.db.loadGateLevelVerilog([str(fixture)])
        self.universe.setTopDesign(top)
        start = time.perf_counter()
        self.analyze()
        elapsed = time.perf_counter() - start
        self.assertEqual([], top.getCaptureEndpoints())
        print(f"analyzeTiming large_hier_gates: {elapsed:.3f}s", flush=True)


    def test_occurrence_value_protocols(self):
        top = self.crossing()
        self.analyze()
        source = top.getClockDomains()[0].getSources()[0]
        endpoint = top.getCaptureEndpoints()[0]
        for occurrence in (source, endpoint):
            with self.subTest(occurrence=str(occurrence)):
                self.assertTrue(repr(occurrence))
                self.assertEqual(str(occurrence), repr(occurrence))
                self.assertEqual(hash(occurrence), hash(occurrence))
                self.assertEqual(1, len({occurrence, occurrence}))
        same_endpoint = top.getCaptureEndpoints()[0]
        self.assertEqual(endpoint, same_endpoint)
        self.assertEqual(hash(endpoint), hash(same_endpoint))

    def test_analyze_rejects_non_top_and_primitive_designs(self):
        top = self.crossing()
        other = top.clone("other")
        with self.assertRaisesRegex(RuntimeError, "requires the active top design"):
            other.analyzeTiming()
        library = naja.NLLibrary.createPrimitives(self.db)
        primitive = naja.SNLDesign.createPrimitive(library, "cell")
        self.universe.setTopDesign(primitive)
        with self.assertRaisesRegex(RuntimeError, "expected a structural top design"):
            primitive.analyzeTiming()

    def test_whole_instance_timing_query_is_rejected(self):
        top = self.crossing()
        occurrence = naja.SNLOccurrence(next(iter(top.getInstances())))
        self.analyze()
        with self.assertRaisesRegex(RuntimeError, "single-bit terminal or net occurrence"):
            occurrence.getDrivingClocks()

    def test_caller_hierarchical_occurrence_requires_refresh(self):
        top = self.load("""module child(input a, output y); assign y = a; endmodule
            module sample(input a, output y); child cell0(a, y); endmodule""")
        self.primary(top, "clock", "a")
        instance = top.getInstance("cell0")
        term = instance.getModel().getScalarTerm("a")
        path = naja.SNLPath(instance)
        occurrence = naja.SNLOccurrence(path, term)
        self.analyze()
        self.assertEqual(1, len(occurrence.getWaveformClocks()))
        top.getScalarTerm("a").setName("input_data")
        top.analyzeTiming()
        with self.assertRaisesRegex(RuntimeError, "Stale hierarchical occurrence"):
            occurrence.getWaveformClocks()
        refreshed = naja.SNLOccurrence(path, term)
        self.assertEqual(1, len(refreshed.getWaveformClocks()))

    def test_coverage_inout_and_multiple_drivers(self):
        top = self.crossing()
        naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.InOut, "io")
        first = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Input, "x")
        second = naja.SNLScalarTerm.create(top, naja.SNLTerm.Direction.Input, "y")
        net = naja.SNLScalarNet.create(top, "shared")
        first.setNet(net)
        second.setNet(net)
        self.analyze()
        report = top.getTimingCoverageReport()
        treatments = {(row["reason"], row["treatment"]) for row in report}
        self.assertIn(("inout", "bidirectional"), treatments)
        self.assertIn(("multiple_drivers", "all_drivers"), treatments)
        counts = top.getTimingCoverageCounts()
        self.assertEqual(1, counts["inout"])
        self.assertEqual(1, counts["multiple_drivers"])

    def test_bidirectional_cycle_diagnostic_role(self):
        designs = naja.NLLibrary.create(self.db)
        primitives = naja.NLLibrary.createPrimitives(self.db)
        top = naja.SNLDesign.create(designs, "sample")
        model = naja.SNLDesign.createPrimitive(primitives, "bidirectional")
        a = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.InOut, "a")
        b = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.InOut, "b")
        naja.SNLDesign.addCombinatorialArcs([a], [b])
        naja.SNLDesign.addCombinatorialArcs([b], [a])
        left = naja.SNLInstance.create(top, model, "left")
        right = naja.SNLInstance.create(top, model, "right")
        for name, source, target in (("n0", a, b), ("n1", b, a)):
            net = naja.SNLScalarNet.create(top, name)
            left.getInstTerm(source).setNet(net)
            right.getInstTerm(target).setNet(net)
        self.universe.setTopDesign(top)
        with self.assertRaises(naja.SNLTimingCycleError) as caught:
            top.analyzeTiming()
        self.assertEqual("load", caught.exception.role)
        self.assertIn("(" + caught.exception.role + ")", caught.exception.description)
        self.assertIsInstance(caught.exception.occurrence, naja.SNLOccurrence)

    def test_bidirectional_cycle_drive_diagnostic(self):
        designs = naja.NLLibrary.create(self.db)
        primitives = naja.NLLibrary.createPrimitives(self.db)
        top = naja.SNLDesign.create(designs, "sample")
        model = naja.SNLDesign.createPrimitive(primitives, "bidirectional")
        terms = [naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.InOut, name)
                 for name in ("a", "b", "c", "d")]
        # Two feedback paths share a drive-side node. The diagnostic must retain
        # its drive role instead of treating every split pin as a load.
        for source, target in ((0, 2), (1, 2), (1, 3)):
            naja.SNLDesign.addCombinatorialArcs([terms[source]], [terms[target]])
        left = naja.SNLInstance.create(top, model, "left")
        right = naja.SNLInstance.create(top, model, "right")
        for i in range(4):
            net = naja.SNLScalarNet.create(top, "n" + str(i))
            left.getInstTerm(terms[i]).setNet(net)
            right.getInstTerm(terms[3 - i]).setNet(net)
        self.universe.setTopDesign(top)
        with self.assertRaises(naja.SNLTimingCycleError) as caught:
            top.analyzeTiming()
        self.assertEqual("drive", caught.exception.role)
        self.assertIn("(drive)", caught.exception.description)
        self.assertIsInstance(caught.exception.occurrence, naja.SNLOccurrence)


if __name__ == "__main__":
    unittest.main()
