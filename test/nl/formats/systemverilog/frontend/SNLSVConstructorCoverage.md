<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# SNLSVConstructor Coverage Notes

This document tracks deliberate coverage exclusions and targeted fallback tests
for `src/nl/formats/systemverilog/frontend/SNLSVConstructor.cpp`.

## Targeted Fallback Coverage

The test suite includes dedicated benchmarks that exercise non-happy paths that
are still expected in real designs:

- `continuous_mul_zero_supported`: multiply-by-zero path in `createMultiplyAssign`.
- `continuous_eq_ne_failure_paths_unsupported`: equality / inequality fallback
  failures for non-integral and unknown operands.
- `continuous_shift_right_unresolved_skipped`: unresolved RHS net in logical
  right shift.
- `continuous_shift_left_unknown_amount_supported`,
  `continuous_shift_right_unknown_amount_supported`, and
  `continuous_logical_shift_right_unknown_amount_supported`: unknown literal
  shift amounts reaching the shared unknown-as-zero fallback.
- `parseContinuousShiftRightUnsupportedAmountExprReportsReason` and
  `parseContinuousShiftLeftUnsupportedAmountExprReportsGenericReason`:
  unsupported shift amount expressions reaching the shared shift-amount
  diagnostic fallback with and without a caller-provided reason string.
- `parseContinuousAssignConfigRangeCheckFunctionUnknownShiftAddressExprSupported`:
  unknown literal shift amount fallback when lowering an address expression
  inside a config range-check function call.
- `seq_add_unbased_x_literal_supported`: unbased unsized unknown literal in
  sequential add, reaching the shared unknown-as-zero arithmetic fallback.
- `continuous_sub_unknown_operand_supported`: sized unknown literal in
  continuous subtraction, reaching the shared unknown-as-zero arithmetic
  fallback.
- `continuous_resolve_expression_bits_failure_paths_unsupported`: targeted
  `resolveExpressionBits` fallback failures in continuous assign for unknown
  integer constants, call-cast recursion, nested binary recursion,
  concatenation recursion, and member-access base resolution.
- `parseCountOnesOperandResolveFailureUnsupported`: `$countones` operand
  bit-resolution failure after the operand width has been established.
- `resolveExpressionBitsPropagatesFullWidthFailureBeforeNarrowing`: failure
  propagation before narrowing logical/arithmetic right shifts, division, and
  remainder when an operand function contains an unsupported case-inside range.
- Direct inferred-memory write coverage includes constant conditional writes,
  nested packed constant-plus-dynamic selectors, rejection of unsupported
  two-dynamic-selector and system-call-selector writes, and nested duplicate
  shadow commits.
- `dynamicPackedFieldAssignmentOwnership` checks disjoint packed-field drivers
  and next-state values for dynamic element writes, ascending and descending
  fields, members of dynamically selected packed structs, and asynchronous reset.
  The Python determinism suite checks live state, dumps, and snapshot round trips
  for dynamic packed-field writes across processes.
- Diagnostics report option paths are covered with explicit tests for:
  no-diagnostics content, non-empty diagnostics content, and empty report-path
  validation.
- `instance_arrays`: recursive lowering of module and interface instance arrays,
  including a two-dimensional module array and interface-array elements
  selected from generated blocks.
- `anonymousBusAssignAliasUnsafeMappingsAreRetained`: a producer bus whose
  assignments feed different destination buses keeps every assignment and its
  original connections.
- `parseContinuousMuxDirectConstantsAndDestinations`: zero constants select
  the zero-net cache and reuse the same net across mux bits and instances.
- `parseAutomaticVariableNonzeroBasedSelects`: all 128 binary inputs check
  scalar and wide dynamic reads of procedural locals with nonzero bounds in
  both range directions, repeated reads of a stable net (cache insertion and
  reuse), package constants (equal candidate-bit shortcut), out-of-range reads,
  and a fixed indexed slice. LLVM coverage confirms the scalar and vector mux
  fallbacks and the following range-select dispatch execute. The vector mux
  block and equal candidate-bit shortcut are no longer excluded.

The `SNLSVConstructorTestAlwaysComb.*Coverage` tests also cover:

- Exhaustive return-function cases without a default, wide non-exhaustive cases,
  and unsupported statements in a default branch.
- Nested concatenation streaming targets, checked bit by bit against their
  source connections; dynamic storage, dynamic selections, unresolved storage,
  unresolved RHS expressions, and dynamically sized RHS streams.
- Missing streaming storage through both the assigned-bit mask and replay
  resolvers, including recursive concatenation failure propagation.
