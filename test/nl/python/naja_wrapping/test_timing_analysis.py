# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import gc
import os
import resource
import sys
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
        self.assertEqual({first}, source.getLaunchClocks())
        endpoints = top.getEndpoints()
        self.assertEqual(2, len(endpoints))
        crossings = [e for e in endpoints if e.getLaunchClocks() == {first}
                     and e.getCaptureClocks() == {second}]
        self.assertEqual(1, len(crossings))
        self.assertIsInstance(crossings[0], naja.SNLOccurrence)
        self.assertEqual([], top.getTimingDiagnostics()["noClock"])

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
        self.assertEqual({domain}, source.getLaunchClocks())
        self.assertEqual({domain}, source.getClocks())
        rows = source.getFaninStartpoints()
        self.assertEqual({domain}, frozenset(c for row in rows for c in row["launchClocks"]))
        self.assertTrue(any(row["kind"] == "clock" and row["launchPin"] == source for row in rows))
        self.assertEqual({domain}, top.getEndpoints()[0].getCaptureClocks())
        self.assertEqual([], top.getTimingDiagnostics()["noClock"])

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
                         {e.getCaptureClocks() for e in top.getEndpoints()})

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
        self.assertEqual({domain}, top.getEndpoints()[0].getCaptureClocks())

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
        self.assertEqual({domain}, generated["output"].getClocks())
        self.assertEqual({domain}, generated["output"].getLaunchClocks())
        self.assertEqual([], diagnostics["noClock"])
        self.assertTrue(all(e.getCaptureClocks() == {domain} for e in top.getEndpoints()))

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
        self.assertEqual([], top.getTimingDiagnostics()["noClock"])
        self.assertEqual(2, len(top.getEndpoints()))

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
        endpoints = top.getEndpoints()
        memory_inputs = [e for e in endpoints
                         if e.getInstTerm().getInstance().getModel().hasMemoryInterface()]
        self.assertGreaterEqual(len(memory_inputs), 3)
        self.assertTrue(all(e.getCaptureClocks() == {clocks["clock"]} for e in memory_inputs))
        self.assertTrue(any(e.getCaptureClocks() == {clocks["gate"]} for e in endpoints))
        self.assertIn(("unsupported_latch", "approximated"),
                      {(row["reason"], row["treatment"])
                       for row in top.getTimingCoverageReport()})

    def test_late_declaration_and_rebuild(self):
        top = self.crossing()
        self.analyze()
        endpoint = top.getEndpoints()[0]
        naja.NLClock.createPrimary(top, "late", 15.0, "ns", [top.getScalarTerm("d")])
        for query in (top.getClockDomains, lambda: endpoint.getLaunchClocks()):
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()
        top.analyzeTiming()
        self.assertEqual(3, len(top.getClockDomains()))
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            endpoint.getCaptureClocks()

    def test_destroy_and_recreate_universe_invalidates(self):
        top = self.crossing()
        self.analyze()
        domain = top.getClockDomains()[0]
        endpoint = top.getEndpoints()[0]
        bound = endpoint.getPath
        self.universe.destroy()
        naja.NLUniverse.create()
        for query in (domain.getNLClock, bound, lambda: endpoint.getLaunchClocks(),
                      lambda: hash(endpoint), lambda: str(endpoint)):
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()
        with self.assertRaises(RuntimeError):
            top.getEndpoints()

    def test_rebuild_invalidates_old_views(self):
        top = self.crossing()
        self.analyze()
        domain = top.getClockDomains()[0]
        endpoint = top.getEndpoints()[0]
        top.analyzeTiming()
        for query in (domain.getNLClock, lambda: endpoint.getCaptureClocks()):
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()
        self.assertEqual(2, len(top.getEndpoints()))

    def test_edit_refreshes_connectivity(self):
        top = self.crossing()
        self.analyze()
        first, second = top.getClockDomains()
        output = naja.SNLOccurrence(top.getScalarTerm("q"))
        self.assertEqual({first}, output.getLaunchClocks())
        self.assertEqual(frozenset(), output.getCaptureClocks())
        self.assertEqual({second}, naja.SNLOccurrence(top.getScalarTerm("q").getNet()).getCaptureClocks())
        top.getScalarTerm("q").setNet(None)
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            output.getLaunchClocks()
        top.analyzeTiming()
        self.assertEqual(frozenset(), output.getLaunchClocks())

    def test_native_context_outlives_python_design_wrapper(self):
        top = self.crossing()
        self.analyze()
        endpoint = top.getEndpoints()[0]
        del top
        gc.collect()
        self.assertEqual(1, len(endpoint.getCaptureClocks()))

    def test_missing_analysis_and_hidden_graph(self):
        top = self.crossing()
        occurrence = naja.SNLOccurrence(top.getScalarTerm("q"))
        with self.assertRaisesRegex(RuntimeError, "analyzeTiming"):
            occurrence.getLaunchClocks()
        self.analyze()
        for name in ("TGraph", "TSNLGraphConstructor", "TClockPropagator", "TNode", "TEdge", "TClock"):
            self.assertFalse(hasattr(naja, name))
        self.assertFalse(hasattr(top, "getGraph"))
        with self.assertRaises(TypeError):
            naja.SNLClockDomain()
        with self.assertRaises(RuntimeError):
            naja.SNLOccurrence().getLaunchClocks()
        with self.assertRaises(RuntimeError):
            naja.SNLOccurrence(top.getInstance(next(iter(top.getInstances())).getName())).getLaunchClocks()

    def test_unclocked_pins(self):
        top = self.load("module sample(input c, d, output reg q); always @(posedge c) q <= d; endmodule")
        self.analyze()
        pins = top.getTimingDiagnostics()["noClock"]
        self.assertEqual(1, len(pins))
        self.assertIsInstance(pins[0], naja.SNLOccurrence)
        self.assertEqual(frozenset(), pins[0].getClocks())
        self.assertEqual(frozenset(), top.getEndpoints()[0].getCaptureClocks())

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
        self.assertEqual({first}, net.getLaunchClocks())
        self.assertEqual({second}, net.getCaptureClocks())
        self.assertEqual(pin.getLaunchClocks(), net.getLaunchClocks())
        self.assertEqual(frozenset(), pin.getCaptureClocks())
        sources = [c for c in output.getNet().getComponents()
                   if isinstance(c, naja.SNLInstTerm) and c.getDirection() == naja.SNLTerm.Direction.Output]
        self.assertEqual(1, len(sources))
        self.assertEqual({second}, naja.SNLOccurrence(sources[0]).getCaptureClocks())

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
        self.assertEqual({a}, q.getLaunchClocks())
        self.assertEqual({b}, q.getCaptureClocks())
        self.assertEqual({b}, r.getLaunchClocks())
        self.assertEqual({c}, r.getCaptureClocks())

    def test_destroy_design_invalidates_native_context(self):
        top = self.crossing()
        self.analyze()
        domain = top.getClockDomains()[0]
        endpoint = top.getEndpoints()[0]
        top.destroy()
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            domain.getSources()
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            endpoint.getLaunchClocks()

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
            occurrence.getLaunchClocks()

    def test_deleted_signal_occurrence_after_reanalysis(self):
        top = self.crossing()
        signal = top.getScalarTerm("q")
        occurrence = naja.SNLOccurrence(signal)
        self.analyze()
        signal.destroy()
        top.analyzeTiming()
        with self.assertRaisesRegex(RuntimeError, "destroyed"):
            occurrence.getCaptureClocks()

    def test_one_hundred_thousand_endpoints(self):
        top = self.load("""module sample(input c, input [99999:0] d,
            output reg [99999:0] q);
            always @(posedge c) q <= d;
        endmodule""")
        self.primary(top, "root", "c")
        self.analyze()
        start = time.perf_counter()
        endpoints = top.getEndpoints()
        elapsed = time.perf_counter() - start
        self.assertEqual(100_000, len(endpoints))
        domain = top.getClockDomains()[0]
        for endpoint in (endpoints[0], endpoints[-1]):
            self.assertEqual({domain}, endpoint.getCaptureClocks())
            self.assertEqual(frozenset(), endpoint.getLaunchClocks())
        print(f"Timing 100000 endpoint occurrences: {elapsed:.3f}s", flush=True)
        first_rows = endpoints[0].getFaninStartpoints()
        start = time.perf_counter()
        queried = 0
        for endpoint in endpoints:
            if endpoint.getLaunchClocks() != endpoint.getCaptureClocks():
                rows = endpoint.getFaninStartpoints()
                self.assertEqual(1, len(rows))
                queried += 1
        elapsed = time.perf_counter() - start
        rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
        mib = rss / (1024**2 if sys.platform == "darwin" else 1024)
        self.assertEqual(100_000, queried)
        # Earlier materialized results have been evicted; symbolic summaries
        # must still produce the same result when those pins are queried again.
        self.assertEqual(first_rows, endpoints[0].getFaninStartpoints())
        print(f"Fanin 100000 differing endpoints: {elapsed:.3f}s; process peak RSS {mib:.1f}MiB", flush=True)

    def test_largest_existing_design_timing(self):
        formats = Path(os.environ.get("FORMATS_PATH", Path(__file__).resolve().parents[2] / "formats"))
        fixture = formats / "verilog/benchmarks/large_hier_gates.v"
        top = self.db.loadGateLevelVerilog([str(fixture)])
        self.universe.setTopDesign(top)
        start = time.perf_counter()
        self.analyze()
        elapsed = time.perf_counter() - start
        self.assertEqual([], top.getEndpoints())
        print(f"analyzeTiming large_hier_gates: {elapsed:.3f}s", flush=True)


    def signal(self, top, name):
        return naja.SNLOccurrence(top.getNet(name))

    def state_output(self, top, name):
        rows = self.signal(top, name).getFaninStartpoints()
        return next(r["launchPin"] for r in rows if r["kind"] == "register")

    def state_pin(self, output, name):
        pin = output.getInstTerm()
        term = pin.getInstance().getModel().getScalarTerm(name)
        return naja.SNLOccurrence(output.getPath(), pin.getInstance().getInstTerm(term))

    def assert_cones(self, top, endpoints=None):
        complete = endpoints is None
        endpoints = list(top.getEndpoints() if complete else endpoints)
        sources, known_sources, pins = [], set(), []

        def add_source(source):
            if source not in known_sources:
                known_sources.add(source)
                sources.append(source)

        # Discover sources from the netlist, independently of fanin results.
        # Otherwise a missing launch point could disappear from both test walks.
        opaque = {row["occurrence"] for row in top.getTimingCoverageReport()
                  if row["treatment"] == "opaque"}
        for term in top.getBitTerms():
            pin = naja.SNLOccurrence(term)
            pins.append(pin)
            if term.getDirection() != naja.SNLTerm.Direction.Output:
                add_source(pin)
            elif complete:
                endpoints.append(pin)
        for clock in list(top.getClockDomains()) + list(top.getResetDomains()):
            for source in clock.getSources():
                add_source(source)
        pending = [(naja.SNLPath(), top)]
        while pending:
            path, design = pending.pop()
            for net in design.getBitNets():
                if net.isConstant():
                    add_source(naja.SNLOccurrence(path, net))
            for instance in design.getInstances():
                model = instance.getModel()
                if not model.isLeaf():
                    pending.append((naja.SNLPath(path, instance), model))
                    continue
                boundary = (model.hasSequentialModel() or model.hasMemoryInterface()
                            or naja.SNLOccurrence(path, instance) in opaque)
                for term in instance.getInstTerms():
                    pin = naja.SNLOccurrence(path, term)
                    pins.append(pin)
                    if boundary and term.getDirection() != naja.SNLTerm.Direction.Input:
                        add_source(pin)
                    if complete and term.getBitTerm().getRole() == naja.SNLTermRole.Clock:
                        endpoints.append(pin)

        fanin_cache, fanout_cache = {}, {}

        def fanin(pin):
            if pin not in fanin_cache:
                rows = pin.getFaninStartpoints()
                self.assertEqual(pin.getLaunchClocks(),
                                 frozenset(c for row in rows for c in row["launchClocks"]), str(pin))
                self.assertEqual(rows, pin.getFaninStartpoints())
                self.assertEqual(len(rows), len({(r["launchPin"], r["startpoint"]) for r in rows}))
                sparse = pin.getFaninStartpoints(withCells=False)
                self.assertEqual([{k: v for k, v in row.items() if k != "cells"} for row in rows], sparse)
                for row in rows:
                    if not pin.getLaunchClocks():
                        self.assertIn(row["kind"], ("input", "blackbox", "constant", "register", "reset"))
                        self.assertFalse(row["launchClocks"])
                    add_source(row["launchPin"])
                fanin_cache[pin] = rows
            return fanin_cache[pin]

        def fanout(source):
            if source not in fanout_cache:
                rows = source.getFanoutEndpoints()
                self.assertEqual(rows, source.getFanoutEndpoints())
                sparse = source.getFanoutEndpoints(withCells=False)
                self.assertEqual([{k: v for k, v in row.items() if k != "cells"} for row in rows], sparse)
                self.assertEqual(len(rows), len({r["endpoint"] for r in rows}))
                fanout_cache[source] = {r["endpoint"]: r for r in rows}
            return fanout_cache[source]

        def equal_path(backward, forward):
            self.assertEqual(backward["cells"], forward["cells"])
            self.assertEqual(backward["direct"], forward["direct"])

        # Check launch-mask equality even at intermediate combinational pins.
        if complete:
            for pin in pins:
                fanin(pin)
        # Fanin -> fanout, including explicitly selected clock/control stops.
        for endpoint in endpoints:
            for row in fanin(endpoint):
                forward = fanout(row["launchPin"])
                self.assertIn(endpoint, forward, (str(row["launchPin"]), str(endpoint)))
                equal_path(row, forward[endpoint])
        # Fanout -> fanin independently checks *every* reported stop. New launch
        # points discovered here join the queue, rather than being silently lost.
        index = 0
        while index < len(sources):
            source = sources[index]
            index += 1
            for endpoint, forward in fanout(source).items():
                backward = [r for r in fanin(endpoint) if r["launchPin"] == source]
                self.assertTrue(backward, (str(source), str(endpoint)))
                for row in backward:
                    equal_path(row, forward)

    def reset(self, top, name="domain", term="rst_n", level=None):
        return naja.NLReset.create(top, name, [top.getScalarTerm(term)],
                                   naja.SNLActiveLevel.Low if level is None else level,
                                   assertion="async", deassertion="sync")

    def depth(self, source, clock):
        def walk(output, visited):
            if output in visited:
                return 0
            stages = []
            for row in output.getFanoutEndpoints():
                if row["kind"] == "register" and row["direct"] and row["captureClocks"] == {clock}:
                    stages.extend(1 + walk(q, visited | {output}) for q in row["registerOutputs"])
            return max(stages, default=0)
        return walk(source, set())

    def test_declared_synchronous_reset_depth(self):
        for count in (2, 3):
            with self.subTest(count=count):
                top = self.load(f"""module sample(input clka, clkb, rst_n, data,
                    output reg src, output reg [{count-1}:0] stages);
                    always @(posedge clka) src <= data;
                    wire [{count-1}:0] next_stages = rst_n ? {{stages[{count-2}:0], src}} : '0;
                    always @(posedge clkb) stages <= next_stages;
                endmodule""")
                self.primary(top, "a", "clka")
                self.primary(top, "b", "clkb")
                top.analyzeTiming()
                src = self.state_output(top, "src")
                self.assertTrue(any(not r["direct"] for r in src.getFanoutEndpoints() if r["kind"] == "register"))
                first = next(r for r in src.getFanoutEndpoints() if r["kind"] == "register")["registerOutputs"][0]
                self.assertEqual(1, 1 + self.depth(first, top.getClockDomains()[1]))
                self.reset(top)
                top.analyzeTiming()
                src = self.state_output(top, "src")
                domain = top.getResetDomains()[0]
                self.assertEqual(count, self.depth(src, top.getClockDomains()[1]))
                rows = [r for r in src.getFanoutEndpoints() if r["kind"] == "register"]
                self.assertTrue(all(r["direct"] and r["syncResets"] == {domain} for r in rows))
                self.assert_cones(top)
                self.universe.destroy()
                self.setUp()

    def test_declared_vector_reset_is_bitwise(self):
        top = self.load("""module sample(input a, b, rst_n, input [3:0] data,
            output reg [3:0] src, meta, sync);
            always @(posedge a) src <= data;
            wire [3:0] next_meta = rst_n ? src : 4'b0;
            wire [3:0] next_sync = rst_n ? meta : 4'b0;
            always @(posedge b) begin meta <= next_meta; sync <= next_sync; end
        endmodule""")
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.reset(top)
        top.analyzeTiming()
        for bit in range(4):
            source = naja.SNLOccurrence(top.getNet("src").getBit(bit))
            launch = next(r["launchPin"] for r in source.getFaninStartpoints() if r["kind"] == "register")
            rows = [r for r in launch.getFanoutEndpoints() if r["kind"] == "register"]
            self.assertEqual(1, len(rows))
            self.assertTrue(rows[0]["direct"])
            self.assertEqual(1, len(rows[0]["registerOutputs"]))
            target = rows[0]["registerOutputs"][0]
            expected = naja.SNLOccurrence(top.getNet("meta").getBit(bit)).getFaninStartpoints()
            self.assertEqual(target, next(r["launchPin"] for r in expected if r["kind"] == "register"))
            self.assertEqual(2, self.depth(launch, top.getClockDomains()[1]))
        self.assert_cones(top)

    def test_declared_reset_domain_crossing(self):
        top = self.load("""module sample(input c, rst_a_n, rst_b_n, d,
            output reg q, r);
            always @(posedge c or negedge rst_a_n) if (!rst_a_n) q <= 0; else q <= d;
            always @(posedge c or negedge rst_b_n) if (!rst_b_n) r <= 0; else r <= q;
        endmodule""")
        self.primary(top, "c", "c")
        self.reset(top, "a", "rst_a_n")
        self.reset(top, "b", "rst_b_n")
        top.analyzeTiming()
        a, b = top.getResetDomains()
        src = self.state_output(top, "q")
        target = self.state_pin(self.state_output(top, "r"), "D")
        row = next(r for r in target.getFaninStartpoints() if r["kind"] == "register")
        self.assertEqual({a}, row["asyncResets"])
        row = next(r for r in src.getFanoutEndpoints() if r["endpoint"] == target)
        self.assertEqual({b}, row["asyncResets"])
        self.assertEqual(frozenset(), row["syncResets"])
        self.assert_cones(top)

    def test_declared_reset_synchronizer(self):
        top = self.load("""module sample(input c, rst_n, input [99:0] data,
            output reg first, second, output reg [99:0] q);
            always @(posedge c or negedge rst_n)
              if (!rst_n) begin first <= 0; second <= 0; end
              else begin first <= 1; second <= first; end
            for (genvar i=0; i<100; i=i+1)
              always @(posedge c or negedge second)
                if (!second) q[i] <= 0; else q[i] <= data[i];
        endmodule""")
        self.primary(top, "c", "c")
        self.reset(top)
        top.analyzeTiming()
        domain = top.getResetDomains()[0]
        stages = top.getTimingDiagnostics()["resetSynchronizers"]
        self.assertEqual([1, 2], sorted(r["stage"] for r in stages))
        self.assertTrue(all(r["resets"] == {domain} for r in stages))
        for pin in top.getEndpoints(check="recovery"):
            self.assertEqual({domain}, pin.getResets())
        source = self.state_output(top, "second")
        rows = [r for r in source.getFanoutEndpoints() if r["kind"] == "recovery"]
        self.assertEqual(100, len(rows))
        self.assert_cones(top)

    def test_declared_reset_gating(self):
        top = self.load("""module sample(input c, rst_n, other, d, output reg q);
            wire rn = rst_n & other;
            always @(posedge c or negedge rn) if (!rn) q <= 0; else q <= d;
        endmodule""")
        self.primary(top, "c", "c")
        self.reset(top)
        top.analyzeTiming()
        domain = top.getResetDomains()[0]
        pin = top.getEndpoints(check="recovery")[0]
        self.assertEqual({domain}, pin.getResets())
        rows = pin.getFaninStartpoints()
        self.assertEqual([naja.SNLOccurrence(top.getScalarTerm("other"))], [r["launchPin"] for r in rows])
        self.assertTrue(rows[0]["direct"])
        controls = top.getTimingDiagnostics()["resetControls"]
        self.assertEqual(1, len(controls))
        self.assertEqual({domain}, controls[0]["resets"])
        self.assertEqual(1, len(controls[0]["nonResetInputs"]))
        self.assert_cones(top)
        # With active-high assertion, inactive 0 makes this AND constant 0.
        # The recovery endpoint must remain visible in the physical reset view.
        list(top.getResets())[0].destroy()
        self.reset(top, level=naja.SNLActiveLevel.High)
        top.analyzeTiming()
        pin = top.getEndpoints(check="recovery")[0]
        self.assertEqual({top.getResetDomains()[0]}, pin.getResets())
        self.assertEqual([], pin.getFaninStartpoints())
        self.assert_cones(top)

    def test_declared_foreign_recovery_crossing(self):
        top = self.load("""module sample(input a, b, rst_n, d, output reg foreign_reset, q);
            always @(posedge a or negedge rst_n) if (!rst_n) foreign_reset <= 0; else foreign_reset <= d;
            always @(posedge b or negedge foreign_reset) if (!foreign_reset) q <= 0; else q <= d;
        endmodule""")
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.reset(top)
        top.analyzeTiming()
        output = self.state_output(top, "foreign_reset")
        rows = [r for r in output.getFanoutEndpoints() if r["kind"] == "recovery"]
        self.assertEqual(1, len(rows))
        self.assertEqual({top.getClockDomains()[0]}, rows[0]["endpoint"].getLaunchClocks())
        self.assertEqual({top.getResetDomains()[0]}, output.getFaninStartpoints()[0]["asyncResets"])
        # A data-driven reset is not inferred as a reset synchronizer.
        self.assertFalse(rows[0]["endpoint"].getResets())
        self.assert_cones(top)

    def test_declared_blackbox_reset_domains(self):
        top = self.load("""module sample(input a, b, c, d, output reg q, r);
            wire left_reset, right_reset;
            controller left(.ref_i(a), .rst_o(left_reset));
            controller right(.ref_i(b), .rst_o(right_reset));
            always @(posedge c or negedge left_reset) if (!left_reset) q <= 0; else q <= d;
            always @(posedge c or negedge right_reset) if (!right_reset) r <= 0; else r <= d;
        endmodule""")
        self.primary(top, "c", "c")
        model = top.getInstance("left").getModel()
        self.reset(model, "domain", "rst_o")
        top.analyzeTiming()
        domains = top.getResetDomains()
        self.assertEqual(2, len(domains))
        self.assertEqual(domains[0].getNLReset(), domains[1].getNLReset())
        self.assertNotEqual(domains[0].getSources(), domains[1].getSources())
        for domain in domains:
            source = domain.getSources()[0]
            rows = [r for r in source.getFaninStartpoints() if r["kind"] == "reset"]
            self.assertEqual(1, len(rows))
            self.assertEqual({domain}, rows[0]["resets"])
        self.assertEqual({frozenset({d}) for d in domains},
                         {p.getResets() for p in top.getEndpoints(check="recovery")})
        self.assert_cones(top)

    def test_declared_reset_validation_and_staleness(self):
        top = self.crossing()
        other = top.clone("other")
        with self.assertRaisesRegex(RuntimeError, "another design"):
            naja.NLReset.create(top, "bad", [other.getScalarTerm("d")], naja.SNLActiveLevel.Low)
        with self.assertRaisesRegex(RuntimeError, "duplicate source"):
            naja.NLReset.create(top, "bad", [top.getScalarTerm("d")]*2, naja.SNLActiveLevel.Low)
        for sources, level, kwargs in (([], naja.SNLActiveLevel.Low, {}),
                ([top.getScalarTerm("d")], naja.SNLActiveLevel.NA, {}),
                ([top.getScalarTerm("d")], naja.SNLActiveLevel.Low, {"assertion": "invalid"}),
                ([top.getScalarTerm("d")], naja.SNLActiveLevel.Low, {"deassertion": "invalid"}),
                ([top], naja.SNLActiveLevel.Low, {}),
                ([None], naja.SNLActiveLevel.Low, {}),
                (tuple([top.getScalarTerm("d")]), naja.SNLActiveLevel.Low, {}),
                ([next(iter(next(iter(top.getInstances())).getInstTerms()))], naja.SNLActiveLevel.Low, {})):
            with self.assertRaises(RuntimeError):
                naja.NLReset.create(top, "bad", sources, level, **kwargs)
        with self.assertRaises(TypeError):
            naja.NLReset.create()
        top.analyzeTiming()
        old = top.getEndpoints()[0]
        declaration = self.reset(top, "reset", "d")
        self.assertEqual([declaration], list(top.getResets()))
        self.assertEqual([declaration], list(self.db.getResets()))
        self.assertEqual("reset", declaration.getName())
        self.assertEqual(top, declaration.getDesign())
        self.assertEqual([top.getScalarTerm("d")], declaration.getSources())
        self.assertEqual(naja.SNLActiveLevel.Low, declaration.getActiveLevel())
        self.assertEqual(1, declaration.getInactiveValue())
        self.assertEqual("async", declaration.getAssertion())
        self.assertEqual("sync", declaration.getDeassertion())
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            old.getResets()
        top.analyzeTiming()
        domain = top.getResetDomains()[0]
        self.assertEqual(domain, top.getResetDomains()[0])
        self.assertEqual(1, len({domain, domain}))
        self.assert_cones(top)
        declaration.destroy()
        with self.assertRaisesRegex(RuntimeError, "Stale"):
            domain.getSources()
        top.analyzeTiming()
        self.assertEqual([], top.getResetDomains())
        self.assertEqual([], list(top.getResets()))

    def test_declared_reset_timing_metadata(self):
        top = self.crossing()
        for assertion in ("async", "sync"):
            for deassertion in ("async", "sync"):
                with self.subTest(assertion=assertion, deassertion=deassertion):
                    reset = naja.NLReset.create(
                        top, "reset", [top.getScalarTerm("d")], naja.SNLActiveLevel.Low,
                        assertion=assertion, deassertion=deassertion)
                    self.assertEqual(assertion, reset.getAssertion())
                    self.assertEqual(deassertion, reset.getDeassertion())
                    reset.destroy()
        for argument in ("assertion", "deassertion"):
            with self.assertRaises(TypeError):
                naja.NLReset.create(top, "reset", [top.getScalarTerm("d")],
                                    naja.SNLActiveLevel.Low, **{argument: None})

    def test_declared_native_sync_reset(self):
        top = self.load("""module sample(input a, b, rst_n, d, output reg src, meta, sync);
            always @(posedge a) src <= d;
            always @(posedge b) if (!rst_n) begin meta <= 0; sync <= 0; end
              else begin meta <= src; sync <= meta; end
        endmodule""")
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.reset(top)
        top.analyzeTiming()
        domain = top.getResetDomains()[0]
        source = self.state_output(top, "src")
        self.assertEqual(2, self.depth(source, top.getClockDomains()[1]))
        rows = [r for r in source.getFanoutEndpoints() if r["kind"] == "register"]
        self.assertTrue(all(r["syncResets"] == {domain} for r in rows))
        self.assert_cones(top)

    def test_declared_reset_order_and_wire_startpoints(self):
        top = self.load("""module branch(input c, rn, d, output reg q);
            always @(posedge c or negedge rn) if (!rn) q <= 0; else q <= d;
        endmodule
        module sample(input c, rst_a_n, rst_b_n, d, output q, r);
            branch left(c, rst_a_n, d, q);
            branch right(c, rst_b_n, d, r);
        endmodule""")
        self.primary(top, "c", "c")
        def snapshot(order):
            declarations = [self.reset(top, "domain_"+n, "rst_"+n+"_n") for n in order]
            top.analyzeTiming()
            rows = []
            for endpoint in top.getEndpoints(check="recovery"):
                fanin = endpoint.getFaninStartpoints()
                self.assertEqual(1, len(fanin))
                self.assertEqual("reset", fanin[0]["kind"])
                self.assertEqual(fanin[0]["launchPin"], fanin[0]["startpoint"])
                self.assertFalse(fanin[0]["launchClocks"])
                self.assertEqual(endpoint.getResets(), fanin[0]["resets"])
                rows.append((tuple(endpoint.getPath().getInstanceIDs()),
                    tuple(sorted(d.getNLReset().getName() for d in endpoint.getResets()))))
            self.assertEqual(["domain_a", "domain_b"], [d.getNLReset().getName() for d in top.getResetDomains()])
            self.assert_cones(top)
            for d in declarations:
                d.destroy()
            return rows
        self.assertEqual(snapshot(("b", "a")), snapshot(("a", "b")))

    def test_declared_reset_independent_inputs_have_no_crossing_pair(self):
        top = self.load("""module sample(input c, rst_a_n, rst_b_n, d,
            output reg q, r);
            always @(posedge c or negedge rst_a_n) if (!rst_a_n) q <= 0; else q <= d;
            always @(posedge c or negedge rst_b_n) if (!rst_b_n) r <= 0; else r <= d;
        endmodule""")
        self.primary(top, "c", "c")
        self.reset(top, "a", "rst_a_n")
        self.reset(top, "b", "rst_b_n")
        top.analyzeTiming()
        self.assertTrue(all(not any(r["kind"] == "register" for r in e.getFaninStartpoints())
                            for e in top.getEndpoints(check="setup")))
        self.assert_cones(top)

    def test_declared_compact_truth_tables(self):
        # Constant reset bits enter arithmetic, shifts, indexed reads and
        # updates. Every output bit must retain only its matching input bit.
        expressions = ["d + {3{rst}}", "d - {3{rst}}", "d * {2'b0, !rst}",
                       "d << {rst, rst}", "d >> {rst, rst}",
                       "d / {2'b0, !rst}", "d % 4'b1000", "({3'b0, d} >> {rst,rst})",
                       "$signed(d) >>> {rst,rst}", "d[rst +: 3]", "d[$signed(rst) +: 3]", "words[rst]", "adjust(d,rst)"]
        for expression in expressions:
            with self.subTest(expression=expression):
                top = self.load(f"""module sample(input c, rst, input [2:0] d, output reg [2:0] q);
                    wire [2:0] words [0:1];
                    assign words[0] = d;
                    assign words[1] = 3'b101;
                    function automatic [2:0] adjust(input [2:0] x, input flag);
                      adjust = x;
                      adjust[flag +: 1] = x[0];
                    endfunction
                    wire [2:0] next_data = {expression};
                    always @(posedge c) q <= next_data;
                endmodule""")
                self.primary(top, "c", "c")
                self.reset(top, term="rst", level=naja.SNLActiveLevel.High)
                top.analyzeTiming()
                for endpoint in top.getEndpoints(check="setup"):
                    rows = endpoint.getFaninStartpoints()
                    self.assertEqual(1, len(rows))
                    self.assertTrue(rows[0]["direct"])
                    self.assertEqual("d", rows[0]["launchPin"].getNetComponent().getName())
                self.assert_cones(top)
                self.universe.destroy()
                self.setUp()

    def test_declared_reset_removal_restores_original_queries(self):
        top = self.load("""module sample(input a, b, rst_n, d, output reg src, q);
            wire next_data = rst_n ? src : 1'b0;
            always @(posedge a) src <= d;
            always @(posedge b) q <= next_data;
        endmodule""")
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        def identity(occurrence):
            obj = occurrence.getNetComponent() or occurrence.getInstance()
            if obj is None:
                obj = next(n for n in top.getBitNets() if naja.SNLOccurrence(n) == occurrence)
            return (tuple(occurrence.getPath().getInstanceIDs()), tuple(obj.getNLID().toTuple()))
        def value(item):
            if isinstance(item, naja.SNLOccurrence):
                return identity(item)
            if isinstance(item, naja.SNLClockDomain):
                return (item.getNLClock().getName(), tuple(identity(o) for o in item.getSources()))
            if isinstance(item, dict):
                return tuple((k, value(v)) for k, v in sorted(item.items()))
            if isinstance(item, frozenset):
                return tuple(sorted(value(i) for i in item))
            if isinstance(item, (tuple, list)):
                return tuple(value(i) for i in item)
            return item
        def snapshot():
            return [(identity(e), value(e.getLaunchClocks()), value(e.getCaptureClocks()), value(e.getClocks()),
                     value(e.getFaninStartpoints()), value(e.getFanoutEndpoints())) for e in top.getEndpoints()]
        top.analyzeTiming()
        before = snapshot()
        self.assertEqual({"noClock", "generatedClocks", "clockControls"}, set(top.getTimingDiagnostics()))
        declaration = self.reset(top)
        top.analyzeTiming()
        self.assert_cones(top)
        declaration.destroy()
        top.analyzeTiming()
        self.assertEqual(before, snapshot())
        self.assert_cones(top)

    def test_declared_reset_clock_overlap_rejected(self):
        top = self.crossing()
        with self.assertRaisesRegex(RuntimeError, "clock source"):
            self.reset(top, term="a")
        declaration = self.reset(top, term="d")
        with self.assertRaisesRegex(RuntimeError, "reset source"):
            self.primary(top, "bad", "d")
        declaration.destroy()
        self.analyze()
        self.assert_cones(top)

    def test_reference_structural_queries(self):
        # Export every state bit to keep the example independent of dead-state
        # elimination. The logic and crossings are those of the reference RTL.
        top = self.load('''module sample(input clka, clkb, in,
            output reg s, x, f1, f2, y, r, output out);
            always @(posedge clka) s <= in;
            always @(posedge clka) x <= s;
            always @(posedge clkb) f1 <= s;
            always @(posedge clkb) f2 <= f1;
            always @(posedge clkb) y <= f2 & x;
            always @(posedge clkb or negedge s)
                if (!s) r <= 1'b0; else r <= 1'b1;
            assign out = y;
        endmodule''')
        self.primary(top, "a", "clka")
        self.primary(top, "b", "clkb")
        self.analyze()
        a, b = top.getClockDomains()
        outputs = {n: self.state_output(top, n) for n in ("s", "x", "f1", "f2", "y", "r")}
        data = {n: self.state_pin(q, "D") for n, q in outputs.items()}
        clocks = {n: self.state_pin(q, "C") for n, q in outputs.items()}
        recovery = top.getEndpoints(check="recovery")
        self.assertEqual(1, len(recovery))
        self.assertEqual(6, len(top.getEndpoints(check="setup")))
        self.assertEqual(7, len(top.getEndpoints()))
        self.assertEqual({b}, clocks["f1"].getClocks())
        clock_fanin = clocks["f1"].getFaninStartpoints()
        self.assertEqual(1, len(clock_fanin))
        self.assertEqual("clock", clock_fanin[0]["kind"])
        self.assertEqual(naja.SNLOccurrence(top.getScalarTerm("clkb")), clock_fanin[0]["launchPin"])
        self.assertEqual({b}, clock_fanin[0]["launchClocks"])
        self.assert_cones(top, list(clocks.values()))
        self.assertEqual({a}, data["f1"].getLaunchClocks())
        self.assertEqual({b}, data["f1"].getCaptureClocks())
        self.assertEqual({a, b}, data["y"].getLaunchClocks())
        first = data["f1"].getFaninStartpoints()
        self.assertEqual(1, len(first))
        self.assertEqual(outputs["s"], first[0]["launchPin"])
        self.assertEqual(clocks["s"], first[0]["startpoint"])
        self.assertEqual("register", first[0]["kind"])
        self.assertEqual(frozenset(), first[0]["cells"])
        self.assertTrue(first[0]["direct"])
        y = data["y"].getFaninStartpoints()
        self.assertEqual({outputs["f2"], outputs["x"]}, {row["launchPin"] for row in y})
        self.assertTrue(all(len(row["cells"]) == 1 and not row["direct"] for row in y))
        self.assertEqual(outputs["s"], recovery[0].getFaninStartpoints()[0]["launchPin"])
        self.assertEqual("input", data["s"].getFaninStartpoints()[0]["kind"])
        self.assertEqual("constant", data["r"].getFaninStartpoints()[0]["kind"])
        fanout = outputs["s"].getFanoutEndpoints()
        by_pin = {row["endpoint"]: row for row in fanout}
        self.assertEqual("recovery", by_pin[recovery[0]]["kind"])
        self.assertEqual({b}, by_pin[recovery[0]]["captureClocks"])
        self.assertEqual((outputs["r"],), by_pin[recovery[0]]["registerOutputs"])
        self.assertEqual(fanout, clocks["s"].getFanoutEndpoints())
        next_stage = next(row for row in outputs["f1"].getFanoutEndpoints() if row["kind"] == "register")
        self.assertEqual(data["f2"], next_stage["endpoint"])
        self.assertEqual((outputs["f2"],), next_stage["registerOutputs"])
        self.assert_cones(top)
        for value in ("hold", 3, False):
            with self.assertRaises(RuntimeError):
                top.getEndpoints(check=value)
        # Bound methods as well as analysis-returned occurrences must reject edits.
        queries = [data["y"].getFaninStartpoints, outputs["s"].getFanoutEndpoints]
        top.getScalarTerm("in").setName("data")
        for query in queries:
            with self.assertRaisesRegex(RuntimeError, "Stale"):
                query()

    def test_chain_divergence_early_fanout_and_reconvergence(self):
        top = self.load('''module sample(input a, b, d,
            output reg s, p, q, r, u, v, output mixed, joined);
            always @(posedge a) s <= d;
            always @(posedge b) p <= s;
            always @(posedge b) q <= p;
            always @(posedge b) r <= q;
            always @(posedge b) u <= s;
            always @(posedge b) v <= u;
            assign mixed = p ^ s;
            assign joined = r & v;
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.analyze()
        p = self.state_output(top, "p")
        stages = [p]
        while True:
            registers = [r for r in stages[-1].getFanoutEndpoints() if r["kind"] == "register"]
            if not registers:
                break
            stages.append(registers[0]["registerOutputs"][0])
        self.assertEqual(3, len(stages))
        self.assertTrue(any(r["kind"] == "output" and not r["direct"] for r in p.getFanoutEndpoints()))
        s = self.state_output(top, "s")
        self.assertEqual(2, sum(r["kind"] == "register" for r in s.getFanoutEndpoints()))
        self.assert_cones(top)
        self.assert_cones(top, [naja.SNLOccurrence(top.getScalarTerm(n)) for n in ("mixed", "joined")])

    def test_structural_hierarchy_repeated_models(self):
        top = self.load('''module leaf(input c, d, output reg q);
            always @(posedge c) q <= d;
        endmodule
        module sample(input a, b, d, output q, r);
            wire s;
            leaf source(a, d, s);
            leaf left(b, s, q);
            leaf right(b, s, r);
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.analyze()
        self.assert_cones(top)
        rows = [e.getFaninStartpoints()[0] for e in top.getEndpoints()]
        launches = [r["launchPin"] for r in rows if r["kind"] == "register"]
        self.assertEqual(2, len(launches))
        self.assertEqual(launches[0], launches[1])
        self.assertEqual(2, sum(r["kind"] == "register" for r in launches[0].getFanoutEndpoints()))
        q, r = [self.state_output(top, n) for n in ("q", "r")]
        self.assertNotEqual(q, r)
        self.assertNotEqual(q.getPath(), r.getPath())

    def test_structural_multibit_register(self):
        top = self.load('''module sample(input a, b, input [4:0] d,
            output reg [4:0] s, q);
            always @(posedge a) s <= d;
            always @(posedge b) q <= s;
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.analyze()
        crossings = [e for e in top.getEndpoints() if e.getLaunchClocks() != e.getCaptureClocks() and e.getLaunchClocks()]
        self.assertEqual(5, len(crossings))
        for endpoint in crossings:
            source = endpoint.getFaninStartpoints()[0]["launchPin"]
            record = next(r for r in source.getFanoutEndpoints() if r["endpoint"] == endpoint)
            self.assertEqual(1, len(record["registerOutputs"]))
            self.assertEqual(endpoint.getInstTerm().getBitTerm().getBit(),
                             record["registerOutputs"][0].getInstTerm().getBitTerm().getBit())
        self.assert_cones(top)

    def test_structural_memory_and_latch(self):
        top = self.load('''module sample(input a, b, g, e, d, input addr,
            output reg ra, output reg q, output reg l, output read);
            reg storage [0:1];
            always @(posedge b) ra <= addr;
            always @(posedge a) if (e) storage[addr] <= d;
            assign read = storage[ra];
            always @(posedge b) q <= read;
            always_latch if (g) l <= d;
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.primary(top, "g", "g")
        self.analyze()
        target = self.state_pin(self.state_output(top, "q"), "D")
        rows = target.getFaninStartpoints()
        memory = [r for r in rows if r["kind"] == "register" and r["launchPin"].getInstTerm().getInstance().getModel().hasMemoryInterface()]
        self.assertEqual(1, len(memory))
        self.assertEqual({"a"}, {c.getNLClock().getName() for c in memory[0]["launchClocks"]})
        self.assertTrue(memory[0]["direct"])
        address = next(r for r in rows if r["launchPin"] == self.state_output(top, "ra"))
        self.assertFalse(address["direct"])
        self.assertEqual(1, len(address["cells"]))
        self.assertEqual("register", self.signal(top, "l").getFaninStartpoints()[0]["kind"])
        self.assert_cones(top)

    def test_structural_blackbox_and_split_inout(self):
        top = self.load('''module sample(input a, output q);
            wire shared;
            opaque_left left(.a(a), .io(shared));
            opaque_right right(.io(shared), .q(q));
        endmodule''')
        self.analyze()
        q = naja.SNLOccurrence(top.getScalarTerm("q"))
        self.assertEqual("blackbox", q.getFaninStartpoints()[0]["kind"])
        left = top.getInstance("left")
        io = naja.SNLOccurrence(left.getInstTerm(left.getModel().getScalarTerm("io")))
        rows = io.getFanoutEndpoints()
        self.assertTrue(any(r["kind"] == "blackbox" and r["endpoint"] != io for r in rows))
        self.assert_cones(top, [q])

    def test_structural_reconvergent_xor_tree(self):
        # Two nodes per level, exponentially many paths, linear cone size.
        lines = ["module sample(input a, b, output z);"]
        x, y = "a", "b"
        for i in range(24):
            lines += [f"wire x{i} = {x} ^ {y};", f"wire y{i} = {x} ^ ~{y};"]
            x, y = f"x{i}", f"y{i}"
        lines += [f"assign z = {x} ^ {y};", "endmodule"]
        top = self.load("\n".join(lines))
        self.analyze()
        z = naja.SNLOccurrence(top.getScalarTerm("z"))
        start = time.perf_counter()
        rows = z.getFaninStartpoints()
        fanin = time.perf_counter() - start
        start = time.perf_counter()
        fanout = naja.SNLOccurrence(top.getScalarTerm("a")).getFanoutEndpoints()
        elapsed = time.perf_counter() - start
        self.assertEqual(2, len(rows))
        self.assertEqual(1, len(fanout))
        self.assertLess(fanin, 1.0)
        self.assertLess(elapsed, 1.0)
        self.assert_cones(top, [z])
        print(f"XOR depth 24: fanin={fanin*1000:.3f}ms fanout={elapsed*1000:.3f}ms", flush=True)

    def test_structural_reset_fanout_ten_thousand(self):
        top = self.load('''module sample(input a, b, d, input [9999:0] data,
            output reg reset, output reg [9999:0] q);
            always @(posedge a) reset <= d;
            for (genvar i=0; i<10000; i=i+1) begin
                always @(posedge b or negedge reset)
                    if (!reset) q[i] <= 1'b0; else q[i] <= data[i];
            end
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.analyze()
        source = self.state_output(top, "reset")
        start = time.perf_counter()
        rows = source.getFanoutEndpoints(withCells=False)
        elapsed = time.perf_counter() - start
        recovery = [r for r in rows if r["kind"] == "recovery"]
        self.assertEqual(10_000, len(recovery))
        self.assertTrue(all("cells" not in r and r["direct"] for r in rows))
        self.assert_cones(top)
        print(f"Reset fanout 10000: {elapsed:.3f}s", flush=True)

    def test_structural_memory_independent_write_clocks(self):
        top = self.load('''module sample(input a, b, e0, e1,
            input [1:0] addr0, addr1, input [2:0] d0, d1,
            output [2:0] read, output reg [2:0] q);
            reg [2:0] storage [0:3];
            always @(posedge a) if (e0) storage[addr0] <= d0;
            always @(posedge b) if (e1) storage[addr1] <= d1;
            assign read = storage[addr0];
            always @(posedge b) q <= read;
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.analyze()
        target = next(e for e in top.getEndpoints() if not e.getInstTerm().getInstance().getModel().hasMemoryInterface())
        rows = [r for r in target.getFaninStartpoints() if r["kind"] == "register"]
        self.assertEqual(2, len(rows))
        self.assertEqual(rows[0]["launchPin"], rows[1]["launchPin"])
        self.assertNotEqual(rows[0]["startpoint"], rows[1]["startpoint"])
        self.assertEqual({frozenset({"a"}), frozenset({"b"})},
                         {frozenset(c.getNLClock().getName() for c in r["launchClocks"]) for r in rows})
        self.assertTrue(all(r["direct"] for r in rows))
        for row in rows:
            self.assertTrue(any(r["endpoint"] == target for r in row["startpoint"].getFanoutEndpoints()))
        self.assert_cones(top)

    def test_structural_clock_gating_and_data_clock(self):
        top = self.load('''module sample(input c, d, output reg source, q, r);
            reg held;
            always @(posedge c) source <= d;
            always_latch if (!c) held <= source;
            wire gated = c & held;
            always @(posedge gated) q <= d;
            always @(posedge source) r <= d;
        endmodule''')
        self.primary(top, "clock", "c")
        self.analyze()
        source = self.state_output(top, "source")
        rows = source.getFanoutEndpoints()
        self.assertIn("clock_gating", {r["kind"] for r in rows})
        self.assertIn("clock", {r["kind"] for r in rows})
        control = next(r for r in rows if r["kind"] == "clock_gating")
        self.assertEqual({top.getClockDomains()[0]}, control["captureClocks"])
        self.assertNotIn(control["endpoint"], top.getEndpoints())
        for row in rows:
            if row["kind"] == "clock":
                self.assertEqual(row["endpoint"], row["registerClock"])
                self.assertTrue(row["registerOutputs"])
                self.assertFalse(row["captureClocks"])
        self.assert_cones(top)
        self.assert_cones(top, [r["endpoint"] for r in rows if r["kind"] in ("clock", "clock_gating")])

    def test_structural_invalid_occurrences(self):
        top = self.crossing()
        for query in ("getFaninStartpoints", "getFanoutEndpoints"):
            pin = naja.SNLOccurrence(top.getScalarTerm("d"))
            with self.assertRaisesRegex(RuntimeError, "analyzeTiming"):
                getattr(pin, query)()
        self.analyze()
        for value in (naja.SNLOccurrence(), naja.SNLOccurrence(next(iter(top.getInstances())))):
            for query in (value.getFaninStartpoints, value.getFanoutEndpoints):
                with self.assertRaises(RuntimeError):
                    query()
        pin = naja.SNLOccurrence(top.getScalarTerm("d"))
        self.assertEqual(pin.getFaninStartpoints(), pin.getFaninStartpoints(True))
        self.assertEqual(pin.getFanoutEndpoints(), pin.getFanoutEndpoints(True))

    def test_structural_clock_used_as_data_and_gated_waveform(self):
        top = self.load('''module sample(input a, b, en, data,
            output reg q, gated_q);
            reg held;
            always_latch if (!a) held <= en;
            wire gated = a & held;
            always @(posedge b) q <= a & data;
            always @(posedge gated) gated_q <= data;
        endmodule''')
        self.primary(top, "a", "a")
        self.primary(top, "b", "b")
        self.analyze()
        q = self.state_output(top, "q")
        data_pin = self.state_pin(q, "D")
        clock_source = naja.SNLOccurrence(top.getScalarTerm("a"))
        rows = data_pin.getFaninStartpoints()
        clock = next(r for r in rows if r["kind"] == "clock")
        self.assertEqual(clock_source, clock["launchPin"])
        self.assertEqual(clock_source, clock["startpoint"])
        self.assertEqual({"a"}, {c.getNLClock().getName() for c in clock["launchClocks"]})
        self.assertEqual(1, len(clock["cells"]))
        self.assertFalse(clock["direct"])
        gated_pin = self.state_pin(self.state_output(top, "gated_q"), "C")
        gated_rows = gated_pin.getFaninStartpoints()
        self.assertTrue(any(r["kind"] == "clock" and r["cells"] and not r["direct"] for r in gated_rows))
        stops = clock_source.getFanoutEndpoints()
        self.assertTrue(any(r["endpoint"] == gated_pin and r["kind"] == "clock" for r in stops))
        self.assertFalse(any(r["kind"] == "output" for r in stops))
        self.assert_cones(top)
        self.assert_cones(top, [gated_pin])

    def test_structural_overlapping_cones(self):
        # Many distinct endpoint pins share a reconvergent trunk. A separate
        # branch adds one cell, so cached suffixes must not mix path cell sets.
        lines = ["module sample(input c, a, b, select, unused,",
                 "output reg [1023:0] q, output reg branch);"]
        x, y = "a", "b"
        for i in range(24):
            lines += [f"wire x{i} = {x} ^ {y};", f"wire y{i} = {x} ^ ~{y};"]
            x, y = f"x{i}", f"y{i}"
        lines += [f"wire shared = {x} ^ {y};",
                  "always @(posedge c) q <= {1024{shared}};",
                  "always @(posedge c) branch <= shared & select;", "endmodule"]
        top = self.load("\n".join(lines))
        self.primary(top, "clock", "c")
        self.analyze()
        endpoints = top.getEndpoints(check="setup")
        branch = self.state_pin(self.state_output(top, "branch"), "D")
        shared = [e for e in endpoints if e != branch]
        self.assertEqual(1024, len(shared))
        expected = shared[0].getFaninStartpoints()
        self.assertEqual(2, len(expected))
        expected_sparse = [{k: v for k, v in row.items() if k != "cells"} for row in expected]
        start = time.perf_counter()
        for endpoint in shared:
            self.assertEqual(expected, endpoint.getFaninStartpoints())
        first = time.perf_counter() - start
        branch_rows = branch.getFaninStartpoints()
        for row in expected:
            extended = next(r for r in branch_rows if r["launchPin"] == row["launchPin"])
            self.assertTrue(row["cells"] < extended["cells"])
            self.assertEqual(1, len(extended["cells"] - row["cells"]))
        start = time.perf_counter()
        for endpoint in reversed(shared):
            self.assertEqual(expected_sparse, endpoint.getFaninStartpoints(False))
            self.assertEqual(expected, endpoint.getFaninStartpoints())
        repeated = time.perf_counter() - start
        # Warm a shared downstream suffix, then query a different upstream pin.
        # The prefix cells must be combined with each cached endpoint path.
        driver = next(p for p in top.getNet("shared").getComponents()
                      if p.getDirection() == naja.SNLTerm.Direction.Output)
        trunk = naja.SNLOccurrence(driver)
        trunk_stops = trunk.getFanoutEndpoints()
        self.assertEqual(1025, len(trunk_stops))
        source = naja.SNLOccurrence(top.getScalarTerm("a"))
        prefix = next(r for r in expected if r["launchPin"] == source)
        stops = source.getFanoutEndpoints()
        self.assertEqual(1025, len(stops))
        for stop in stops:
            if stop["endpoint"] != branch:
                self.assertEqual(prefix["cells"], stop["cells"])
                self.assertEqual(prefix["direct"], stop["direct"])
        self.assertFalse(naja.SNLOccurrence(top.getScalarTerm("unused")).getFanoutEndpoints())
        self.assert_cones(top)
        print(f"Shared XOR cone, 1024 endpoint pins: first={first:.3f}s mixed-mode reverse={repeated:.3f}s", flush=True)

    def test_structural_order_independence(self):
        def snapshot(order):
            top = self.load('''module sample(input a, b, d, output reg s, q);
                always @(posedge a) s <= d;
                always @(posedge b) q <= ~s;
            endmodule''')
            self.primary(top, "a", "a")
            self.primary(top, "b", "b")
            primitive_lib = naja.NLLibrary.createPrimitives(self.db)
            model = naja.SNLDesign.createPrimitive(primitive_lib, "opaque")
            a = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "a")
            z = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Output, "z")
            # Identical named disconnected cells in opposite creation order.
            for name in order:
                instance = naja.SNLInstance.create(top, model, name)
                instance.getInstTerm(z).setNet(top.getNet("s"))
                instance.getInstTerm(a).setNet(top.getNet("d"))
            top.analyzeTiming()
            q = self.state_pin(self.state_output(top, "q"), "D")
            before = q.getFaninStartpoints()
            for endpoint in reversed(top.getEndpoints()):
                endpoint.getFanoutEndpoints(False)
                endpoint.getFaninStartpoints(False)
            self.assertEqual(before, q.getFaninStartpoints())
            self.assertTrue(before[0]["direct"])
            self.assertEqual(1, len(before[0]["cells"]))  # DB0 inverter
            self.assert_cones(top)
            def identity(occurrence):
                pin = occurrence.getInstTerm()
                instance = pin.getInstance() if pin else occurrence.getInstance()
                name = instance.getName() or f"unnamed_{instance.getID()}"
                return name + ("/" + pin.getBitTerm().getName() if pin else "")
            return [(identity(r["launchPin"]), identity(r["startpoint"]),
                     r["kind"], sorted(identity(c) for c in r["cells"]), r["direct"])
                    for r in before]
        first = snapshot(("left", "right"))
        self.universe.destroy()
        self.setUp()
        second = snapshot(("right", "left"))
        self.assertEqual(first, second)

    def test_occurrence_value_protocols(self):
        top = self.crossing()
        self.analyze()
        source = top.getClockDomains()[0].getSources()[0]
        endpoint = top.getEndpoints()[0]
        for occurrence in (source, endpoint):
            with self.subTest(occurrence=str(occurrence)):
                self.assertTrue(repr(occurrence))
                self.assertEqual(str(occurrence), repr(occurrence))
                self.assertEqual(hash(occurrence), hash(occurrence))
                self.assertEqual(1, len({occurrence, occurrence}))
        same_endpoint = top.getEndpoints()[0]
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
            occurrence.getLaunchClocks()

    def test_caller_hierarchical_occurrence_requires_refresh(self):
        top = self.load("""module child(input a, output y); assign y = a; endmodule
            module sample(input a, output y); child cell0(a, y); endmodule""")
        self.primary(top, "clock", "a")
        instance = top.getInstance("cell0")
        term = instance.getModel().getScalarTerm("a")
        path = naja.SNLPath(instance)
        occurrence = naja.SNLOccurrence(path, term)
        self.analyze()
        self.assertEqual(1, len(occurrence.getClocks()))
        top.getScalarTerm("a").setName("input_data")
        top.analyzeTiming()
        with self.assertRaisesRegex(RuntimeError, "Stale hierarchical occurrence"):
            occurrence.getClocks()
        refreshed = naja.SNLOccurrence(path, term)
        self.assertEqual(1, len(refreshed.getClocks()))

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


    def test_declared_native_sync_roles(self):
        designs = naja.NLLibrary.create(self.db)
        primitives = naja.NLLibrary.createPrimitives(self.db)
        top = naja.SNLDesign.create(designs, "sample")
        model = naja.SNLDesign.createPrimitive(primitives, "sync_cell")
        pins = {name: naja.SNLScalarTerm.create(model, direction, name)
                for name, direction in (
                    ("c", naja.SNLTerm.Direction.Input),
                    ("rst", naja.SNLTerm.Direction.Input),
                    ("d", naja.SNLTerm.Direction.Input),
                    ("q", naja.SNLTerm.Direction.Output))}
        model.setSequentialModel(kind="flip_flop", clocked_on="c",
            states=[dict(name="state", next_state="rst & d")],
            outputs=[(pins["q"], "state")])
        for name, role in (("c", naja.SNLTermRole.Clock),
                           ("d", naja.SNLTermRole.DataInput),
                           ("q", naja.SNLTermRole.DataOutput),
                           ("rst", naja.SNLTermRole.SyncReset)):
            pins[name].setRole(role)
        naja.SNLDesign.addInputsToClockArcs([pins["rst"], pins["d"]], pins["c"])
        naja.SNLDesign.addClockToOutputsArcs(pins["c"], [pins["q"]])
        instance = naja.SNLInstance.create(top, model, "storage")
        for name, pin in pins.items():
            terminal = naja.SNLScalarTerm.create(top, pin.getDirection(), name)
            net = naja.SNLScalarNet.create(top, name)
            terminal.setNet(net)
            instance.getInstTerm(pin).setNet(net)
        self.universe.setTopDesign(top)
        self.primary(top, "clock", "c")
        self.reset(top, term="rst", level=naja.SNLActiveLevel.High)
        top.analyzeTiming()
        domain = top.getResetDomains()[0]
        rows = [row for row in naja.SNLOccurrence(top.getScalarTerm("d")).getFanoutEndpoints()
                if row["kind"] == "register"]
        self.assertEqual(1, len(rows))
        self.assertEqual({domain}, rows[0]["syncResets"])
        self.assertNotEqual(domain, top.getClockDomains()[0])
        self.assertNotEqual(domain, object())
        with self.assertRaises(TypeError):
            domain < domain
        self.assert_cones(top)
        with self.assertRaisesRegex(RuntimeError, "expects an SNLDesign"):
            naja.NLReset.create(None, "bad", [top.getScalarTerm("rst")],
                                naja.SNLActiveLevel.High)

    def test_declared_raw_reset_startpoints(self):
        top = self.load('''module sample(input c, rst_n, d, a, b, output reg q, r);
            wire floating;
            always @(posedge c or negedge rst_n)
                if (!rst_n) q <= 0; else q <= (d & a) | (d & b);
            always @(posedge c or negedge rst_n)
                if (!rst_n) r <= 0; else r <= floating;
        endmodule''')
        self.primary(top, "clock", "c")
        self.reset(top)
        top.analyzeTiming()
        # Reconvergence and an undriven D pin must not create reset synchronizers.
        self.assertEqual([], top.getTimingDiagnostics()["resetSynchronizers"])
        self.assert_cones(top)

    def test_declared_reset_clock_edges(self):
        top = self.load('''module sample(input c, rst, d, output reg q);
            wire derived = c & rst;
            always @(posedge derived) q <= d;
        endmodule''')
        self.primary(top, "clock", "c")
        self.reset(top, term="rst", level=naja.SNLActiveLevel.High)
        top.analyzeTiming()
        domain = top.getResetDomains()[0]
        # Physical and reset-inactive provenance stop at state boundaries.
        self.assertEqual([], top.getTimingDiagnostics()["resetSynchronizers"])
        self.assertEqual(frozenset(), naja.SNLOccurrence(top.getScalarTerm("q")).getResets())
        self.assertEqual({domain}, domain.getSources()[0].getResets())
        self.assert_cones(top)


if __name__ == "__main__":
    unittest.main()
