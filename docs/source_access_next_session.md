<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Source access: next session prompt

Continue unified source information management in the repository root.
Follow AGENTS.md. Use the existing graphify graph first if available.
Do not commit or push in Naja or its dependencies without new authorization.
Preserve the substantial existing uncommitted work.
Read `docs/source_information_design.md` and verify the status before editing.

## Implemented in the gate-Verilog session

- SNLRTLInfos remains the shared source-reference model; the legacy location
  and get_source_range() behavior remain independent and unchanged.
- naja-verilog exposes grammar-derived declaration ranges separately from its
  legacy diagnostic location. Module headers, instance names, port/net
  declarations and assign productions have spans. Named built-in gates use
  name spans; anonymous gates use the gate type span. These are partial spans,
  not full definition bodies. Columns are byte positions, not display columns.
- Naja populates declarations without retained source text or ASTs. Instance
  occurrences remain separate from model definitions. Non-ANSI port metadata
  survives deferred interface creation; grouped nets/ports and module instances
  retain attributes on each object. Implicit port nets share port source data
  until an explicit net declaration supplies their declaration.
- `Config::yosysSourceAttributes_`, raw `NLDB.loadGateLevelVerilog(...,
  yosys_source_attributes=True)` and high-level `VerilogConfig` explicitly select
  Yosys provenance; the default is false. There is no banner/filename heuristic.
- `src` origins split on `|` and the last colon. Numeric coordinates are checked
  as uint32; missing/zero coordinates stay unknown. Malformed, empty, reversed,
  overflowing and non-string entries warn and are skipped independently.
  Valid origins retain order and exact duplicates collapse. No language,
  RTL representation or AST link is inferred.
- Raw attributes preserve their Verilog spelling. Only origin paths decode
  backslash/quote/newline/tab/octal escapes. Invalid escapes warn and are skipped.
  The scanner now tokenizes escaped quotes, including in Windows path attributes.
- Python source access is read-only: declarations/origins are populated by native
  loaders. Python setters and input conversion helpers were removed.
- Custom source-reference carrier import/export and its codec were removed.
  Typed references survive NajaIF round trips after the original file is removed.
  Source access for naja-schematic is the current scope.
- Default Bazel now selects both naja-if and naja-verilog source checkouts;
  registered mode clears both overrides. naja-verilog's native Bazel module uses
  BCR Bison/Flex/headers and explicit rules_cc loads. Changes are in the dependency
  checkout itself, with no Naja-owned patches/overlays. Both source and Bazel
  changes are now upstream; the submodule is clean at `4459520` (PR #33).
  The parent gitlink update remains uncommitted.
- naja-if remains at `0d28430f84d61e9d5973f989bfff303325a7510c`.

## Validation and artifacts

- CMake: 153 frontend/exporter CTest checks passed, including the rebuilt
  Python-loader executable. Its initial crashes were stale-header ABI artifacts.
- Parser: 67 tests run through a temporary CMake harness using Naja's existing
  googletest checkout (the parser's own googletest submodule is uninitialized).
  Harness: `/tmp/naja-parser-source-check`; log: `/tmp/naja-parser-tests.log`.
- Raw Python source/attribute integration: 12 tests passed, including Yosys
  normalization and both persistence paths. NLDB loader suite: 35 tests passed.
- High-level Python attribute/source integration: three tests passed.
- Dependency configuration: six unit tests and the live alignment checker passed.
- Sphinx passed with `-D html_theme=alabaster` (RTD theme unavailable locally).
- REUSE: no missing notices; two existing invalid license expressions remain in
  `SNLSVConstructorTestAlwaysComb.cpp` and `SNLSVConstructorTestMemoryInference.cpp`
  due to SPDX marker text inside generated fixture strings. No new files or
  licensing metadata were added by this implementation session.
- Tinyrocket's Yosys-generated benchmark loaded successfully: five designs,
  26,636 instances, 32,165 nets and 457 terms. All instances and terms have
  declarations; 3,508 instances have upstream origins. Ten generated constant
  nets lack declarations. Origins can refer to technology/model source files,
  not just RTL. Details are in the design document.
- Main logs: `/tmp/naja-source-ctest.log`, `/tmp/naja-source-rawtests.log`,
  `/tmp/naja-source-nldbtests.log`, `/tmp/naja-source-high-tests.log`,
  `/tmp/naja-tinyrocket-source-check.log`, `/tmp/naja-source-sphinx.log`,
  `/tmp/naja-source-reuse.log`, `/tmp/naja-source-sync.log`.

## Next precise stage: delivery validation

Bazel selected and analyzed the edited parser checkout, but compilation failed
in rules_cc_autoconf because the macOS toolchain rejected this absolute dependency:

`/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/SDKSettings.json`

See `/tmp/naja-source-bazel.log`. No CMake/Bazel parity or BCR readiness is claimed.
Resolve the local toolchain issue, then run the same frontend, Python API and
round-trip checks against actual Bazel outputs. Do not validate an older parser
or bypass include checking to claim equivalent products.

Command attempted:

```bash
bazel test //test/nl/formats/verilog/frontend:snlVRLConstructorTests \
  @naja-verilog//:naja_verilog_tests
```

naja-verilog source support is upstream in PR #32, and native Bazel source-build
support is upstream in main commit
`4459520a51dd246ce81bc72dd0ca8b1ed75518e6` (PR #33), including the standalone
Abseil override. The local submodule is clean at that revision; the parent
gitlink update is uncommitted. Registry releases remain independent: publish compatible
immutable dependencies and validate registered/downstream consumption separately.
Registered naja-if still predates the source-reference schema.

After updating to `4459520`, dependency synchronization passed and both focused
Bazel targets analyzed successfully. Compilation still failed on the macOS
`SDKSettings.json` absolute dependency, this time in Bison's `bazel_runfiles.cc`;
no tests executed. Log: `/tmp/naja-upstream-4459520-bazel.log`.

## Remaining functional stages

- Preprocessing has no original-file source map. Automatic declarations are
  omitted when preprocessing is enabled; explicit Yosys origins still work.
- NajaIF does not serialize general raw Verilog attributes; typed source roles
  do round-trip. Raw-attribute persistence is separate work.
- Source access supplies file/range references without path relocation,
  filesystem resolution, retained text or AST binding. Linking origins to
  separately loaded RTL is explicit future work.
- VHDL population/alignment, optional AST adapters, transformation provenance
  audits and broader SV source-population coverage remain separate stages.

Mandatory delivery policy: for the same revision, platform and requested
features, CMake/Bazel must deliver equivalent behavior, APIs, defaults,
diagnostics, semantics, serialization and package contents. Matching hashes
alone does not establish parity. BCR publication may lag but must not ship a
different product for the same release. Report any blocked validation explicitly.
