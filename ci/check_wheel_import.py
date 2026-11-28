# SPDX-FileCopyrightText: 2025 The Naja authors
# <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import najaeda

p = najaeda.__file__
print("Imported najaeda from:", p)

if "site-packages" not in p.replace("\\", "/"):
    raise RuntimeError(
        "najaeda was NOT imported from the installed wheel "
        f"(loaded from {p})"
    )

package_version = getattr(najaeda, "__version__", None)
if package_version is None and hasattr(najaeda, "version"):
    package_version = najaeda.version()
print("najaeda version:", package_version or "unknown")
print("Basic wheel import check: OK")

# Inspect the runtime selected by this interpreter after loading the wheel. DLL
# discovery during wheel repair does not prove which copies Windows loads.
if sys.platform == "win32":
    import os
    import subprocess

    command = (
        f"(Get-Process -Id {os.getpid()}).Modules | "
        "Where-Object { $_.ModuleName -match '^(msvcp|vcruntime|ucrtbase|naja)' } | "
        "ForEach-Object { [PSCustomObject]@{ "
        "Name=$_.ModuleName; Path=$_.FileName; Version=$_.FileVersionInfo.FileVersion } } | "
        "ConvertTo-Json"
    )
    print("Python runtime:", sys.version, flush=True)
    try:
        result = subprocess.run(
            ["pwsh", "-NoProfile", "-NonInteractive", "-Command", command],
            capture_output=True, text=True, timeout=30)
        print("Loaded Windows runtime DLLs:", result.stdout, flush=True)
        if result.returncode:
            print("Runtime inspection failed:", result.stderr, flush=True)
    except (OSError, subprocess.TimeoutExpired) as error:
        print("Runtime inspection unavailable:", error, flush=True)
