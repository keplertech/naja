# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Optional schematic viewer: https://github.com/najaeda/naja-schematic."""

try:
    import naja_schematic as _viewer
    from naja_schematic import (
        show,
        diagnosis_response,
        handle_request,
        style_response,
        export_svg,
        export_svgs,
        export_png,
        RenderError,
        __version__,
    )
except ImportError as e:
    raise ImportError(
        'The schematic viewer requires pip install "najaeda[schematic]"'
    ) from e

__all__ = [
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

# Comparison drawings, in naja-schematic releases that have them.
for _name in ("diff_svg", "diff_png"):
    if hasattr(_viewer, _name):
        globals()[_name] = getattr(_viewer, _name)
        __all__.append(_name)
del _name, _viewer
