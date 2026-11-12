# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

from copy import deepcopy
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import serial_link_simulation as slink


class SerialLinkSimulationTest(unittest.TestCase):
    def test_burst_coverage_uses_last_and_pinned_argument_order(self):
        # The upstream NUM_READS_1 label actually requests writes. One write
        # can have multiple W beats; all must remain in the compared trace.
        lines = ["TEST PASSED", "[SYS] Links are ready", "NAJA_SLINK_COMPLETE"]
        for endpoint, port in ((1, "in"), (2, "out")):
            for channel, timestamp, data, last in (
                    ("aw", 10, "15", 1), ("w", 20, "31", 0),
                    ("w", 30, "42", 1), ("b", 40, "00", 1)):
                lines.append(f"NAJA_SLINK_TX ep={endpoint} port={port} ch={channel} "
                             f"time={timestamp} data={data} last={last}")
        log = "\n".join(lines)
        coverage, streams = slink.validate(log, (1, 0, 0, 0))
        self.assertEqual(8, coverage["accepted"])
        self.assertEqual(1, coverage["counts"]["1/in/w"])
        self.assertEqual(2, coverage["beats"]["1/in/w"])
        self.assertEqual([("20", "31"), ("30", "42")], streams["1/in/w"])
        for broken in (log.replace("TEST PASSED", ""),
                       log.replace("NAJA_SLINK_COMPLETE", ""),
                       log.replace("data=42 last=1", "data=42 last=0"),
                       log.replace("data=31", "data=xx"),
                       log + "\n%Error: synthetic checker failure",
                       log + "\nAW queue is empty!"):
            with self.subTest(log=broken), self.assertRaises(ValueError):
                slink.validate(broken, (1, 0, 0, 0))

    def test_trace_comparison_rejects_payload_order_time_and_truncation(self):
        original = {"1/in/w": [("10", "a1"), ("20", "b2")],
                    "2/out/w": [("30", "a1"), ("40", "b2")]}
        slink.compare_traces(original, deepcopy(original))
        variants = []
        for changed in (("10", "ff"), ("11", "a1")):
            trace = deepcopy(original)
            trace["1/in/w"][0] = changed
            variants.append(trace)
        trace = deepcopy(original)
        trace["1/in/w"].reverse()
        variants.append(trace)
        trace = deepcopy(original)
        trace["1/in/w"].pop()
        variants.append(trace)
        trace = deepcopy(original)
        trace.pop("2/out/w")
        variants.append(trace)
        for trace in variants:
            with self.subTest(trace=trace), self.assertRaises(ValueError):
                slink.compare_traces(original, trace)

    def test_idle_case_still_requires_upstream_completion(self):
        good = "TEST PASSED\n[SYS] Links are ready\nNAJA_SLINK_COMPLETE\n"
        self.assertEqual({}, slink.validate(good, (0, 0, 0, 0))[1])
        with self.assertRaises(ValueError):
            slink.validate(good.replace("[SYS] Links are ready", ""), (0, 0, 0, 0))

    def test_failed_simulation_retains_partial_transactions_for_comparison(self):
        def failed(command, cwd, log_path, timeout, env):
            log_path.write_text("NAJA_SLINK_TX ep=1 port=in ch=aw time=10 data=12 last=1\n"
                                "NAJA_SLINK watchdog expired\n")
            raise RuntimeError("simulator failed")
        with tempfile.TemporaryDirectory() as directory, patch.object(slink, "execute", failed):
            name, record, trace = slink.run_case((1, 0, 0, 0), "generated", Path(directory), 1, {})
        self.assertEqual("failed", record["status"])
        self.assertEqual("Simulation watchdog expired", record["error"])
        self.assertEqual(1, record["partial_accepted"])
        self.assertEqual({"1/in/aw": [("10", "12")]}, trace)


if __name__ == "__main__":
    unittest.main()
