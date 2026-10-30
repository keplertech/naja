# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import tarfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pulp_benchmarks as pulp


class PulpBenchmarksTest(unittest.TestCase):
    def test_unsupported_warnings_and_incomplete_diagnostics_fail(self):
        success = "summary.status=success\n"
        pulp.validate_diagnostics(success + 'diagnostic stage="naja_elaboration" severity="warning"')
        for report in ("status=in_progress", success + 'severity="error"',
                       success + 'severity="fatal"',
                       success + 'stage="naja_unsupported_warning" severity="warning"',
                       success + 'stage="naja_unsupported_error" severity="warning"'):
            with self.subTest(report=report), self.assertRaises(RuntimeError):
                pulp.validate_diagnostics(report)

    def make_package(self, root):
        package = root / pulp.PACKAGE
        package.mkdir(parents=True)
        (package / "targets.mk").write_text("sample.default.TOP := top\n")
        (package / "source.sv").write_text("module top(input a, output y); assign y=a; endmodule\n")
        pulp.write_manifest(package)
        return package

    def test_prepared_archive_requires_pinned_revision_and_file_hashes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package = self.make_package(root)
            archive = root / "prepared.tar.gz"
            with tarfile.open(archive, "w:gz") as output:
                output.add(package, arcname=pulp.PACKAGE)
            self.assertEqual(root / "rtl" / pulp.PACKAGE, pulp.unpack(archive, root / "rtl"))
            (package / "source.sv").write_text("changed input")
            with self.assertRaisesRegex(ValueError, "SHA-256 manifest mismatch"):
                pulp.validate_package(package)
            pulp.write_manifest(package)
            manifest = json.loads((package / pulp.MANIFEST).read_text())
            manifest["revision"] = "0" * 40
            (package / pulp.MANIFEST).write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, "pinned RTL revision"):
                pulp.validate_package(package)

    def test_missing_or_added_package_files_are_rejected(self):
        for added in (False, True):
            with self.subTest(added=added), tempfile.TemporaryDirectory() as tmp:
                package = self.make_package(Path(tmp))
                if added:
                    (package / "extra.sv").write_text("unexpected source")
                else:
                    (package / "source.sv").unlink()
                with self.assertRaisesRegex(ValueError, "manifest mismatch"):
                    pulp.validate_package(package)

    def test_preparation_fetches_exact_commit_and_caches_verified_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            cache = Path(tmp).resolve()
            def run(command, **kwargs):
                if command == ["bash", "setup.sh"]:
                    prepared = kwargs["cwd"] / "benchmarks"
                    prepared.mkdir(parents=True)
                    (prepared / "targets.mk").write_text("sample.default.TOP := top\n")
                    self.assertEqual("0", kwargs["env"]["SIMULATION"])
                    self.assertEqual("0", kwargs["env"]["KEEP_BUILD_TOOLS"])
                    self.assertEqual(sys.executable, kwargs["env"]["PYTHON"])
            with patch.object(pulp.sys, "platform", "linux"), \
                 patch.object(pulp.subprocess, "run", side_effect=run) as invoked, \
                 patch.object(pulp.subprocess, "check_output", return_value=pulp.REVISION + "\n"):
                package = pulp.prepare(cache)
                self.assertEqual(cache / pulp.PACKAGE, package)
                fetch = invoked.call_args_list[1].args[0]
                self.assertEqual(["fetch", "--depth", "1", pulp.REPOSITORY, pulp.REVISION], fetch[3:])
                invoked.reset_mock()
                self.assertEqual(package, pulp.prepare(cache))
                invoked.assert_not_called()
            (package / "targets.mk").write_text("changed inventory")
            with self.assertRaisesRegex(ValueError, "manifest mismatch"):
                pulp.prepare(cache)

    def test_wrong_checkout_is_rejected_before_running_setup(self):
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(pulp.sys, "platform", "linux"), \
             patch.object(pulp.subprocess, "run") as invoked, \
             patch.object(pulp.subprocess, "check_output", return_value="0" * 40):
            with self.assertRaisesRegex(ValueError, "checkout mismatch"):
                pulp.prepare(Path(tmp))
            self.assertFalse(any(call.args[0] == ["bash", "setup.sh"] for call in invoked.call_args_list))

    def test_target_inventory_rejects_duplicates_and_empty_input(self):
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp)
            manifest = package / "targets.mk"
            for content in ("", "x.y.TOP := top\nx.y.TOP := other\n"):
                manifest.write_text(content)
                with self.assertRaises(ValueError):
                    pulp.targets(package)
            manifest.write_text("x.y.TOP := top\nz.w.TOP := other\n")
            self.assertEqual({"x.y": "top", "z.w": "other"}, pulp.targets(package))

    def test_failed_and_timed_out_workers_cannot_reuse_stale_outputs(self):
        for outcome, expected in ((subprocess.CompletedProcess([], 1), "failed"),
                                  (subprocess.TimeoutExpired([], 1), "timeout"),
                                  (subprocess.CompletedProcess([], 0), "failed")):
            with self.subTest(expected=expected), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                source = root / "design/variants/default"
                source.mkdir(parents=True)
                (source / "filelist.f").write_text("input.sv\n")
                artifacts = root / "artifacts"
                artifacts.mkdir()
                (artifacts / "netlist.v").write_text("stale")
                (artifacts / "design-stats.json").write_text("{}")
                with patch.object(pulp, "run_command") as run:
                    if isinstance(outcome, Exception):
                        run.side_effect = outcome
                    else:
                        run.return_value = outcome
                    result = pulp.run_target(root, "design.default", "top", artifacts, root, 1)
                self.assertEqual(expected, result["status"])
                self.assertFalse((artifacts / "netlist.v").exists())
                self.assertEqual(expected, json.loads((artifacts / "summary.json").read_text())["status"])
                self.assertEqual(1, run.call_args.kwargs["timeout"])
                self.assertEqual(source, run.call_args.kwargs["cwd"])

    def test_dump_timeout_preserves_completed_statistics(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "design/variants/default"
            source.mkdir(parents=True)
            (source / "filelist.f").write_text("input.sv\n")
            artifacts = root / "artifacts"

            def execute(command, **kwargs):
                (artifacts / "design-stats.json").write_text('{"top_terms": 5}')
                raise subprocess.TimeoutExpired(command, 1)

            with patch.object(pulp, "run_command", side_effect=execute):
                result = pulp.run_target(root, "design.default", "top", artifacts, root, 1)
            self.assertEqual("timeout", result["status"])
            self.assertEqual(5, result["design_stats"]["top_terms"])

    def test_slang_args_are_case_specific_in_both_modes(self):
        for case in ("cheshire.default", "redmule.default", "nvdla.top", "cv32e40p.default"):
            for dump in (False, True):
                with self.subTest(case=case, dump=dump), tempfile.TemporaryDirectory() as tmp:
                    root = Path(tmp)
                    family, variant = case.split(".")
                    source = root / family / "variants" / variant
                    source.mkdir(parents=True)
                    (source / "filelist.f").write_text("source.sv\n")
                    with patch.object(pulp, "run_command", return_value=subprocess.CompletedProcess([], 1)) as run:
                        result = pulp.run_target(root, case, "top", root / "output", root, 1,
                                                 dump_netlist=dump)
                    expected = {
                        "cheshire.default": ["--allow-use-before-declare"],
                        "redmule.default": ["--relax-enum-conversions"],
                        "nvdla.top": ["--timescale=1ns/1ps"],
                    }.get(case, [])
                    self.assertEqual(expected, result["frontend_options"]["slang"])
                    actual = [arg.removeprefix("--slang-arg=") for arg in run.call_args.args[0]
                              if arg.startswith("--slang-arg=")]
                    self.assertEqual(expected, actual)

    def test_worker_cli_forwards_compatibility_option(self):
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(sys, "argv", ["pulp_benchmarks.py", "--worker", "top", "--output", tmp,
                                       "--elaboration-only", "--slang-arg=--allow-use-before-declare", "--slang-arg=--relax-enum-conversions"]), \
             patch.object(pulp, "worker") as worker:
            self.assertEqual(0, pulp.main())
            worker.assert_called_once_with("top", Path(tmp).resolve(), False, {"slang": ["--allow-use-before-declare", "--relax-enum-conversions"]})

    def test_lint_failure_is_not_a_load_dump_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "design/variants/default"
            source.mkdir(parents=True)
            (source / "filelist.f").write_text("input.sv\n")
            artifacts = root / "artifacts"

            def execute(command, **kwargs):
                if command[0] == "verilator":
                    raise FileNotFoundError("verilator is missing")
                (artifacts / "netlist.v").write_text("module top; endmodule\n")
                (artifacts / "design-stats.json").write_text('{"top": "top"}')
                return subprocess.CompletedProcess(command, 0)

            with patch.object(pulp, "run_command", side_effect=execute):
                result = pulp.run_target(root, "design.default", "top", artifacts, root, 1, "local")
            self.assertEqual("lint_failed", result["status"])
            self.assertIn("verilator is missing", result["error"])

    def test_lint_inputs_share_the_artifact_directory(self):
        for runner in ("local", "docker"):
            with self.subTest(runner=runner), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                source = root / "design/variants/default"
                source.mkdir(parents=True)
                (source / "filelist.f").write_text("input.sv\n")
                artifacts = root / "artifacts"
                artifacts.mkdir()
                primitives = artifacts / "naja_primitives.v"
                primitives.write_text("stale")

                def execute(command, **kwargs):
                    if command[0] == sys.executable:
                        self.assertFalse(primitives.exists())
                        (artifacts / "netlist.v").write_text("module top; endmodule\n")
                        primitives.write_text("// freshly generated primitive models\n")
                        (artifacts / "design-stats.json").write_text('{"top": "top"}')
                    else:
                        self.assertEqual("// freshly generated primitive models\n", primitives.read_text())
                        inputs = command[command.index("-Wno-ASCRANGE") + 1:]
                        expected = ["netlist.v", "naja_primitives.v"]
                        if runner == "docker":
                            self.assertEqual(1, command.count("-v"))
                            self.assertIn(f"{artifacts}:/work:ro", command)
                            self.assertEqual([f"/work/{name}" for name in expected], inputs)
                        else:
                            self.assertEqual([str(artifacts / name) for name in expected], inputs)
                    return subprocess.CompletedProcess(command, 0)

                with patch.object(pulp, "run_command", side_effect=execute):
                    result = pulp.run_target(root, "design.default", "top", artifacts, root, 1, runner)
                self.assertEqual("passed", result["status"])

    def test_timeout_stops_launcher_children(self):
        with tempfile.TemporaryDirectory() as tmp:
            marker = Path(tmp) / "child-survived"
            ready = Path(tmp) / "child-started"
            child = ("import time; from pathlib import Path; Path(" + repr(str(ready)) +
                     ").touch(); time.sleep(2); Path(" + repr(str(marker)) + ").touch()")
            launcher = "import subprocess, sys, time; subprocess.Popen([sys.executable, '-c', " + repr(child) + "]); time.sleep(10)"
            with self.assertRaises(subprocess.TimeoutExpired):
                pulp.run_command([sys.executable, "-c", launcher], timeout=1)
            self.assertTrue(ready.exists())
            time.sleep(1.2)
            self.assertFalse(marker.exists())

    def test_primitive_free_dump_can_be_linted(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "design/variants/default"
            source.mkdir(parents=True)
            (source / "filelist.f").write_text("input.sv\n")
            artifacts = root / "artifacts"

            def execute(command, **kwargs):
                if command[0] == sys.executable:
                    (artifacts / "netlist.v").write_text("module top; endmodule\n")
                    (artifacts / "design-stats.json").write_text('{"top": "top"}')
                else:
                    self.assertEqual([str(artifacts / "netlist.v")],
                                     command[command.index("-Wno-ASCRANGE") + 1:])
                return subprocess.CompletedProcess(command, 0)

            with patch.object(pulp, "run_command", side_effect=execute):
                result = pulp.run_target(root, "design.default", "top", artifacts, root, 1, "local")
            self.assertEqual("passed", result["status"])

    def test_elaboration_only_does_not_require_a_dump(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "design/variants/default"
            source.mkdir(parents=True)
            (source / "filelist.f").write_text("input.sv\n")
            artifacts = root / "artifacts"

            def execute(command, **kwargs):
                (artifacts / "design-stats.json").parent.mkdir(parents=True, exist_ok=True)
                (artifacts / "design-stats.json").write_text('{"top": "top"}')
                return subprocess.CompletedProcess(command, 0)

            with patch.object(pulp, "run_command", side_effect=execute):
                result = pulp.run_target(root, "design.default", "top", artifacts, root, 1,
                                         dump_netlist=False)
            self.assertEqual("passed", result["status"])
            self.assertNotIn("dump_bytes", result)
            self.assertIn("--elaboration-only", result["command"])


if __name__ == "__main__":
    unittest.main()
