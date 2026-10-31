# Naja — Agent Guide

Guidance for AI coding agents (Claude Code reads this as `CLAUDE.md`; Codex reads it as `AGENTS.md` via symlink) working in this repository.

## Codebase questions: use the knowledge graph *if it's there*

This is a large codebase (~1500 files). A graphify knowledge graph makes orienting much faster — but it is **optional and machine-local** (`graphify-out/` is gitignored, so a fresh clone won't have it).

**First, check availability.** The graph is usable only if **both** are true: the `/graphify` skill is installed *and* `graphify-out/graph.json` exists in the project root.

- **If the graph is available** — use it first, before grepping or reading files top-to-bottom:
  - Natural-language question: `/graphify query "how does the SystemVerilog constructor lower always_ff blocks?"`
  - Trace between two concepts: `/graphify path "SNLSVConstructor" "DNL"`
  - Explain one node: `/graphify explain "SNLSVConstructor"`
  - Browse: [`graphify-out/GRAPH_REPORT.md`](graphify-out/GRAPH_REPORT.md) — its "Community Hubs" section is a navigable TOC (*SNLSVConstructor Core Implementation*, *Sequential Assignment Lowering*, *najaeda Instance Query API*, *NL Library Management*, …).

  Graph first for *where* and *how it connects*; then Read/Grep the candidate files for the *exact lines*.

- **If `graphify-out/` is absent or the skill isn't installed** — just use the normal tools (Grep, Glob, Read) and the source layout below. **Do NOT run a full `/graphify` build to answer an ordinary question** — that's a slow, expensive 1500-file extraction and is not worth it for a lookup. Building the graph is opt-in, and only when the user explicitly asks for it.

**Keeping it fresh (only if you already have a graph):** after a non-trivial change, `/graphify . --update` re-extracts just the changed files so future queries stay accurate.

## Local research inputs

Machine-local papers and implementation notes may be present under
`internal/`, which is gitignored. Before implementing a feature, check
`internal/design/` for a relevant design note and `internal/papers/README.md`
for its research sources.

Treat the design note as the implementation contract. A paper is supporting
evidence, not a requirement to implement every idea it contains. When adding a
paper, catalog it and create or update a design note with exact sections,
Naja-specific mappings, non-goals, verification criteria, and unresolved
questions. Do not commit or redistribute papers unless their license permits
it.

## What Naja is

Open-source EDA framework for hardware design loading and transformation — from Verilog and SystemVerilog RTL elaboration through structural netlist analysis, optimization, and editing. Usable from C++ and Python (`najaeda`). See [README.md](README.md) for the public overview.

Two complementary C++ APIs:
- **SNL** (Structured Netlist) — full read/write netlist representation.
- **DNL** (Dissolved Netlist) — fast, read-only flattened view for parallel analysis.

## Source layout

- `src/nl/netlist/` — core netlist model: `snl/`, `pnl/`, `core/`, `decorators/`, `serialization/` (Cap'n Proto interchange), `visual/`.
- `src/nl/formats/` — frontends/backends: `systemverilog/` (slang-based), `verilog/`, `liberty/`, `lefdef/`.
- `src/nl/python/` — Python bindings for the `najaeda` package.
- `src/dnl/` — Dissolved Netlist (flattened, read-only, parallel).
- `src/najaeda/` — Python package (`najaeda/`, `examples/`, `benchmarks/`).
- `src/apps/naja_edit/` — `naja_edit` CLI (optimize/translate netlists).
- `src/app_snippet/` — copy-to-start template for a new C++ tool.
- `src/{bne,core,metrics,optimization}/` — supporting libraries (logic opt: DLE, constant propagation).
- `primitives/` — primitive/standard-cell libraries. `test/` mirrors `src/`. `tutorials/` — the six Colab notebooks.

## NajaIF snapshot compatibility — current status

NajaIF snapshots carry a small `snl.mf` manifest. The immediate objective is
to prevent a snapshot written by a different Naja build from being silently
deserialized into a truncated or otherwise incorrect netlist.

- The manifest writes `V <major> <minor> <revision>` (the legacy format/schema
  revision) and `P <naja-version> <git-hash>` (the producer identity).
- `SNLCapnP::load()` reads this manifest before either Cap'n Proto payload.
  It retains the strict `V` check and, for now, also requires an exact match
  of both producer values with `naja::NAJA_VERSION` and
  `naja::NAJA_GIT_HASH`. A mismatch, or a legacy manifest without `P`, throws
  `SNLDumpException`; callers must regenerate the snapshot.
- `naja.snapshotManifest(path)` reads only `snl.mf` and returns
  `schema_version`, `producer_version`, and `producer_git_hash`, without
  creating an `NLUniverse` or loading payloads.
- This exact-build producer gate is deliberately temporary and conservative.
  The remaining design work is to define and maintain a schema version owned
  by `thirdparty/naja-if`, then use that version as the durable compatibility
  contract so compatible Naja builds can exchange snapshots.

Relevant implementation: `SNLCapnP.cpp`, `SNLDumpManifest.cpp`, and
`PyNLDB.cpp`; focused tests live under `test/nl/snl/serialization/capnp/` and
`test/nl/python/naja_wrapping/test_nldb.py`.

## Build & test

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$NAJA_INSTALL
make && make test && make install
```

- Build dirs already present: `build/`, `build-coverage/`, `build-coverage-svconstructor/`. Prefer building in an existing one to reuse the CMake cache.
- Tests are CTest-driven; run `ctest` (or `make test`) from the build dir. Test sources live under `test/` mirroring `src/`.
- Python usage after install needs `export PYTHONPATH=$PYTHONPATH:$NAJA_INSTALL/lib/python`.
- Build deps (macOS): `brew install cmake capnp tbb bison flex boost` and put flex/bison on `PATH`.

## Build and delivery policy: CMake, Bazel, and BCR

**Product equivalence is mandatory.** For the same Naja revision, platform and
requested features, choosing CMake or Bazel must not change the delivered product.
This applies to Naja, najaeda, command-line tools, libraries, wheels, containers
and any other package or distribution. Public C++ and Python APIs, supported
formats and language constructs, defaults, diagnostics, netlist semantics,
serialization, primitive models and package contents must remain equivalent.
Build-system differences must not introduce missing features, different results
or incompatible snapshots. Such a difference is a bug to fix, not an accepted
property of either build path.

Matching dependency commit hashes is necessary but not sufficient: registry
patches, generated parser/schema code, compile definitions, feature flags,
version/producer identity and packaging rules must preserve the same behavior.
When changing a build path or its dependencies, run the relevant common tests
against both outputs and compare observable results, including package/API
checks where applicable. Do not treat successful compilation alone as parity.
Any validation blocked by the local environment must be reported explicitly.

BCR publication may advance independently, but that is a scheduling distinction,
not permission to ship a different product for the same Naja release. Each BCR
release must satisfy the same product-equivalence requirement as its CMake and
source-based Bazel counterparts before publication.

Naja has three delivery paths:

- CMake builds from sources, for users and for packages such as najaeda wheels
  and Docker images.
- Bazel builds from sources, for users and for package builds. This is a normal
  supported source build path, not a development-only mode.
- Bazel Central Registry (BCR) distribution, published and validated on its own
  schedule.

**Align source builds, decouple registry publication.** CMake and source-based
Bazel builds must use the same revisions of shared source dependencies. The
intended default Bazel configuration uses the same dependency checkouts as
CMake, including changes not yet published in a registry. Native Bazel build
files belong in each dependency's main branch; fix dependencies there rather
than adding Naja-owned patches or overlays.

BCR versions may lag source builds. Registry publication must not block source
changes or source/package builds. Keep registered dependency versions immutable;
publish new versions when preparing BCR distribution. Validate that distribution
with registry dependencies and no local overrides, including Naja as a dependency
of another project. A successful source build does not establish BCR readiness.
BCR publication requires a compatible, validated set of registered dependencies;
it does not require registry pins to match the latest source checkout at all times.

**Current configuration.** Default Bazel builds use
`--override_module=naja-if=%workspace%/thirdparty/naja-if` from .bazelrc, so
initialize that submodule before building. `--config=registered` clears the local
override and resolves the version in MODULE.bazel. naja-if main now contains its
native Bazel support; no separate bazel-support descendant is required for source
builds. CMake remains the established install/packaging workflow; Bazel currently
exposes build/test targets.

The source CI workflows initialize naja-if and run default Bazel builds/tests.
`ci/check_submodule_bazel_sync.py` verifies source checkout selection. Slang lacks
a native module, and naja-verilog's registry build adaptations are still needed;
these two dependencies temporarily remain registry-backed with exact source
revision checks against CMake. Direct checkout selection for them requires native
upstream Bazel support; do not add Naja-owned build patches to bypass that work.
Googletest deliberately uses different sourcing mechanisms and remains excluded.

`bazel-registered.yml` runs independently through workflow_dispatch when preparing
BCR distribution. It uses `--config=registered` without initializing submodules.
The checker's `--registered` option checks configuration, not API compatibility;
actual registered builds/tests and downstream consumer validation establish BCR
readiness. Registry lag is allowed to fail that independent validation without
blocking source changes. Do not claim BCR readiness from default source tests.

## Bazel: a BCR-ready module

naja's Bazel build is meant to be published to the Bazel Central
Registry (BCR) and consumed by other modules (kepler-formal does) with a
plain `bazel_dep`. Keep it that way. Invariants:

- `MODULE.bazel` contains only `bazel_dep`s. No `http_archive`,
  `git_override`, `archive_override`, module extensions or repository
  rules of our own. Select source checkouts through the root build configuration,
  keeping registered versions in MODULE.bazel for BCR consumers. Local overrides
  do not propagate to consumers and must be disabled for BCR validation.
- Nothing runs cmake/make inside Bazel (no `rules_foreign_cc`), and
  nothing is found on the host (`PATH`, pkg-config, `python3-config`,
  system headers). Tools and libraries come from Bazel modules: bison
  and flex rules from BCR `bison`/`flex`, Python from `rules_python`.
- Load every rule (`@rules_cc//cc:cc_library.bzl`,
  `@rules_shell//shell:sh_test.bzl`, …); Bazel 9 has no native ones.
- Fix a dependency in its own module (registry entry or upstream), never
  by patching its BUILD files from this repository, and never by asking
  consumers to patch naja.

**Where registered dependencies come from.** BCR distribution resolves registry
versions independently of the source build checkouts. A module version that is
not on BCR yet can come from its open
bazel-central-registry pull request: `.bazelrc` lists the PR's commit
(`https://raw.githubusercontent.com/<fork>/bazel-central-registry/<sha>/`)
ahead of BCR, and Bazel takes each `name@version` from the first
registry that has it. Drop the line once the PR merges. Unreleased
commits use `<release>-<YYYYMMDD>-<commit>` versions. Never change a
version's contents once something depends on it; add a new version.
When a PR needs a fix, push a new commit (don't force-push, so pinned
commits stay reachable) and move the pin.

**Publishing to BCR**: one module per bazel-central-registry pull
request, each based on BCR main, so each goes in on its own. naja itself is
published from a release tag by the publish-to-bcr app, using the
templates in `.bcr/` (maintainers: xtofalex, nanocoh), once its
dependencies are on BCR. BCR rejects symlinks in entries, so
`overlay/MODULE.bazel` is a copy.

**Known traps** (each already fixed; don't reintroduce):

- hermetic toolchains (BCR `llvm`, used by kepler-formal) pass libc
  headers as early `-isystem` flags. Libraries whose own headers must
  shadow libc's (gnulib in bison) need `-I`, not `includes = [...]`;
  hence the registry's `bison 3.8.2.bcr.10`. Building only with the host
  toolchain hides this class of bug, and also hides headers leaking in
  from `/usr/include` (slang's `boost/regex.hpp` did).
- `cc_shared_library` (`naja_runtime`) drops linker inputs its graph
  aspect cannot see: rules must advertise `CcInfo` and own their linker
  inputs. That's why `src/nl/python/pyloader/python_libs.bzl` re-owns
  libpython instead of using `current_py_cc_libs` directly. Libraries
  linked into `naja_runtime` that a binary also uses (TBB, zlib) must be
  in its `exports_filter`, or the binary links a second copy.
- Root source builds collect `NAJA_GIT_HASH` through
  `ci/bazel_workspace_status.sh`, matching CMake's checkout identity. Header
  generation uses hermetic Python and declared stable-status inputs. Dependency
  builds use the module version (`src/core/naja_version.bzl`), never the consumer
  workspace's Git hash; registry validation remains independent.

**Validation:** source-based Bazel changes need build/test checks against the
same shared source revisions as CMake. Before publishing BCR updates, also build
Naja as a dependency with a hermetic toolchain and registry dependencies:
kepler-formal's tests (`bazel test //... --override_module=naja=<path to this
checkout>`) are the reference consumer. Do not assume Naja's root .bazelrc
configuration is inherited by that consumer.

## Determinism is a correctness requirement

For identical ordered inputs, options, build and initial universe, loading and
transforming a design must reproduce object creation order, names, IDs and
connectivity across processes. Verilog dumps may differ only in banner timestamps;
DOT and NajaIF snapshots (including manifests) must match byte for byte.

- Never let hash iteration or pointer-address ordering drive object creation,
  merging, naming counters, representative selection or emission. Unordered
  containers are fine for lookup and membership; use explicit insertion order,
  scoped declaration order or stable NLIDs for observable traversals.
- Sorting only a dump is insufficient: live IDs and names must already agree.
  Sort keys must distinguish relevant objects; never break ties with pointers.
- New creation paths and ordering fixes need a synthetic cross-process regression
  in `test/najaeda/test_najaeda_determinism.py`. Preserve comparisons of live state,
  all dump artifacts and snapshot round trips; do not normalize new differences
  away. Keep private designs out of tracked fixtures and CI artifacts.
- Run `ctest --test-dir build -L determinism --no-tests=error --output-on-failure`
  or `bazel test //test/najaeda:najaeda_determinism_tests --test_output=errors`.
  Build-path changes require both, under the product-equivalence policy above.

See [the determinism strategy](test/najaeda/DETERMINISM.md) for coverage, stress
runs and the review checklist. Linux/macOS CMake and Bazel CI run the contract;
the Linux najaeda workflow also stress-tests it weekly.

## Conventions

- Whenever the `najaeda` version is incremented, update the pinned `najaeda` version in all Colab tutorials under `tutorials/notebooks/` and the local installation command in `tutorials/README.md` in the same change.
- Every new file, including test fixtures and helper scripts, must have both copyright and licensing information. For Naja-authored files, add `SPDX-FileCopyrightText: <year> The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>` and `SPDX-License-Identifier: Apache-2.0` using the file format's comment syntax. Preserve third-party attribution and licenses. For files that cannot contain comments, add coverage in `.reuse/dep5`. Before finishing a change that adds files or changes licensing metadata, run `reuse lint` and fix any missing copyright or licensing information introduced by the change.
- Match the surrounding code's style, naming, and comment density — the SNL layer uses `NL*`/`SNL*` prefixes; follow the local idiom.
- The SystemVerilog frontend is built on **slang**; sequential lowering and always-block handling live in `SNLSVConstructor` and the "Sequential Assignment Lowering" community — query the graph before touching them.
- Post-elaboration netlists must never silently encode an unsupported construct incorrectly. If faithful lowering is not available, reject the construct or emit a clear diagnostic rather than dropping a value, leaving a net undriven, or otherwise producing a plausible but wrong netlist.
- Prefer general frontend support over fixes tailored to one reproducer: identify the shared language semantics and cover representative variations in syntax, type/shape, and downstream use where practical.
- New DB0 primitives (flops, etc.) follow a canonical ID scheme resolved on capnp load; don't invent ad-hoc primitive IDs.
- NLDB0 primitives must be frontend independent and usable by both SystemVerilog and VHDL loaders, as well as direct C++/Python construction where exposed. Define their contracts in terms of language-neutral values, operations and parameters; keep AST types and language-specific elaboration rules out of the primitive model. Each frontend must explicitly preserve its own typing, indexing, bounds, value-domain and scheduling semantics when mapping to that contract. Do not silently impose SV behavior on VHDL or the reverse. Validate equivalent mappings from both languages wherever supported, alongside direct primitive-model tests.
- Every NLDB0 primitive must support NajaIF save and load. Preserve its canonical identity, signature, instance parameters, connectivity and semantic modeling across snapshot round trips, including reload in a fresh process that has not previously created the primitive. Add focused serialization regressions for new families and capability changes; reconstruct derived modeling deterministically on load where appropriate. Extend the naja-if schema in its owning dependency when required rather than silently dropping unsupported state or relying on process-local caches.
- When working on HDL frontends or elaboration, always stop and discuss with the user when a needed primitive or primitive capability is missing. Explain the missing representation and the options for extending an existing NLDB0 primitive or adding a new one. Wait for agreement before implementing a substitute lowering, expanding the construct into other primitives, or changing the primitive model. This applies even when an alternative lowering could preserve behavior; do not let design-loading progress bypass this discussion.
- `*.py~`, `*.txt~`, `build*/`, and `graphify-out/.venv*` are local artifacts — don't edit or commit them.

## VHDL and SystemVerilog loading alignment

Keep VHDL and SystemVerilog loading behavior and APIs parallel wherever practical.
Use the existing SystemVerilog loader as the reference for API names, defaults,
configuration options, diagnostics, warning deduplication and suppression, report
file routing, and error handling. Apply this consistently across the C++ loaders,
raw Python bindings, and high-level `najaeda.netlist` APIs. Prefer matching an
existing SV convention over introducing a VHDL-specific one. Differences should
reflect language requirements or clearly documented implementation limits; update
the relevant documentation and focused tests when introducing such differences.

HDL loading uses `NLLibrary` as the destination and logical-library identity.
Both Python VHDL and SV loaders accept `library="DESIGN"`; basic library names
match case-insensitively, extended names exactly, and ambiguous root names are
errors. Lookup stays within root libraries of the same `NLDB`, without recursive
or cross-database fallback. In VHDL, `work` always denotes the library owning the
referenced source unit, including imported package contexts. Retained sources,
packages, model caches, and elaborated designs must keep that ownership. `std`
and `ieee` use explicit built-in providers for the supported language subset.

## Primitive timing-model alignment

`SNLDesignModeling.h` is the canonical C++ primitive timing-model API. Timing
metadata can be populated by the Liberty frontend, by direct C++ construction
of NLDB0 primitives, and by the Python primitive libraries under
`src/najaeda/najaeda/primitives/`. Keep these three paths aligned: when adding
or changing timing arcs, timing parameters, term roles, active levels, or
related queries, verify that the raw `najaeda.naja` bindings expose the feature
needed to express the same model from Python and update the Python primitive
loaders where applicable. Add or update focused tests for both the raw bindings
and the affected Python primitive libraries so that equivalent decorations do
not silently drift apart.

## najaeda Python API documentation

When changing either Python API level exposed by the `najaeda` package, update the package documentation in `src/najaeda/najaeda/docs/source/` in the same change. This includes both the high-level `najaeda.netlist` API and the raw compiled `najaeda.naja` / `naja.so` API.

This applies to:
- the high-level API in `src/najaeda/najaeda/netlist.py`;
- Python helper modules such as `instance_visitor.py`, `net_visitor.py`, `stats.py`, and `pandas_stats.py`;
- raw Python bindings under `src/nl/python/naja_wrapping/`, including exported module functions, exception types, classes, constructors, enum-like values, and public methods;
- changes in underlying SNL/NL C++ APIs when they alter what the raw Python bindings expose or how raw Python users should call them.

Documentation expectations:
- High-level API changes should update `api.rst`, the relevant class guide page, or the user guide pages (`concepts.rst`, `quickstart.rst`, `loading.rst`, `editing.rst`).
- Raw `najaeda.naja` / `naja.so` API changes should update `raw_api.rst`, including the expert reference table when public raw classes, module functions, exceptions, enum-like values, constructors, or methods change.
- If a raw binding change makes a new native feature usable from Python, document when experts should use the raw API directly and whether a high-level `najaeda.netlist` wrapper should also be added.
- Workflow or example changes should update `examples.rst.in` or the relevant guide page.

Before finishing a documentation-impacting change, run a Sphinx build when the local environment supports it:

```bash
sphinx-build -b html src/najaeda/najaeda/docs/source /tmp/najaeda-docs-check
```

If `sphinx_rtd_theme` is not installed locally, use the built-in theme override:

```bash
sphinx-build -b html -D html_theme=alabaster src/najaeda/najaeda/docs/source /tmp/najaeda-docs-check
```

## Confidential inputs and maximum anonymization

Treat user-supplied designs, benchmarks, logs, screenshots, and reproductions as
confidential unless their public provenance and redistribution permission are
established. Permission to analyze an input is not permission to publish it.

- Never copy confidential inputs or recognizable excerpts into tracked files,
  tests, benchmarks, documentation, notebooks, generated outputs, commit messages,
  PR descriptions, issues, or public CI artifacts. Keep originals and detailed
  audit findings outside the repository; do not upload them to external services
  or use their identifying strings in web searches.
- Reproduce the underlying language or API behavior with a minimal, independently
  written synthetic example. Anonymize maximally: use neutral filenames, test
  names, modules, cells, libraries, instances, signals, parameters, and comments.
  Remove company/product/project/person names, customer references, internal URLs,
  email addresses, absolute paths, source attributes, timestamps, tool banners,
  notebook outputs, metadata, and other provenance clues.
- Renaming is not sufficient. Do not preserve distinctive hierarchy, circuitry,
  algorithms, constants, lookup tables, dimensions, timing/power/process data, or
  cell models from a confidential design. Use unrelated synthetic values and
  structures while retaining only the semantics required to demonstrate the bug.
  If a safe synthetic reproduction cannot be made, keep the regression private
  and ask for explicit publication authorization before adding any derived input.
- Apply the same review to compressed fixtures, snapshots, archives, images,
  generated netlists, expected diagnostics, build/package contents, and reports.
  Before delivery, inspect the entire proposed diff, filenames, and generated
  artifacts for identifying strings and recognizable confidential content.
  Describe fixes by generic behavior, without naming the source of a private bug.
- Preserve required copyright/license notices and legitimate public dependency,
  standard, and API names. Do not disguise third-party code as Naja-authored code.
  If redistribution rights are unclear, replace the material with a synthetic
  example or keep it private rather than deleting its attribution.
- Report suspected existing leaks privately with locations and uncertainty.
  Cleaning the current tree does not remove Git history or published artifacts;
  flag those separately and do not rewrite history or remove releases without
  explicit authorization.

## Git

- Default/PR base branch: `main`. Don't commit or push unless asked. Use a
  feature branch and PR by default, including when starting on `main`.
- When the user explicitly requests a direct commit or push to `main`, routine
  maintenance such as README/documentation edits and version updates may bypass
  the PR workflow using the repository administrator bypass. Keep the change
  scoped to the requested maintenance and run the applicable checks. This
  exception does not authorize unrelated changes or force pushes.
- Create feature branches with `git switch --no-track -c <branch> origin/main`.
  Never configure a feature branch to track `origin/main`.
- Before pushing, verify the current branch and its configured upstream. Push
  feature branches to an explicit destination, for example
  `git push -u origin HEAD:refs/heads/<branch>`, and confirm that the destination
  is the intended feature branch. Never push a feature branch to `main`.
- For an explicitly requested direct maintenance push, verify that the current
  branch is `main`, check its upstream, and push to the explicit destination
  `git push origin HEAD:refs/heads/main`.
