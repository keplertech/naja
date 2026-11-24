# SPDX-FileCopyrightText: 2026 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import io
import logging
import os
import re
import unittest
from unittest import mock

from najaeda import naja
from najaeda import _logging


class NajaedaLoggingTest(unittest.TestCase):
    def setUp(self):
        test_dir = os.environ["NAJAEDA_TEST_PATH"]
        self.log_path = os.path.join(test_dir, "najaeda_native_logging.log")
        if os.path.exists(self.log_path):
            os.remove(self.log_path)
        naja.clearLogSinks()
        naja.addLogFile(self.log_path)
        naja.setLogLevel("debug")

    def tearDown(self):
        naja.clearLogSinks()
        naja.setLogLevel("info")

    def test_package_records_use_native_logger_without_root_duplication(self):
        root_output = io.StringIO()
        root_handler = logging.StreamHandler(root_output)
        root_logger = logging.getLogger()
        root_logger.addHandler(root_handler)
        try:
            logger = logging.getLogger("najaeda.test.forwarding")
            logger.info("python info through native logger")
            logger.warning("python warning through native logger")
            logger.error("python error through native logger")
        finally:
            root_logger.removeHandler(root_handler)

        # Closing the file sink flushes it before inspection.
        naja.clearLogSinks()
        with open(self.log_path, "r", encoding="utf-8") as log_file:
            contents = log_file.read()

        self.assertEqual("", root_output.getvalue())
        self.assertRegex(
            contents,
            re.compile(
                r"\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2},\d{3} "
                r"\[naja\] \[info\] python info through native logger"
            ),
        )
        self.assertIn("[warning] python warning through native logger", contents)
        self.assertIn("[error] python error through native logger", contents)

    def test_forward_native_logging_without_echo(self):
        from najaeda import forward_native_logging
        logger = logging.getLogger("najaeda.native")
        record = logging.LogRecord("najaeda.native", logging.WARNING, "", 0, "native", (), None)
        with (
            mock.patch.object(logger, "handle", wraps=logger.handle) as receive,
            mock.patch.object(naja, "log", wraps=naja.log) as send,
        ):
            handle = forward_native_logging("warning")
            try:
                # Exercise the helper using a real native loader message.
                import pathlib
                import tempfile
                db = naja.NLDB.create(naja.NLUniverse.create())
                try:
                    with tempfile.TemporaryDirectory() as directory:
                        source = pathlib.Path(directory) / "sample.sv"
                        source.write_text("module sample(output [2:0] q); assign q = 5'b10101; endmodule")
                        db.loadSystemVerilog([str(source)], diagnostics_report_path=None)
                finally:
                    naja.NLUniverse.get().destroy()
                send.assert_not_called()
                self.assertTrue(receive.call_count)
                records = [call.args[0] for call in receive.call_args_list]
                self.assertTrue(all(r.levelno == logging.WARNING for r in records))
                self.assertTrue(all(r._naja_native_record for r in records))
            finally:
                naja.remove_log_callback(handle)
        record._naja_native_record = True
        with mock.patch.object(naja, "log") as send:
            _logging.NativeLogHandler().emit(record)
            send.assert_not_called()

    def test_native_level_extremes(self):
        self.assertEqual("critical", _logging._native_level(logging.CRITICAL))
        self.assertEqual("debug", _logging._native_level(logging.DEBUG))

    def test_native_handler_reports_emit_failure(self):
        handler = _logging.NativeLogHandler()
        record = logging.LogRecord(
            "najaeda.test", logging.INFO, __file__, 1, "message", (), None
        )
        with (
            mock.patch.object(_logging.naja, "log", side_effect=RuntimeError("failure")),
            mock.patch.object(handler, "handleError") as handle_error,
        ):
            handler.emit(record)

        handle_error.assert_called_once_with(record)


if __name__ == "__main__":
    unittest.main()
