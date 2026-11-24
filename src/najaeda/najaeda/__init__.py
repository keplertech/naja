# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import importlib


def _load_naja():
    try:
        return importlib.import_module(".naja", __name__)
    except ModuleNotFoundError as error:
        if error.name != f"{__name__}.naja":
            raise
        # Some development layouts expose the extension directly on PYTHONPATH.
        return importlib.import_module("naja")


naja = _load_naja()

from ._version import version, git_hash  # noqa: E402
from ._logging import configure_native_logging, forward_native_logging  # noqa: E402

__version__ = version()

configure_native_logging()

__all__ = ["naja", "version", "git_hash", "__version__", "configure_native_logging", "forward_native_logging"]