- Sequential streaming rejection during scheduling replay: a blocking write
  followed by a read into another target forces this path. A separate test
  checks the earlier combinational nonblocking-assignment diagnostic.
- Independent latch condition calls and side effects, real conditions and
  targets, delayed nonblocking writes, unsupported data expressions, local
  declarations, empty statements, and constant true/false guards.
- An unsigned loop-step parameter exceeding `INT64_MAX`, and a compound shift
  whose count cannot be lowered.

## Deliberate LCOV Exclusions

The lines below are marked `LCOV_EXCL_LINE` because they require internal
primitive-construction failures or structurally unreachable null conditions in
the current architecture:

- Dynamic element-select fallback: vector mux construction failure remains
  excluded after both candidate vectors have been validated to have the same
  nonzero element width. The constant-true selector shortcut remains excluded
  because constant in-range selectors return through the earlier direct slice.
- `createSubAssign`: unary gate creation failure (`return false` after
  `createUnaryGate`).
- `createSubAssign`: full-adder instantiation failure (`return false` after
  `createFAInstance`).
- `createEqualityAssign`: null XNOR output after preallocated output-net call
  to `createBinaryGate`.
- `createEqualityAssign`: reduction AND creation failure with prebuilt gate
  library.
- `createInequalityAssign`: NOT gate creation failure after successful equality
  net construction.
- `createLogicalLeftShiftAssign`: missing / zero shift width after successful
  SV elaboration (defensive guard).
- `getConstantBit`: null expression after `stripConversions` on AST expression
  references (defensive guard).
- `getConstantInt64`: active loop-variable select evaluation guards for a
  missing evaluation symbol, invalid loop binding symbol/type, or failed local
  creation. Parser-backed integral loop selects supply value symbols with
  positive-width types, and Slang's `createLocal` returns the stored value's
  address.
- `getConstNet`: invalid net-type and four-state-value throws. Internal callers
  only pass an assign-constant type, and `slang::logic_t` only represents 0, 1,
  X, and Z.
- `tryCollapseAnonymousBusAssignAlias`: invalid component / missing instance
  guard after design terminals have been ruled out. Remaining components are
  instance terminals, which always have an owning instance.
- `connectInstanceTermBits`: scalar-terminal fallback. Current callers use
  canonical DB0 bus terminals even when their width is one.
- Case-inside value-range bound evaluation after `getConstant()` fails. Current
  Slang ASTs expose folded integer constants for legal bounds; the fallback is
  retained for alternate or future AST spellings.
- Generic unknown-integer bit expansion after the dedicated literal,
  structured-pattern, and constant-expression resolvers. Parser-backed unknown
  constants are consumed by those earlier paths; the generic loop is retained
  for alternate or future AST spellings.
- `resolveExpressionBits`: defensive call-cast guards for malformed argument
  list and zero/non-integral width (`$signed` / `$unsigned`).
- `resolveExpressionBits`: defensive unknown-bit recheck after
  `intValue.hasUnknown()`.
- `resolveExpressionBits`: internal primitive creation failure for per-bit
  binary gate construction.
- `resolveExpressionBits`: defensive concatenation operand width guard for
  invalid/non-integral operands.
- `resolveExpressionBits`: member-access fast-path fallthrough brace after
  failed recursive value resolution, prior to generic selectable fallback.
- `resolveExpressionBits`: late dynamic indexed-range multiply-selector
  recovery after direct selector bit resolution fails. Current parser-backed
  tests either resolve these selectors before the recovery path or fail before
  the multiply-specific fallback.
- `resolveExpressionBits`: brace-only LCOV artifacts in call / binary /
  element-select / member-access blocks.
- Active for-loop constant helpers: whitespace-only or missing source excerpts
  and parameter-initializer recursion retained for alternate AST shapes, while
  current Slang elaboration provides source ranges and folded parameter values.
- Element-selection width helpers: non-bitstream range fallbacks and missing
  fixed-range diagnostics retained for alternate type modeling; parser-backed
  dynamic element assignments reach these helpers with fixed-range bitstream
  bases.
- Inferred-memory write-target extraction: missing bitstream width after a
  supported memory signature, and dynamic-target expansion reaching the memory
  root without first collecting a dynamic selector, are defensive fallbacks.
- Direct inferred-memory matching's ignorable-statement return is unreachable
  after candidate collection: a direct candidate exists only when that same
  statement tree contains an indexed memory assignment.
- Inferred-memory selector guards: expansion always supplies an expression and
  selector index; after their width and resolved bits are validated, failure to
  build the equality guard requires an internal primitive-construction failure.
