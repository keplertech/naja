# SPDX-FileCopyrightText: 2026 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Bridge NajaEDA Python logging and the native Naja logger."""

import logging

from . import naja


def _native_level(level: int) -> str:
    if level >= logging.CRITICAL:
        return "critical"
    if level >= logging.ERROR:
        return "error"
    if level >= logging.WARNING:
        return "warning"
    if level >= logging.INFO:
        return "info"
    return "debug"


class NativeLogHandler(logging.Handler):
    """Forward Python records to the thread-safe native logging backend."""

    def emit(self, record: logging.LogRecord) -> None:
        if getattr(record, "_naja_native_record", False):
            return
        try:
            naja.log(_native_level(record.levelno), self.format(record))
        except Exception:
            self.handleError(record)


def configure_native_logging() -> logging.Logger:
    """Install the native handler once on the top-level NajaEDA logger."""

    logger = logging.getLogger("najaeda")
    if not any(isinstance(handler, NativeLogHandler) for handler in logger.handlers):
        logger.addHandler(NativeLogHandler())
    # Forward all standard Python levels; the native logger applies its own level.
    logger.setLevel(logging.DEBUG)
    logger.propagate = False
    return logger


def forward_native_logging(level: str = "info") -> int:
    """Forward native messages to ``najaeda.native``; return a removable handle.

    Attach application handlers to that logger before enabling forwarding.
    Delivery is synchronous on the native logging thread. Console output remains
    enabled; call ``naja.set_console_logging(False)`` to disable it.
    """
    logger = logging.getLogger("najaeda.native")
    levels = {
        "trace": logging.DEBUG,
        "debug": logging.DEBUG,
        "info": logging.INFO,
        "warning": logging.WARNING,
        "error": logging.ERROR,
        "critical": logging.CRITICAL,
    }

    def forward(severity, message):
        logger.log(levels[severity], message, extra={"_naja_native_record": True})

    return naja.add_log_callback(forward, level=level)
