<!--
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# VHDL frontend regressions

## Language features

`test_soc_frontend.py` and the small `soc_probes/` fixtures exercise sequential
conditional assignment without a final else, process-local declarations,
architecture object aliases, integer-range ports, falling-edge registers,
static string generics, standard simulation-package imports, and initializer
semantics. Exported Verilog cycle checks use Icarus Verilog when available.
Run them with the built raw `naja` module on `PYTHONPATH`, or through
`ctest -R '^VHDLSocFrontend$'` / `bazel test //regress/vhdl:soc_frontend_tests`.
Output defaults and runtime impure function calls remain explicit diagnostics.
Constrained string generics, general runtime file operations,
aliases outside architecture declarative parts, falling-edge asynchronous
resets, and falling-edge inferred memory writes are not added by these probes.


## Static RAM initialization

`memory_file_init.vhdl` and `test_memory_initialization.py` cover read-only TextIO
functions used in static signal/constant initializers. File declarations are
supported in function bodies, with `text` files opened in `read_mode`, local
`line` variables, `endfile`, and the two-positional-argument forms of `readline`,
`read`, and `hread` for logic vectors. Static string formals/defaults and ordinary
local array assignments, conditionals, `for`/`while`, `exit`, and returns are
interpreted at elaboration. Unsupported overloads and operations fail explicitly.
Binary reads accept 0/1 and representable uppercase X/Z values for logic vectors;
hex reads support hexadecimal digits and internal underscores. Missing files,
short/invalid data and nonzero truncated high bits are errors. Relative filenames
use the loader process's working directory. The VHDL function itself determines
address order, zero-fill, EOF handling and whether surplus input is read.

Arrays satisfying the existing synchronous-memory inference rules retain their
initial values on the canonical writable-memory `INIT_ENABLE`/`INIT` parameters.
Inference runs before per-word signal nets are allocated. Uniform binary or X/Z
nibbles are stored in a compact hex literal; mixed X/Z nibbles retain binary
encoding. No runtime file-reader hardware is generated. Memory initialization
survives NajaIF reload without the original data file. Reusing a cached design
after a contributing file changes is rejected; load into a fresh library.
The existing hardware-array size and elaboration-step limits still apply.

Tests include initialized reads, read-before-write behavior, masked writes,
nonzero/descending addresses, EOF/zero-fill, malformed files, a larger initialized
array, changed-file cache validation, and fresh-process snapshot reload. Optional
NVC and Icarus checks compare reference VHDL and exported Verilog behavior. Run
`ctest -R '^VHDLMemoryInitialization$'` or
`bazel test //regress/vhdl:memory_initialization_tests`.
