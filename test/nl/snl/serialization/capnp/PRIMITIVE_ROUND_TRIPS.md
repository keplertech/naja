<!--
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# Canonical NLDB0 snapshot audit

`SNLCapnPPrimitiveAuditTest.cpp` inventories the families handled by
`NLDB0::getOrCreatePrimitive`. Each case creates three instances sharing a
canonical model: inherited defaults, explicit defaults, and initialization
parameter overrides where supported. The user database is dumped, the entire
universe destroyed, and the snapshot reloaded. Live state and all four NajaIF
files are compared, including `db0_interface.snl`.

## Coverage matrix

| Family | Configurations | Model identity | Instance configuration |
| --- | --- | --- | --- |
| Assign, full adder | Scalar | Fixed library/design reference | None |
| AND, NAND, OR, NOR, XOR, XNOR | 1, 5, 65 inputs | Library selects operation; design ID selects fan-in | None |
| BUF, NOT | 1, 5, 65 outputs | Library selects operation; design ID selects fan-out | None |
| Mux2 | Widths 1, 5, 65 | Library/design ID and canonical WIDTH default | Explicit WIDTH equal to model |
| Add, subtract, multiply | Widths 1, 5, 65 | Operation library and width/design ID | Explicit WIDTH equal to model |
| Divmod | Signed/unsigned; widths 1, 5, 65 | Signedness library and width/design ID | Explicit WIDTH/SIGNED equal to model |
| Shift | Widths 1, 5, 65; count widths 1, 80; logical left/right and arithmetic right | WIDTH, COUNT_WIDTH, RIGHT, ARITHMETIC | Explicit matching signature; differing signatures are not legitimate overrides |
| Table select | Widths 1, 5, 65; depth 3; address width 2 | WIDTH, DEPTH, ABITS | Explicit matching dimensions |
| D latch | Widths 1, 5, 65 | Family library and width/design ID | Explicit WIDTH equal to model |
| DFF, DFFN, DFFRN, DFFR, DFFS, DFFE, DFFRE, DFFSE | Widths 1, 5, 65 | Family library and width/design ID | Inherited INIT, explicit INIT, sized initialization containing X/Z |
| DFFSR, DFFSRN, DFFSS, DFFSSN, DFFSRE, DFFSRNE, DFFSSE, DFFSSNE | Widths 1, 5, 65 | Family library and width/design ID | Inherited INIT, explicit INIT, sized initialization containing X/Z |
| RAM | Widths 1, 5, 65; depth 3; address width 2; two read/write ports; all five reset modes | Dimensions, port counts, reset mode, independent-clock capability | INIT_ENABLE/INIT; valid reset-enable override tested separately from model defaults |
| ROM | Widths 1, 5, 65; depth 3; address width 2; two read ports | Memory dimensions and zero write ports | Independent INIT_ENABLE/INIT per shared-model instance |
| Masked multi-clock RAM | Widths 1, 5, 65; two read/write ports | Memory signature with independent write clocks | WCLK/WMASK connectivity and per-port modeling; initialization overrides |

All cases check model names/IDs/sharing, parameter types/defaults and explicit
values, terminal names/widths/directions, bit connectivity, timing arcs, term
roles and active levels, and applicable sequential expressions/memory interfaces.
Available combinational truth-table representations are compared as well. The
multi-output BUF/NOT truth-table query is unsupported; their wiring, timing and
canonical interfaces remain covered. This audit does not add that capability.

The existing determinism suite invokes `AllFamiliesArtifacts` in at least five
fresh writer processes with different heap allocations and independent reader
processes. That combined fixture uses widths 1 and 5, includes unused predecessor
models and reverse first-use order, and compares native live state, Verilog
(including primitive sidecars, excluding only standard banner timestamps), DOT
and all NajaIF bytes. The focused native cases cover width 65. SV and VHDL
fixtures additionally check frontend-created models, including VHDL-inferred
memory with inherited reset fields. Every HDL fixture also has a fresh reader.

## Valgrind selection

Normal CTest and determinism runs retain the complete audit matrix and the
combined `AllFamiliesArtifacts` fixture. The Valgrind workflow runs the other
serialization/identity/error-path tests unchanged, then selects a smaller
primitive audit set. It covers every family at width 1, both inherited and
explicit round trips, all memory reset modes, ROM and masked multi-clock RAM,
and wide shift-count/vector-address configurations.

Selected width-65 mux, flop, shift, vector-update and vector-select cases retain
multiword-boundary coverage in both parameter modes. One inherited width-65 RAM
case also retains wide memory-port allocation/reload coverage. The remaining
width-5/65 matrix and `AllFamiliesArtifacts` are omitted only from Valgrind:
repeated full memory-interface queries dominate their instrumented runtime.
No native test assertions or ordinary CTest registrations are removed.

