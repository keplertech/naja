<!---
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# Regressions

- [`sv/`](sv/README.md): pinned external SystemVerilog load/dump, lint, and
  functional simulation regressions. Case-specific files live in `sv/cases/`,
  shared helpers in `sv/common/`, and runner unit tests in `sv/tests/`.
- [`vhdl/`](vhdl/README.md): synthetic VHDL frontend and RAM initialization
  regressions, with optional reference and exported-Verilog simulation checks.
- `designs/verilog/`: legacy design regressions using `Makefile.inc`.
- `liberty/`: machine-local Liberty regression inputs, when available.

Build products and external checkouts belong under `build/`, not here.