- Declaration-initializer DFF INIT finalization: skip after the first
  unconsumed bit of a partially consumed variable is an unordered-map iteration
  artifact, not a distinct parser-backed behavior.
- Dynamic element sub-assignment lowering: internal offset and width mismatch
  guards retained for inconsistent selection-step data after prior fixed-range
  and bit-width validation.
- `getDriverFailureDetails`: parse-diagnostics aggregation from
  `driver.syntaxTrees` on driver failure. With current slang behavior, tested
  driver failure modes report via `sourceLoader` / argument parsing and keep
  `syntaxTrees` empty, so parse-diagnostics formatting lines are treated as
  near-unreachable.
- `dumpDiagnosticsReport`: filesystem I/O failure branches (directory creation,
  report file creation, report file final write-check) are defensive and
  environment-dependent, so they remain excluded.
- Output-function `inout` / `ref` setup: the null-LHS arm after conversion is
  unreachable because output-actual collection already accepted that same call
  argument only after it converted to an LHS.
- DPI output abstraction: non-bitstream actuals are rejected before this
  lowering path, while Slang normalizes legal width conversions before the
  output bits are applied. The corresponding width and application-failure
  guards are retained as defensive checks.
- Sequential event-list condition extraction always receives a parser-backed
  process statement; its null and failed-unwrapping guards are defensive.
  Slang also represents a one-event sensitivity list as `SignalEvent`, leaving
  the one-element `EventList` arm as an alternate-AST fallback.
- Priority `always_latch` mux / OR construction and controlled-negedge clock
  inversion use validated nets and canonical DB0 primitives. Their null-result
  diagnostics require an internal primitive-construction failure.

- Streaming LHS bit resolution: assignment collection has already rejected
  dynamic streams and invalid leaves. Fixed widths must equal the sum of operand
  widths; successfully resolved fixed leaves must have materialized roots, and
  replay entries keep their root storage width. These invariant checks remain
  defensive. Recursive operand-resolution failures are **tested**, not excluded.
- Loop-bound and step unsigned fallbacks: `getConstantInt64` already accepts
  unsigned constants fitting `int64_t`. Success after that helper failed is
  defensive; the unsigned-overflow rejection is tested with a typed parameter.
- Independent latch mux construction failures require failure of canonical DB0
  primitives with validated one-bit inputs. Constant guard shortcuts are tested.
- Compound assignment replay always supplies current bits at the target width;
  Slang represents compound shifts with the matching binary operation. Missing
  current bits and malformed compound-shift AST guards are excluded, while an
  unresolvable shift count is tested.


## Requested line coverage (October 2026)

The line numbers below refer to the source before this coverage change.

| Original lines | Coverage or exclusion |
| --- | --- |
| 4714–4715 | Excluded: both unpacked-constant callers validate the fixed bitstream width; legal constant arrays have integral leaves whose total width matches that type. |
| 5369 | `independentWriteClockMemoryBehavior`, ascending packed-word variant. |
| 6650–6651, 6657 | `memoryWriterCensusRejectsUnresolvedLoopsCoverage`: runtime stop bound and runtime step. |
| 6670, 6674–6676 | `memoryWriterCensusRejectsDynamicPackedOverlapCoverage`: indexed ranges, packed elements, and packed struct members conservatively own their containing element. |
| 7855–7859, 7895, 9025–9026, 9042–9043, 9098–9099 | `independentMemoryResolutionFailuresCoverage`: unsupported runtime system calls in addresses, a secondary write clock, and write data must report the specific lowering failure and throw. |
| 7899, 7901, 8035–8038 | `independentWriteClockMemoryBehavior`, wide and signed address variants, including invalid-address writes that must not alias valid words. |
| 8060 | Excluded: a parser-backed range-select requires a fixed-range packed base; the width-only alternative is defensive. |
| 9046–9047 | `independentWriteClockMemoryBehavior`, negative-clock variant. |
| 9152–9157 | `independentWriteClockMemoryBehavior`, priority and self-read variants, comparing the dumped netlist against RTL simulation. |
| 12641 | Existing config range-check function tests exercise zero bits in nonzero base/limit constants. |
| 12893 | Excluded: Slang creates an implicit return variable for every non-void function; a void call cannot supply an integral RHS. |
| 13079 | Excluded: the preceding successful `resolveExpressionNet` requires a value-symbol reference, which necessarily has a root symbol. |
| 13602, 13604, 13609, 13621 | `parsePriorityReturnFunctionNestedStatementsCoverage`: nested assignments, duplicate loops, declaration-only blocks, and a loopless unsupported return expression. |