The selection is maintained in `.github/workflows/valgrind.yml`. When adding a
primitive family, extend that selection alongside the full audit inventory.
The same workflow also runs the complete `tgraphTests` executable with full
leak checking and errors for every leak kind.

## Defects and ownership

These defects belong to Naja's serialization integration. No naja-if dependency,
schema, primitive model or canonical library-ID change is needed.

1. Compound reconstruction used explicit instance parameters as model signatures.
   Inherited memory reset fields, shift fields and table dimensions could be
   absent. Snapshot dumping now saves complete DB0 interfaces using the existing
   schema; loading reconstructs them before instances.
2. A supported memory instance reset override could reconstruct a different
   canonical model, changing defaults, roles/modeling and sharing. Instance
   loading now resolves the restored model reference before applying overrides.
3. Compound IDs followed first instance use during reload, losing original model
   creation order and unused predecessor models. The DB0 payload retains all
   existing canonical models, emitted in library/design ID order. Constructors
   recreate them in that order; identity conflicts are rejected.
4. File-path dump operations omitted truncation, leaving trailing bytes when a
   shorter snapshot replaced a longer one. Interface and implementation files
   now use truncation; a focused regression compares overwritten and clean dumps.
5. File-path load operations leaked their descriptors. They now close them on
   success and exceptions, including the additional DB0 payload.

Missing DB0 definitions, incomplete signatures, altered canonical interfaces and
conflicting pre-existing compound IDs are rejected. Existing constructor defaults
are validated, rather than substituted for missing required metadata.

## Compatibility and limits

Snapshots require the additional DB0 payload; older snapshots must be regenerated.
The existing exact-producer manifest checks remain in force. Low-level C++ users
of separate interface/implementation operations must save/load DB0's interface
before the user database. No new raw or high-level Python method is introduced.

The payload includes unused canonical models to preserve DB0 state and subsequent
creation IDs, so its size reflects models already created in the universe, not
just models referenced by the selected user database. Loading into a conflicting
existing universe fails; fresh-universe loading is the round-trip contract.

Truth tables and sequential expressions provide behavioral comparisons where the
native API supports them. This is not exhaustive HDL simulation or a proof of
memory behavior across every input sequence. Build parity requires the common
checks to pass under both build systems; dependency revision agreement alone does
not establish it. BCR publication validation is separate.

## Frontend integration

`VHDLRTLConstructor::finalizeMemories` explicitly copies dimensions and port
counts, while reset parameters may remain inherited. The SystemVerilog memory
creation path normally supplies more explicit parameters. Both use the same
canonical NLDB0 constructors and now receive complete-signature persistence from
serialization; neither frontend needs additional parameter-copying rules.

## Validation on the audit branch

- Before the fix, the new native audit ran 277 tests: 232 passed and 45 failed
  (42 inherited compound-signature cases and three identity/sharing cases).
- CMake built `snlCapnPTests`, `snlTests`, `snlDB0InternalTests`, `naja`,
  `snlSVConstructorTests` and `snlVHDLConstructorTests` successfully.
- The selected CTest serialization, NLDB0 and frontend run passed 875/875 tests.
  Selection: `SNLCapnP|SNLCapNp|SNLDumpManifest|NLDB0|VHDLConstructor|SVConstructor.*(Memory|Shift|Table|Flop|Latch|Arithmetic)`.
- Raw Python `test_nldb.py` passed 39/39 tests, including snapshot and manifest
  coverage.
- The subsequently added low-level separate-interface/implementation regression
  passed 1/1. It includes a memory reset override and initialization data.
- Final `ctest --test-dir build -L determinism --no-tests=error --output-on-failure`
  passed 2/2 CTest entries (setup plus eight Python determinism tests), in 43.07 s.
  Each fixture used five writer and five independent reader processes. This final
  run includes initialization overrides and memory reset overrides in the native
  combined fixture, as well as the VHDL memory fixture.
- `python3 ci/check_submodule_bazel_sync.py` passed for naja-if, naja-verilog and
  slang. Both source build systems select the same shared source revisions.
- Bazel analysis passed for the serialization, kernel, SV frontend and determinism
  targets with `bazel build ... --nobuild`. Actual `bazel test` was blocked before
  Naja compilation: the local Apple toolchain reported Xcode's
  `MacOSX.sdk/SDKSettings.json` as an undeclared absolute include while compiling
  fmt's `src/os.cc`. Therefore CMake/Bazel behavioral parity remains unvalidated
  locally; no BCR readiness is claimed.
- Sphinx HTML build passed with the built-in Alabaster theme and the CMake Python
  extension. The installed Sphinx used Python 3.15 while Naja was built for 3.14;
  the successful build ran Sphinx's installed Python packages under Python 3.14.
- `reuse lint` passed: all 1894 scanned files have copyright and licensing
  information. `git diff --check` passed. No dependency submodule changed.