Additional requested lines refer to the source before the following change:

| Original lines | Coverage or exclusion |
| --- | --- |
| 727 | Excluded: the unpacked-struct width overflow safeguard requires over `UINT32_MAX` bits, beyond practical test resources. |
| 4729–4734, 11121–11123 | `parseUnpackedStructValuesCoverage`: nested unpacked struct constants and runtime positional assignment patterns, with every flattened bit checked for its value or source connection. |
| 13430 | `rejectReturnOnlyFunctionExpressionCoverage`: a return-only statement list containing an unsupported runtime system call. |
| 34789–34791 | `rejectContinuousStreamingRHSCoverage`: an unsupported runtime system call on the RHS must emit the streaming-assignment diagnostic and throw. |

Procedural concatenation requests refer to the source before this change
(`280820 to 28022` is interpreted as `28020–28022`):

| Original lines | Coverage or exclusion |
| --- | --- |
| 25056, 28007 | Excluded: concatenation flattening can fail only for a null Slang operand, which a parser-backed concatenation cannot contain. |
| 28020–28022 | `SNLSVProceduralConcatTest.matchesPerElementRewrite`, `nba_replay` variant: a blocking temporary activates replay while nonblocking concatenation reads must retain the pre-clock target values; both compilation-unit modes are checked against simulation. |
| 28035–28036 | Excluded: Slang requires integral, fixed-width leaves in a procedural concatenation LHS. |
| 28052 | `proceduralConcatResolutionFailuresCoverage`, `rhs` variant: an unsupported runtime system call must report the RHS resolution failure and throw. |
| 28060–28061 | Excluded: leaf widths were validated before RHS lowering, which returns their total width; subsequent slices must fit. |
| 28077, 28094 | `proceduralConcatResolutionFailuresCoverage`: unsupported dynamic selectors fail both dependency replay and tracked-target application, with the selector diagnostic checked. |
| 28097–28098 | Excluded: a leaf accepted by `isTrackedSelectionSubLhsOf` is handled by the element/range handlers or by the fixed-subassignment handler, which sets `handled` for every remaining selection rooted at the tracked LHS. |

Inferred-memory local equality requests (source line numbers before this change):

| Original lines | Coverage or exclusion |
| --- | --- |
| 8659 | `parseQDNextLocalCommitEqualityOperandFailureFallback`, `lhs` variant: a reference to a different memory entry cannot resolve as the current entry's local slice; lowering must diagnose and throw. The `rhs` variant preserves the existing opposite-operand regression. |
| 8667 | `wildcardLocalConditionsKeepMemoryInference`, signed `==?` and `!=?` variants: signed memory words and signed wildcard patterns exercise both operand type checks, retain one inferred memory, and match RTL simulation. |


Procedural assignment-offset requests (source line numbers before this change):

| Original lines | Coverage or exclusion |
| --- | --- |
| 29619 | Excluded: the only caller first verifies that the assigned expression belongs to the tracked storage root. Dynamic selected targets have a nonempty element/member/range chain back to that root. |
| 29632 | Excluded: accepted fixed-range integral arrays have positive element widths dividing the parent storage width; preceding static selections retain whole typed elements or fail before reaching this check. |
| 29642 | Excluded: Slang rejects nonconstant simple-range bounds and indexed ranges with nonconstant or nonpositive widths before constructor lowering. |
| 29647 | Excluded: Slang rejects any chained selection after a range select, so a range selection is necessarily the final step. Parser-backed probes checked dynamic element, constant element, and range suffixes. |

Nested packed assignment requests (source line numbers before this change):

| Original lines | Coverage or exclusion |
| --- | --- |
| 27051 | `nestedPackedWriteFailuresCoverage`, `range` variant: a range following two packed element selections retains the unsupported dynamic sequential assignment diagnostic. |
| 27073–27074 | `nestedPackedWriteFailuresCoverage`, outer and inner index variants: unsupported runtime system calls must report the nested packed index resolution failure and throw. |
| 27083–27085, 27089–27090, 27103–27106 | `nestedPackedMemberWritesPreserveOtherFields`: packed struct fields before, between, and after dynamic element selections. Four-state simulation checks untouched fields, invalid and unknown indices, reset, enable, and NBA write priority against an independent flat reference. |
| 27086–27087 | Excluded: range steps have already returned; the remaining static steps are packed struct fields whose positive widths and in-bounds offsets Slang validates against their containing packed type. |
