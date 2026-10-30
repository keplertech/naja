Loading and Saving
==================

The top-level load and dump functions live in :mod:`najaeda.netlist`.

Session lifecycle
-----------------

.. code-block:: python

   from najaeda import netlist

   netlist.reset()
   top = netlist.create_top("top")

Call :func:`najaeda.netlist.reset` at the beginning of scripts and tests when
you need an empty universe.  Loading a design returns the top
:class:`najaeda.netlist.Instance` wrapper.

Gate-level Verilog
------------------

``load_gate_level_verilog`` and ``GateLevelVerilogConfig`` refer to **gate-level (structural)
Verilog**: modules, cell instances, nets, ports, and structural connections.
This loader does not provide general behavioral Verilog RTL support, such as
procedural ``always`` blocks or RTL arithmetic expressions.

For Verilog or SystemVerilog RTL, use ``load_system_verilog`` with
``SystemVerilogConfig``, including for ``.v`` files. That frontend elaborates a
supported RTL subset through slang; it is not a full language implementation or
a simulator. Choose the loader by the source's contents, not its file extension.

The explicit gate-level names are preferred. Existing names remain compatibility
aliases and emit a visible ``FutureWarning`` saying they **will be deprecated
soon**, with the replacement to use. No removal version is scheduled yet.

.. list-table:: Gate-level Verilog API migration
   :header-rows: 1
   :widths: 45 55

   * - Compatibility name
     - Preferred name
   * - ``netlist.load_verilog``
     - ``netlist.load_gate_level_verilog``
   * - ``netlist.VerilogConfig``
     - ``netlist.GateLevelVerilogConfig``
   * - ``Instance.dump_verilog``
     - ``Instance.dump_gate_level_verilog``
   * - ``netlist.VerilogDumpConfig``
     - ``netlist.GateLevelVerilogDumpConfig``
   * - ``NLDB.loadVerilog``
     - ``NLDB.loadGateLevelVerilog``
   * - ``NLDB.dumpVerilog``
     - ``NLDB.dumpGateLevelVerilog``
   * - ``SNLDesign.dumpVerilog``
     - ``SNLDesign.dumpGateLevelVerilog``

Arguments, defaults, return values and netlist behavior are unchanged. Existing
configuration objects work with the new methods, and the compatibility methods
also accept the new configuration classes. New calls do not emit compatibility
warnings internally. The CLI uses ``-f gate-level-verilog`` for gate-level input and
``-t gate-level-verilog`` for structural output. Both options still accept
``verilog`` as a compatibility alias and warn that it will be deprecated soon.
An inferred output format does not produce a second warning.
``-f systemverilog`` (or ``-f sv``) selects RTL elaboration.

.. code-block:: python

   from najaeda import netlist

   config = netlist.GateLevelVerilogConfig()
   top = netlist.load_gate_level_verilog(["cells.v", "design.v"], config)

Load Liberty or
primitive libraries first when the design instantiates standard cells or
technology primitives.

Loader arguments are validated before native parsing starts.  A missing input
raises :class:`FileNotFoundError` with both the supplied and resolved path;
wrong argument and configuration types raise :class:`TypeError`; empty paths
or unsupported configuration values raise :class:`ValueError`.  Errors for a
list entry identify its zero-based index (for example, ``files[2]``).

Verilog syntax-error exceptions include the failing input file path alongside
the parser's line and column range, including when loading multiple files.
With preprocessing enabled, these positions refer to the preprocessed text.

Verilog dumps can opt into Verilator's packed-signal splitting with
``GateLevelVerilogDumpConfig(verilatorSplitPackedSignals=True)`` passed to
``Instance.dump_gate_level_verilog``. This adds ``/* verilator split_var */`` comments to
internal bus nets and child-module bus ports so Verilator can track dependencies
between separate packed sections. Public top-module ports are excluded. The
option defaults to ``False`` and adds no warning suppression or logic changes.
Genuine bit-level combinational loops still produce lint diagnostics.

Gate-netlist source references
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Structural Verilog loading records declarations for module headers, instance
names, explicit net declarations and ANSI/non-ANSI ports. An instance declaration
points at its instantiation; ``get_model_source_declaration()`` points at the
model header. References use provider ``naja-verilog`` and language ``verilog``;
representation stays unknown. Declaration spans describe the available grammar
production, rather than a full module or instance body. Implicit port nets share
the port declaration until an explicit net declaration is encountered. Generated
constant nets and unresolved model definitions have no textual declaration.
``get_source_range()`` continues to return the independent ``SourceLoc`` range.
It does not fall back to declarations or origins.

Select Yosys provenance explicitly when loading synthesized output::

   top = netlist.load_gate_level_verilog(
       "synthesized.v",
       netlist.GateLevelVerilogConfig(yosys_source_attributes=True))
   cell = top.get_child_instance("u")
   gate_location = cell.get_source_declaration()
   upstream_locations = cell.get_source_origins()

The option defaults to ``False``. A filename, banner or generic ``src`` attribute
never implicitly selects Yosys. With the option enabled, string ``src`` values
are decoded as ordered ``|``-separated upstream references with provider
``yosys``; language and representation remain unknown. Exact duplicates collapse
in first-occurrence order. Raw attributes remain unchanged by normalization.
The last colon starts the numeric suffix, preserving Windows drive letters and
other colons in paths. Accepted suffixes are ``line[.column][-end_line[.end_column]]``.
Missing coordinates and explicit zero remain unknown (zero). Coordinates above
``4294967295``, malformed or empty entries, non-string values, and known reversed
ranges produce warnings and are skipped; valid neighboring entries survive.
Columns use one-based inclusive byte offsets in the shared source API.

Yosys origin paths decode Verilog string escapes (backslash, quote, newline,
tab and up to three octal digits); the raw attribute spelling stays unchanged.
Unsupported escapes, NUL and out-of-byte-range octal escapes are diagnosed and
skipped per entry. Windows paths should use doubled backslashes in the Verilog
string literal. The parser scanner accepts escaped quotes in attribute strings.


Neither retained source text nor an AST is required. NajaIF preserves typed
references. Verilog export retains raw attributes but does not preserve the
typed source references. NajaIF does not currently persist general raw Verilog
attributes. Preprocessing lacks a
source map: automatic declarations are omitted when ``preprocess_enabled=True``;
explicit upstream Yosys references still work. Locating relocated
source files and linking origins to separately loaded RTL/ASTs are future work.

Verilog and SystemVerilog RTL
-----------------------------

.. code-block:: python

   from najaeda import netlist

   config = netlist.SystemVerilogConfig()
   top = netlist.load_system_verilog(["pkg.sv", "rtl.sv"], config)

``load_system_verilog`` accepts both Verilog (``.v``) and SystemVerilog (``.sv``)
RTL within the supported subset. It elaborates the design through slang and
then builds SNL objects. Frontend diagnostics are raised as native
``SystemVerilog*`` exceptions from :mod:`najaeda.naja`.

Interface ports with modports are flattened into scalar or packed member ports.
Net and variable members retain their identity across separate interface
instances, interface arrays and generated scopes. Forwarding a modport through
a module connects the flattened ports to that module's internal logic; connected
interface parameters determine member widths and packed ranges. Explicit modport
connection expressions, such as ``input .value(payload[2:0])``, are currently
rejected with an unsupported-construct diagnostic.

In a fresh universe, loading the same ordered sources and configuration with the
same Naja build reproduces instance and net names, IDs, and creation order,
including generated function-call temporaries. Verilog dumps match after
excluding their timestamp comment headers. NajaIF snapshots, including their
producer manifest, contain no timestamp and match byte for byte.

Blocking read-modify-write assignments to fixed unpacked array elements (including
nested arrays, packed struct fields and partial elements) use the value from
before the statement, including when the index is variable. Combinational cycles
in the logic emitted for a single ``always_comb`` or ``always @*`` block are
rejected with a ``combinational cycle inside procedural block`` diagnostic.

Sequential dynamic writes to packed struct fields drive only the selected
field's bits; independent processes may drive disjoint fields. This also applies
to fields inside dynamically selected packed array elements. Dynamic sequential
selections without a supported lowering, including unsupported nested memory
selectors, raise an unsupported-construct diagnostic rather than dropping the write.

Backend-specific settings use the frontend-neutral ``frontend_options`` field.
It maps a frontend name to its ordered option tokens. The active SystemVerilog
frontend is currently ``"slang"``; other names raise an error, even with an empty
list. This namespace allows future frontends to define their own options without
interpreting Slang flags or silently ignoring them. ``None`` (the default), an
empty dictionary, or ``{"slang": []}`` adds no frontend options.

.. code-block:: python

   config = netlist.SystemVerilogConfig(
       frontend_options={"slang": ["--relax-enum-conversions"]})
   top = netlist.load_system_verilog(["source.sv"], config)

The Slang adapter passes each list entry as one driver argument, without shell
parsing. Use separate entries for an option and its value; do not shell-quote
paths containing spaces. Tokens must be nonempty strings without NUL bytes.
They are inserted after typed configuration options and before sources/filelists.
Slang handles repeated options and diagnoses invalid or unknown options. These
settings apply to direct inputs and ``flist`` sources alike. They change frontend
acceptance, without adding Naja lowering support for unsupported constructs.
Common loader settings such as ``top``, ``flist``, ``defines``, warning suppression,
blackboxing and diagnostic routing retain their typed fields.

Strict SystemVerilog checks remain the default. For Slang, the following options
are useful when loading sources written for more permissive tools:

* ``--relax-enum-conversions`` permits implicit integral-to-enum conversions,
  including unnamed values, using the enum base representation.
* ``--allow-use-before-declare`` permits variable references before declarations.
* ``--timescale=1ns/1ps`` supplies a fallback time unit and precision.
* ``--single-unit`` shares preprocessor macros across files in input order.

Each file is a separate compilation unit unless ``--single-unit`` is enabled.
Place macro definitions before their uses. This works with top selection and
filelists:

.. code-block:: python

   config = netlist.SystemVerilogConfig(
       frontend_options={"slang": ["--single-unit"]})
   top = netlist.load_system_verilog(["defs.sv", "use.sv"], config)

   config = netlist.SystemVerilogConfig(
       frontend_options={"slang": ["--single-unit"]}, flist="sources.f")
   top = netlist.load_system_verilog([], config)

Direct nonblocking indexed writes from multiple clocked processes now infer one
shared ``naja_mem`` primitive, including different clocks and true dual-port RAMs
with a shared clock. Each write action carries its own clock, enable, address,
data, and bit mask. Constant packed bit/part selections and statically unrolled
masked-write loops are supported. Positive and negative clock edges are
supported; negative edges use an inverted write-clock signal. No raw storage
array or flops with multiple clocks are created.

Memory inference also applies to constant-index writes and statically unrolled
loops. Reading an inferred array as a whole, including through an output port,
continuous assignment, aggregate expression, or submodule input, materializes
constant-address asynchronous read ports for its elements. These ports share
existing constant-index read ports. Element-only consumers keep the compact
memory representation; whole-array consumers require a flattened view of all
stored bits. Whole-array writes without indexed writes and fully packed arrays
continue to use ordinary registers.

Read ports are asynchronous; registered reads and read enables remain ordinary
flops around the memory. Nonblocking reads on a write edge observe the old data.
Multiple writes within one process preserve source-order priority per bit;
untouched bits are masked, rather than written back from an earlier read.
Simultaneous writes from different processes to the same address and overlapping
bits are unspecified, as in the source RTL. No cross-process priority is added.
The ``multi_clock_memory_collision`` diagnostic states this limitation.

This inference supports fixed, one-dimensional unpacked memories whose lowest
index is zero, with at least one runtime write address. Initialization supports
the existing constant full-memory for-loop fill. Blocking updates within writer
processes, whole-array updates, and memory resets shared between independent
writers remain unsupported. Statically disjoint constant selections retain
normal generic lowering. Unsupported overlapping or dynamic writers are rejected
by default. To keep elaborating a larger design, explicitly opt into module
blackboxing for those unsupported cases:

.. code-block:: python

   config = netlist.SystemVerilogConfig(blackbox_multi_writer_memories=True)
   top = netlist.load_system_verilog("rtl.sv", config)

The entire elaborated module containing the unsupported memory becomes a
``UserBlackBox`` with its port names, widths, directions, and parent connections
preserved. All internal behavior is omitted, including logic unrelated to the
memory. The ``multi_writer_memory_blackbox`` warning identifies the memory and
module in the normal diagnostics report. Successfully inferred multi-port
memories retain their behavior even when this fallback option is enabled.

OpenTitan/Pavona's generic ``prim_ram_2p`` already guards its internal body with
``SYNTHESIS_MEMORY_BLACK_BOXING``. The existing define support is a workaround
that requires no fallback option:

.. code-block:: python

   config = netlist.SystemVerilogConfig(
       defines=["SYNTHESIS_MEMORY_BLACK_BOXING"])
   top = netlist.load_system_verilog(["prim_ram_2p_pkg.sv", "prim_ram_2p.sv"], config)

Supply the normal include paths/file list and dependencies for the source tree.
The macro removes the memory implementation before elaboration; the resulting
port-only module is detected as a blackbox. It affects every module honoring
that macro and leaves memory behavior unspecified.

Continuous assignments can call combinational functions whose result is selected
by ``case``, ``casez``, or ``casex``, including functions with escaped names
and concatenated arguments. Wildcard patterns use
the frontend's two-state matching behavior; overlapping items retain source-order
priority. A result must be defined for every selector value; functions with an
uncovered path are rejected.

Wildcard equality (``==?``) and inequality (``!=?``) ignore ``X``, ``Z``, and
``?`` bits in constant right-hand patterns after operand sizing and extension.
This includes parameter, concatenation, replication, and cast expressions in
assignments and conditions. Left-hand unknown bits remain comparison inputs.
Runtime four-state right-hand patterns retain the frontend's two-state
comparison limitation and emit ``case_comparison_two_state``.

Automatic combinational functions can return fixed-size unpacked arrays of
integral elements, including multiple dimensions and ascending or descending bounds.
Explicit returns and assignments to the function's implicit result preserve
declaration order and blocking-write semantics. Unwritten automatic locals
use SystemVerilog default values (including X for four-state logic and zero
for two-state bits). Calls with constant values may be evaluated by Slang after
the function signature, locals, expressions and
nested calls have been checked for representable types. This path preserves
four-state constant bits and retains Slang's constant-expression restrictions
and evaluation limits; it does not enable string/real arguments, output arguments,
or runtime side effects. Calls depending on runtime inputs use normal function
inlining instead. Static function/local storage in runtime array-return functions
remains unsupported. Block-scoped locals in these runtime functions require an
explicit declaration initializer so that repeated block entry reinitializes them.

Combinational function input arguments can include fixed-size unpacked arrays
of integral elements. Actuals may be local or package parameters, or unpacked
variables; earlier blocking writes in a combinational block are visible to the
call. Selected read/modify/write operations and implicit return-variable partial
writes retain source-order semantics.

Pure combinational table reads and dynamic powers of two share lowered results
when their ordered input bits, result widths and selection shapes match within
a design. Reassigning a procedural value before a later read selects a distinct
expression. Power-of-two lowering combines upper exponent bits into a shared
overflow condition; integral negative exponents and exponents outside the result
width produce zero. Result sizing and signed exponent types remain significant
when deciding whether results can be shared.

Statically bounded ``foreach`` loops are unrolled in combinational and sequential
blocks and function bodies. Packed and unpacked dimensions iterate in declared
order (``[3:0]`` visits 3, 2, 1, 0; ``[0:3]`` visits 0, 1, 2, 3), with nested
iteration for multiple indices and support for omitted indices. Named blocks
and compound assignments use the same lowering as in ``for`` loops. Dynamically
sized iterated dimensions are rejected; the unroll limit is 4096 body executions.

SystemVerilog addition, subtraction, variable multiplication and dynamic shifts
use compact canonical NLDB0 primitives. Arithmetic operands are sized and
sign-extended according to SystemVerilog before the primitive retains the low
result-width bits. Addition and subtraction compute modulo ``2**WIDTH``; their
operation is fixed by the canonical model identity. Dynamic shifts preserve
all count bits, including oversized counts, and signed arithmetic right-shift
fill behavior. These primitives retain combinational dependencies and survive
NajaIF snapshot round trips; structural Verilog exports include their models.
Ordinary dynamic packed bit and indexed slice writes can use the canonical
``naja_vector_update`` primitive. It preserves the full offset, leaves the
base value unchanged for unknown indices, and clips partially overlapping
writes. The primitive is language neutral and survives NajaIF save/load;
frontends normalize their own indexing and assignment rules before using it.
The VHDL loader also uses it for supported simple combinational element writes
whose binary index domain is provably in bounds. Nested targets and compound
updates retain their existing lowering. Formal consumers need explicit support
for the ``VECTOR_UPDATE`` semantic model; dependency arcs alone are insufficient.

Dynamic indexed slice reads can use ``naja_vector_select``. Its output reads a
bit window from the full signed or unsigned offset; each out-of-range bit is X,
and an unknown offset produces an all-X result. X/Z data is preserved. Like
vector updates, the primitive is frontend independent and survives NajaIF
save/load. The VHDL loader uses the shared family for nonconstant vector bit
reads whose index domain is provably in bounds. Formal consumers need a
``VECTOR_SELECT`` encoder that handles invalid selections explicitly; its
binary evaluator returns no value for an out-of-range bit rather than inventing
a Boolean value. Scalar constant propagation folds fully known, valid reads;
other cases remain conservative.

VHDL arithmetic currently retains its scalar full-adder lowering; the compact
add/subtract lowering described here applies to SystemVerilog.

Combinational procedural lowering supports compound shifts (``<<=``, ``>>=``,
``<<<=``, ``>>>=``), preserving the full shift-count width and signed arithmetic
right-shift behavior. Blocking assignments earlier in the block are visible to
subsequent shifts.

Sequential multi-assignment lowering preserves the bits written by each process:
replaying a whole vector to resolve overlapping assignments does not create
flops for untouched bits. Nested writes to the same target retain source-order
priority and share one driver per bit. In the conditional multi-assignment path,
mixed binary asynchronous reset values use separate reset/set cells, including
active-low controls and negative-edge clocks, rather than a clocked data mux.

Functions whose bodies consist of returns, conditionals, and case statements
support nested early returns. The first executed return takes priority over
later statements, including inside an ``else`` branch.

Fixed unpacked structs with integral fields, including nested structs and fixed
arrays, are represented as flat SNL buses. Member reads, continuous member
assignments, whole-struct copies, and member conditions use the same storage.
Unpacked struct fields occupy successive slices starting with the first declared
field at the least significant end; packed structs retain SystemVerilog packed
bit ordering. Structs containing strings, real values, or dynamic arrays remain
unsupported. Streaming and bitstream casts of unpacked structs are also
rejected.

Fixed-size streaming targets in continuous assignments support both directions,
explicit slice sizes (including partial slices), and nested streams. A wider
source is consumed from its most significant end. Dynamic selections, target
``with`` clauses, and overlapping destination bits are rejected.

Fixed-size streaming assignment targets in ``always_comb`` support both stream
directions, explicit slice sizes (including partial slices), nested streams,
multiple targets, and fixed unpacked arrays. A wider source is consumed from its
most significant end. Dynamic target selections, target ``with`` clauses,
overlapping target bits, and sequential streaming assignments remain unsupported
and are rejected.

``always_latch`` also supports independent guarded writes in statement lists and
statically bounded, nested ``for`` loops. Targets may be fixed array elements or
static bit/part selections. Each written bit retains its value while disabled;
later writes take priority, and unconditional writes become combinational logic.
This path supports blocking or nonblocking assignments, but rejects mixing them
on one variable, reading a variable written in the same block, dynamic targets,
function calls, expression side effects, and timed assignments. Existing simple
latch patterns retain their separate support.

Input-only function calls capture actual argument bit values, including constant
packed values and fixed unpacked arrays. Constant bits remain visible inside the
function body, so bitwise AND with zero or one and XOR with zero fold during
elaboration, including accumulators initialized with ``'0``. Argument binding and
returned replay values do not require assign primitives. ``keep_assigns=True``
retains source-level connections; it does not require copies at function-call
boundaries. Set ``keep_assigns=False`` to merge remaining assign connections after
elaboration. Function-local variables written by earlier loops are replayed
before later loops read them, including partial writes and locally computed
indices. These dependencies must remain driven in the returned netlist.
Declaration initializers of automatic function locals execute on every call as
blocking assignments, including expressions using arguments or earlier locals,
packed vectors and structs, and declarations in named blocks. Later assignments
see and can overwrite the initialized value. Combinational automatic tasks with
output arguments use the same local replay behavior.

Functions ending in an explicit ``return``, or an ``if`` or ``case`` whose
paths all return, can return from earlier branches after blocking assignments. Each path returns its current
local values; statements after an early return execute only on paths that fall
through. Writes to packed-struct input members, including nested members,
update the function's local copy without changing the caller. Nested calls,
packed return layouts, and argument and return conversions retain their normal
SystemVerilog widths and signedness.

Automatic function arrays receive their type's default value when their
declaration executes, including repeated block entry. Declarations in separate
scopes retain separate storage even when their names match. Automatic integral
function results also start at their type's default value: unwritten four-state
bits remain X and unwritten two-state bits remain zero. Persistent static
function state and uncovered explicit-return paths remain unsupported.

Output arguments are copied back before subsequent statements read the caller's
locals, including nested calls. Input argument conversions use the formal
argument's width and signedness, including truncation and extension.
Output subroutines that write outside their local scope are rejected, including
calls whose explicit copy-out results are unused.

Connected bits of inlined locals and formal arguments which have no driver emit
an ``undriven_inlined_local`` elaboration warning with the declaration location.
This check runs after alias cleanup and mux-chain simplification. Connected
intermediate logic can also cause an undriven local to produce the warning.
The normal warning suppression and diagnostics-report settings apply.

By default, an incremental diagnostics report is written to
``naja_sv_diagnostics.log``. Set ``diagnostics_report_path=None`` in
:class:`najaeda.netlist.SystemVerilogConfig` to disable that file and retain
console diagnostics only.

Set ``SystemVerilogConfig(frontend_options={"slang": ["--timescale=1ns/1ps"]})``
to supply the default
time unit and precision for design elements that do not specify a time scale.
Explicit source ``timeunit`` / ``timeprecision`` declarations and `` `timescale``
directives retain their normal precedence over this fallback. Omitting this option leaves the frontend default unchanged. Slang validates the
``<unit>/<precision>`` value during loading and reports invalid values as
``SystemVerilogSyntaxError``. This setting does not add simulation or delay
support to Naja.

``SystemVerilogConfig`` validates path, boolean, define, and warning-suppression
fields when it is created and again when loading begins.  Entries in
``suppress_warnings`` are warning names such as ``"width-trunc"``; omit the
``-W`` or ``-Wno-`` command-line prefix.  To load only from a command file,
pass an empty file list and set ``flist``.

VHDL
----

.. code-block:: python

   from najaeda import netlist

   top = netlist.load_vhdl("design.vhd")

For a supported structural file containing multiple design units, select the
top entity explicitly:

.. code-block:: python

   top = netlist.load_vhdl("hierarchy.vhd", top="soc_top")

Python VHDL loading emits a :class:`RuntimeWarning` on each parser invocation
because the frontend is in Beta mode. Its supported language subset and behavior
may change between releases.

The RTL path can infer the top when exactly one entity is not instantiated by
another design unit, including instances inside nested generate statements.
If there are several roots or no root, pass ``top`` explicitly.

VHDL loading is experimental and implements a bounded, two-state RTL subset.
It supports integer and boolean generic defaults and explicit generic maps, constrained
arrays, static indexing and slices, nested ``for`` loops, ``for generate`` and
static ``if``/``elsif``/``else generate`` statements, component instances, vector registers, synchronous reset/enable,
asynchronous reset/set,
and immediate process variables. Generated instances can bind indexed or sliced
ports; integer constant tables can supply their generic values. Architecture
and package constants may use positional array aggregates, including
unconstrained array types with ``natural``, ``positive``, or ``integer`` indices.
Constrained arrays also support static index choices, ranges, grouped choices
(``|``), and a final ``others`` association, including nested arrays and integer
constant tables. Positional elements follow declaration order and may precede
``others``, but cannot be mixed with named index choices. Overlapping or out-of-range choices, missing elements, and
associations after ``others`` are rejected. Named array aggregates require
explicit bounds; unconstrained bound inference remains positional-only.

Boolean signals, ports, and arrays use one bit per element while retaining
boolean type checking. Boolean array constants support static indexing in
constants and generate conditions, and dynamic indexing through the existing
read-only memory representation. Nested and enumeration-indexed constants are
supported. Architecture-local scalar functions can capture integer-indexed
boolean arrays; static package-function array lookups remain unsupported.
Intermediate integer addition/subtraction retains the supported 32-bit width;
assignments to constrained hardware integers use their declared width.

Pure functions returning a scalar integer or boolean may also accept
one-dimensional integer-array constants or forward integer-array parameters.
Array type identity and element subtypes are checked. Unconstrained formals
retain the actual bounds; constrained formals map elements by position into
their declared bounds and direction. Indexing, scalar bounds attributes, and
``for`` loops over the parameter's ``'range`` are evaluated statically.
Architecture-local functions capture preceding integer-array constants, including
for default arguments. Array actuals currently must be names; array-valued
returns, local array variables, aggregate/slice actuals, and enumeration-indexed
integer-array parameters remain unsupported.

Simple signal wiring assignments such as ``y <= a;`` use canonical DB0 Assign
instances, one per bit. ``Equipotential.Mode.TRAVERSE_ASSIGNS`` crosses these
connections, including slices and positional vector mappings. Constants remain
net driving types; logic gates, multiplexers, and registers remain traversal
boundaries.

Conditional generates select the first true branch at elaboration time, using
static boolean expressions built from integer/boolean generics, constants, enclosing
loop parameters, and supported static package functions. Only the selected
branch creates hardware. Generate bodies may contain assignments, instances,
clocked processes, and nested conditional or loop generates. Instance names
retain generate scope prefixes. Generate-local signals, constants, array types,
record types, subtypes, and pure subprograms are elaborated in declaration order. Each loop iteration has
independent local nets and register initialization; local names do not escape
their body. Eligible generate-local arrays use the same RAM inference as
architecture-local arrays; other arrays retain register/mux lowering. Shadowing an outer name,
local components, alternative labels, and explicit branch-body
``end`` statements remain unsupported and are diagnosed.

Record types declared in packages or architectures support nested records,
arrays of records, field selection, whole-record and field assignments, and
positional or named record aggregates. Named aggregates must supply every field
exactly once; a sole ``others`` association is also supported when its value
matches every field type. Record ports are flattened to one SNL bus in declaration
order, recursively, with the first field at the most significant end. Original
vector directions determine the order of bits within each field. Record type
names remain distinct for assignment checking. ``std_ulogic`` and
``std_ulogic_vector`` use the same lowering as their resolved counterparts;
multiple drivers remain unsupported. ``std_logic`` and ``std_ulogic`` literals
preserve ``X`` and ``Z`` as SNL constant nets, including vector strings,
binary/octal/hex bit strings, aggregates, constants, mux inputs, and register
data. Other nonbinary enumeration values and lowercase ``x``/``z`` are rejected.
``bit`` and ``bit_vector`` remain binary. Explicit ``X``/``Z`` operands in
arithmetic and comparisons are rejected; nine-valued simulation is unsupported.
Register initialization and asynchronous reset values must remain binary.

Array objects and constrained user-defined array types support ``'left``,
``'right``, ``'low``, ``'high``, ``'length``, and ``'ascending``. Bounds and
direction come from the declared subtype, including ascending and nonzero-based
ranges. Record fields, slices, and statically indexed nested arrays retain their
own bounds. Attribute queries do not read signal or variable values, so output
ports and uninitialized process variables can supply bounds without introducing
hardware reads or sensitivity-list dependencies.
``'range`` and ``'reverse_range`` may supply port, signal, and variable bounds,
process-loop ranges, generate ranges, and array slices. Constrained array subtype
prefixes retain their declared bounds, including inside runtime functions. The optional dimension argument must
be the static integer ``1``. Enumeration-indexed arrays support ``'length`` and
``'ascending``; enumeration-valued bounds and ranges remain unsupported.
Unconstrained type prefixes, runtime-indexed prefixes, and array attributes
inside static function bodies remain unsupported and are diagnosed.

Vector conversions between ``std_logic_vector``, ``std_ulogic_vector``,
``unsigned``, and ``signed`` preserve bit order and width, including ascending
source ranges. These conversions require visible ``std_logic_1164`` and
``numeric_std`` packages; they do not resize or sign-extend their operands.

Identifier-based enumerated types declared in packages, architectures, and
generate bodies support internal signals, variables, constants, record fields,
and arrays. Literals are encoded by declaration position using at least one bit;
explicit initializers are preserved. Type identity is retained for assignment and
equality/inequality and ordering checks. Cases may cover all declared literals without an
``others`` branch, including types whose literal count is not a power of two.
Enum ports and overloaded identifier literals are supported when their nominal
type is unambiguous from context. A literal may share a name with a function.
Enumeration ``'pos`` preserves ordinal values, and ``'val`` converts integer
ordinals back to the nominal enum type. Invalid static ordinals are errors;
invalid runtime ordinals produce X rather than wrapping onto valid literals.
Character literals, enumeration subtype constraints, other enum attributes, and
enum-valued static functions remain unsupported. Implicit enum initialization is not synthesized.

Pure package functions with static integer and boolean arguments can compute
constants, generic defaults, vector bounds, and constant expressions. The
supported bodies contain local scalar variables/constants, variable assignments,
``if``/``elsif``/``else``, bounded ``for`` and ``while`` loops, nested function calls, and early
``return`` statements. Unlabeled ``exit`` and ``exit when`` leave the innermost
function loop; labeled exits and process-loop exits remain unsupported. Positional and named actuals and default arguments are
supported. Defaults and function bodies resolve package declarations rather
than caller-local names. Integer arithmetic includes division, ``mod``, ``rem``,
exponentiation, and ``abs``; boolean comparisons and logical operators preserve
short-circuit evaluation. Subtype violations, overflow, missing returns,
ambiguous overloads, and excessive recursion or evaluation work are diagnosed.
Static ``boolean(...)`` conversions accept boolean operands, including comparisons
and nested conversions, in constants, generic defaults, and function bodies.
Short-circuited operands are type-checked without evaluating their values;
integer-to-boolean conversions are rejected.
Pure package and architecture-local functions can also lower binary scalar and
vector arguments and results to combinational hardware. Unconstrained vector
formals retain actual bounds; constrained formals bind by position. Calls support
positional/named arguments and declaration-scope defaults, nested pure calls,
local binary variables/constants, variable initializers, bounded ``for`` loops,
array attributes, ``if`` statements, and ``case`` alternatives. Early returns
may occur inside runtime branches and bounded loops, with remaining statements
providing a fallback return. Every reachable path must return a value. Function
scopes resolve package or captured declaration constants rather than caller-local names.
Integer and boolean function arguments may be static or runtime values. String
evaluation, impure functions, branch-dependent return bounds, ``exit`` statements,
local declarations conflicting with formal parameters, and shadowed loop
parameters remain unsupported and are diagnosed. Locals may hide imported or
captured outer names.
Missing returns, incompatible types/widths, function reads before definite assignment,
excessive recursion, and excessive elaboration work are errors.

Architecture-local pure functions also support static integer/boolean evaluation.
Bodies and default arguments use declarations visible at the function declaration,
including enclosing generics and scalar constants. Binary runtime functions also
capture visible binary constants. Overloading and architecture function forward
declarations remain unsupported.

Input port maps accept character, string, and binary/octal/hex bit-string
literals in positional or named associations, for direct entities and components.
The formal port supplies the type and bit order, including ``X``/``Z`` for logic
types. Input actuals also accept supported expressions, function calls, and aggregates
with the formal type as context. Output actuals must select writable signals.
Width/type mismatches and unsupported values are rejected.

Named port associations may select vector elements, slices, and record fields
on the formal side, for example ``data_i(0) => data_bit``. Individual associations
must be consecutive and cover every scalar subelement exactly once. Overlaps,
missing elements, invalid directions or bounds, and type/width mismatches are
errors. Formal index expressions currently support integer literals, locally
static integer constants, and predefined arithmetic; generics and generate
iterators cannot be used as formal indices. Actual selections may still use
elaboration-time generic or generate values. Component selections use the
component's bounds and bind to the entity by position. Whole output ports may be associated with ``open``, including scalar,
vector, and record ports in positional or named maps. Their instance terminals
remain unconnected, matching SV empty output connections, without a warning.
Whole output ports may also be omitted from named maps or left off the end of
positional maps, with the same unconnected result. Inputs without defaults must
be associated; explicit ``open`` inputs remain unsupported. Individual formal
elements or slices cannot be associated with ``open``, and partially associated
ports must still cover every element.
As with connected record ports, their type package currently needs to be visible
in the instantiating scope as well as the child entity.

Processes may repeat their opening label in ``end process label;``, including
combinational, clocked, and asynchronous-reset processes inside generates.
The closing label is optional; when present it must match the opening label.
Basic identifiers are case-insensitive, while extended identifiers retain case.

Arrays may use an enumeration as their index type, for example
``type requests_t is array(device_t) of request_t;``. Element order follows the
enumeration declaration, with nominal index-type checks. Enum literals and
constants select elements directly, including record elements in port maps.
Explicit enum ranges, descending slices, and unconstrained enum-indexed arrays
with explicit object bounds are supported. Runtime enum indices use mux reads
and decoded signal writes; clocked enum-indexed arrays retain register lowering
rather than RAM inference. Integer-indexed arrays reject enum indices, and
enum-indexed arrays reject integer or unrelated-enum indices. Enumeration-indexed
integer constant tables and enumeration subtype declarations remain unsupported.


Concurrent selected assignments (``with ... select``) support static choices,
grouped choices, and a final ``others`` alternative, including inside generates.
They share case-statement type, duplicate-choice, and coverage checks and lower
to combinational multiplexers. Delays and matching selections remain unsupported.

The netlist loader honors case-insensitive ``translate_off`` / ``translate_on``
comment directives prefixed by ``pragma``, ``synthesis``, or ``synopsys``.
Excluded simulation code contributes no hardware; source locations are preserved.
Nested, unmatched, and unterminated regions are rejected. Ordinary lexer/parser
calls retain simulation code unless their synthesis option is enabled. Unmarked
file declarations and other simulation operations remain unsupported.
In synthesis mode, assertion and report statements are parsed but ignored,
including their conditions, messages, and severity (even ``failure``).
Configuration assertions therefore do not validate generic values for now.
Each load emits one console warning per code (``ignored-assertion`` or
``ignored-report``). All occurrences, including source locations, are written to
``naja_vhdl_diagnostics.log``, overwritten per load. Internal re-parsing of
retained dependencies does not repeat their warnings. Unsupported hardware
constructs still fail loading. Choose a report path or select console-only output:

.. code-block:: python

   top = netlist.load_vhdl("design.vhd", diagnostics_report_path="logs/vhdl.log")
   # Alternatively: diagnostics_report_path=None for console-only warnings.


Additional VHDL RTL constructs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Subtypes retain their base type and static bounds in packages, architectures,
and generate scopes. Sized and signed/unsigned binary, octal, hexadecimal, and
decimal bit-string literals preserve VHDL padding and truncation rules. Integer
literals support based notation and nonnegative exponents; static ``minimum``
and ``maximum`` accept integer and boolean operands. Logic-vector reduction
operators preserve their scalar element type.

Entity interfaces, architecture signals, and subprogram locals can hide imported
names. Duplicate direct declarations remain errors. Pure procedures inline
variable ``in``, ``out``, and ``inout`` parameters,
including record fields. Writable actuals must be definitely assigned when read, and procedure outputs
must be assigned before returning;
overlapping output actuals and signal-class parameters are rejected.
Unconstrained integer process temporaries use signed 32-bit arithmetic; static
values assigned during loop unrolling may supply slice bounds. Numeric-vector
unary plus, minus, and ``abs`` and addition/subtraction with a scalar logic carry
are supported. ``shift_left`` and ``shift_right`` accept static natural counts
or runtime counts represented by nonnegative integer subtypes (including
``to_integer(unsigned_value)``). Runtime shifts use the canonical NLDB0 shift
primitive and retain every count bit; oversized counts produce zero for logical
shifts and the sign bit for signed right shifts. ``rotate_left`` and
``rotate_right`` currently require static natural counts.

Fixed-width vector slices may have runtime positions when their endpoint
expressions prove a constant width. They use canonical table-select primitives;
out-of-range reads produce X. Their results may be assigned by position or
returned from functions. Indexing those runtime-bound results or passing them to
unconstrained function formals remains unsupported; constrained formals fix the
bounds. Nested dynamic writes can select array dimensions and record fields in
signals or definitely assigned variables. Hardware integer multiplication and
``mod`` by a static positive power of two are supported.

``numeric_std.std_match`` supports a typed logic operand and a binary/don't-care
literal pattern in either order, scalar operands, and static logic values.
Untyped pairs of vector literals are ambiguous and rejected; unconstrained
``others`` comparison aggregates are also rejected. Concatenated aggregates
infer their own positional or explicit-choice bounds.

IEEE ``is_X`` uses synthesis semantics: a fully constant operand detects X/Z,
while a dynamic operand folds to false. No unknown-detection hardware primitive
is created. Synthesis attribute declarations/specifications for mapping hints
emit ``ignored-attribute`` warnings; attribute values do not affect the netlist.
Foreign subprogram attributes remain unsupported. Unused ``ieee.math_real.all``
imports are accepted; real constants and functions are not implemented.
Array dimensions remain bounded to 65,536 elements, with an aggregate limit
of 1,048,576 bits for arrays and records.

The NEORV32 default ``neorv32_top`` configuration remains a development target.
The complete core file list parses and resolves its named-library imports when
loaded into ``library="neorv32"``. Boolean/vector generics, array attributes,
and pure binary vector helpers now elaborate within the supported subset.
Additional ``numeric_std`` overloads and further constructs are still needed;
the complete top has not been validated.

Dynamic reads of constant arrays lower to read-only ``naja_mem`` instances
with ``WR_PORTS=0`` and ``INIT_ENABLE=1``. The instance ``INIT`` parameter
stores the contents, with address zero in the least-significant word. Binary
images use sized hexadecimal literals; images containing X/Z use sized binary
literals. These memories have only read address/data terminals and combinational
timing arcs; clocked read registers and enable muxes remain separate. Both HDL
frontends use the same representation. General signal or variable arrays use
``naja_table_select``, including signals whose constant drivers are encountered
after the read is lowered. Identical
lookups within a design share the primitive. Nonnegative ascending and descending
bounds are supported, including nonzero lower bounds. Indices can use
``conv_integer`` with the Synopsys packages or ``to_integer(unsigned(...))``
with ``numeric_std`` (up to 31 input bits). Address bits are preserved so
out-of-range or unknown addresses produce X in exported Verilog rather than
wrapping to a valid entry. Dynamic negative bounds and signed ``to_integer``
remain unsupported. Inferred writable memories retain their existing memory
primitive lowering.

The RTL path supports ``numeric_std.unsigned`` and ``numeric_std.signed`` addition, subtraction,
multiplication, equality/inequality, ordering comparisons and concatenation. Arithmetic
and comparisons accept integer operands on either side: static signed 32-bit
integers, or hardware integers with supported nonnegative ranges. Unsigned
operations require natural operands. Scalar arithmetic converts the integer to
the vector's width; addition/subtraction retain that width, while multiplication
produces twice that width. Comparisons preserve the integer's full value, even
when it lies outside the vector's representable range. Static
``to_unsigned(value, size)`` and ``to_signed(value, size)`` calls and conversions between ``unsigned``, ``signed``,
and ``std_logic_vector`` are supported. ``to_unsigned`` also accepts dynamic integers with a supported nonnegative
representation, including enum positions. Signed arithmetic extends the sign bit.
``std_logic_signed`` vector addition, subtraction, multiplication and
equality/inequality are also supported.
Addition and subtraction use canonical ``naja_fa`` full adders, as in the
SystemVerilog frontend. Multiplication forms partial products and accumulates
them with the same full adders; zero partial products and the first accumulation
need no adder. Supported ``std_logic_unsigned`` addition and subtraction use
full adders too, including sources that also import ``std_logic_arith``.
Static ``resize(vector, size)`` calls support unsigned zero extension and
truncation, and signed sign extension and sign-preserving truncation. Unsigned
resize and static selections of expression results propagate the required width
through arithmetic, conversions and nested expressions, so discarded high
arithmetic bits are not elaborated. Signed resize retains the original sign bit,
which may require the full arithmetic carry chain. Assignment alone does not
implicitly truncate numeric vectors: use ``resize`` or an explicit slice to
match the target length.
Loading preserves unused RTL signals and registers, and slicing an intermediate
signal does not propagate a smaller width back to its driver. Such designs may
still contain dead logic. For a pruned netlist, set the loaded design as the top
and run ``naja.NLUniverse.get().applyDLE()`` after loading; this also removes
unused RTL registers. Loading does not run DLE automatically.
VHDL signal wiring can retain explicit Assign
primitives; traverse Assigns when comparing arithmetic connectivity.
Explicit binary signal initializers on locally clocked registers are preserved
as DFF ``INIT`` parameters. A combinational initializer must either match the
startup value proved from constant logic and explicitly initialized local
registers, or affect only registered data/enable paths after combinational
settling. Register INIT values remain intact. Initializers reaching observable
outputs or asynchronous controls without such a proof are rejected.

Arrays with one dynamically indexed, whole-word clocked write
site are inferred as canonical RAM primitives. Conditional write enables are
preserved, and clocked read assignments retain a separate output register,
including its enable and read-before-write behavior on address collisions.
Resetting address registers does not reset the RAM contents. Both ascending and
descending arrays with nonnegative 32-bit bounds are supported. Static initial
contents are preserved in the RAM initialization parameters. Arrays with
multiple syntactic write sites, whole-word writes inside loops,
writes in asynchronous-reset processes, or direct connections to child-instance
ports retain the register/mux lowering.

Partial-word RAM writes use the existing canonical per-bit mask representation.
A single syntactic write site may be unrolled across static loops when every
write uses the same decoded address. Disabled lanes retain their contents;
later assignments take priority for overlapping bits. Different simultaneous
addresses in this profile are rejected. Output ports may be read using
VHDL-2008 signal semantics, including pre-edge register values.

Combinational processes accept ``process(all)`` or an explicit sensitivity list
covering every signal bit read. Explicit entries may name whole records or nested
record fields, such as ``process(packet.inner.data, packet.inner.flag)``. A parent
record covers its descendants; listing one field does not cover its siblings.
Indexed or sliced sensitivity entries remain unsupported. The same coverage check
applies to assignments outside a clock guard. Their bodies support ordered signal assignments,
immediate variables, static loops, and nested ``if``/``elsif``/``else`` and ``case`` branches.
Default assignments followed by partial conditional overrides preserve the last
assignment to each bit. Every driven bit must be assigned on every path; latch
inference is rejected. Variables must be assigned before use. Assignment proofs retain repeated ``if``
guards and ``case`` selections across intervening statements; unrelated guards
do not establish definite assignment. Reads of signals driven by the same process retain scheduled signal semantics.
They are accepted only when the local combinational dependency can be proved
acyclic; feedback cycles and unsupported dependency models are rejected.
Active ``wait`` statements remain unsupported.
Process constants and local shadowing are supported. Clocked process variables
retain values across activations, including individual record fields and vector
bits. Only incoming values used by the observable data path acquire DFF storage;
values overwritten before use remain immediate temporaries. Binary initializers
are preserved; implicit logic defaults become unknown hardware state, while
bit, Boolean, and enumeration defaults are zero. Retained integer values require
explicit initializers; retained Z initializers are currently rejected.
Scalar ``bit`` and ``std_ulogic`` conditions use VHDL-2008 condition conversion
in processes, runtime functions, and conditional expressions; explicit ``??``
is supported for these types. Binary inputs use their logic value, and static
X/Z conditions are false. Vectors require an explicit scalar test.
Combinational process variables must still be definitely assigned before use.
Processes used only for ignored assertions/reports and temporary calculations
create no hardware. Signal reads retain VHDL's scheduled
assignment semantics and are not replaced by earlier assignments in the body.

Sequential ``case`` statements support binary scalar/vector, boolean, and
nonnegative integer or enumerated selectors, grouped static choices (``|``), integer ranges,
and a final ``others`` alternative. ``null`` preserves the incoming assignment
state. Duplicate or overlapping choices and runtime choices are rejected.
Without ``others``, binary alternatives must cover the entire selector domain;
integer selectors require ``others`` in this profile. Choice expansion is bounded
to 4096 values. Cases in static package-function bodies remain unsupported.

Clock guards accept ``rising_edge(clk)`` or ``clk'event and clk = '1'``,
including parentheses around the guard or its operands, in both simple and
structured processes. An additional Boolean condition joined with ``and`` acts
as an enable and preserves the register value while false.

Asynchronous reset accepts a scalar equality to ``'0'`` or ``'1'`` before
the edge guard: ``if rst = '0' then ... elsif rising_edge(clk) then ...``.
Both reset and clock must appear in the sensitivity list. Reset values must
resolve to binary constants per bit; mixed reset-to-zero and reset-to-one values
are supported. Bits omitted from the reset branch hold their state while reset
is active, including at clock edges. Enables and explicit register initializers
are preserved. Dynamic asynchronous loads, multiple reset sources, and assignments
outside the clock/reset guard are rejected. General event-driven processes,
unsupported nonbinary values, multiple drivers, dynamic ``to_unsigned`` conversions and
general VHDL remain unsupported and are diagnosed. Argument and path errors
use the same Python exception categories as the other high-level loaders.
Frontend or lowering failures are reported as :class:`RuntimeError`.
RTL lowering errors include the original file, line, and column.

Both ``netlist.load_vhdl`` and ``najaeda.naja.NLDB.loadVHDL`` accept a single
path or a list of files in any order. All files are parsed before elaboration;
package declarations and entity/architecture dependencies resolve internally.
Omit ``top`` to infer the unique uninstantiated entity in the destination
library, or specify it explicitly when there are several roots. Missing units
and dependency cycles include the file, line, and column of the reference.

.. code-block:: python

   top = netlist.load_vhdl(
       ["firdec.vhd", "fir16.vhd", "types.vhd"], top="firdec_DSP")

Separate incremental calls on the same database remain supported. A single file with one entity that
requires generic values returns ``None`` when no ``top`` is specified; its
source is retained for later elaboration by its parent. An explicit ``top``
requires immediate elaboration and reports missing generic values. The raw API
also supports package-only files returning ``None``; the high-level loader
expects a completed top design.

Boolean generic defaults and actuals use typed static expressions, including
references to earlier generics and supported pure scalar functions. Positional
and named maps work for both direct entity instances and component bindings.
Boolean values remain distinct from integers: ``0``/``1`` cannot replace
``false``/``true``. Boolean specializations have separate cached models, and
conditions can select generate branches or drive supported boolean expressions.
Required boolean generics follow the same deferred dependency-loading convention
as required integer generics. Vector generic values are not yet supported.

VHDL source references
~~~~~~~~~~~~~~~~~~~~~~

VHDL loading records source declarations for entities, explicit instances, ports,
and signals, including bus bits. A model declaration identifies the entity;
its origins identify the architecture. Generated logic has source origins for
its expressions or processes, rather than textual declarations. Shared generated
logic can have several origins. Generic specializations and generate iterations
retain references to their original source syntax.

Use ``get_source_declaration()``, ``get_model_source_declaration()``, and
``get_source_origins()`` on high-level objects, or the equivalent expert methods
in :doc:`raw_api`. References carry ``provider="naja-vhdl"``, ``language="vhdl"``,
and ``representation="rtl"``. File coordinates remain local to each original
input across batch and incremental loads and logical libraries. Coordinates are
one-based, inclusive byte positions. Anonymous C++ string inputs use
``<source>`` as their filename.

The loader also populates the legacy ``get_source_range()`` location. Typed
references survive NajaIF snapshot round trips without retaining the AST or
requiring the source files to remain available. Live VHDL AST links are not
currently exposed.

HDL destination libraries
-------------------------

Both loaders accept a keyword-only ``library`` name, defaulting to ``"DESIGN"``:

.. code-block:: python

   top = netlist.load_vhdl("top.vhd", library="neorv32")
   top = netlist.load_system_verilog("top.sv", library="implementation")

The destination is a root ``NLLibrary`` in the current database. Basic names
match case-insensitively; extended names retain their case. Missing destinations
are created, while multiple matching roots are an error. Nested libraries and
other databases are not searched. This option chooses where SV designs are
stored; it does not add cross-library SV source binding.

For VHDL, named references such as ``cells.leaf`` and ``cells.types.all`` resolve
against these roots after a corresponding ``library cells;`` clause. ``work``
means the defining library of the current entity or package. Dependencies retain
their own library context, and identically named units in different libraries
remain distinct. ``std.standard`` and supported IEEE packages use built-in
providers; general standard-library source compilation is not supported.

Load package-only or generic-dependent files through the raw API first:

.. code-block:: python

   db.loadVHDL("types.vhd", library="cells")
   db.loadVHDL("leaf.vhd", library="cells")
   top_design = db.loadVHDL("top.vhd", top="top", library="DESIGN")

Liberty and primitive libraries
-------------------------------

.. code-block:: python

   from najaeda import netlist

   netlist.load_liberty(["NangateOpenCellLibrary_typical.lib"])
   netlist.load_primitives("xilinx")

The primitives library is available through
:func:`najaeda.netlist.get_primitives_library` for scripts that need to inspect
or create primitive-backed models.

Built-in Python primitive libraries attach combinational/sequential timing
arcs and characterize clock, data, enable, reset/set, and memory terms.  When
authoring a custom primitive loader, use the raw timing-model API described in
:ref:`Primitive timing modeling <primitive-timing-modeling>`.

Naja interchange
----------------

.. code-block:: python

   from najaeda import netlist

   top = netlist.load_naja_if("design.naja")
   netlist.dump_naja_if("roundtrip.naja")

Naja interchange preserves the SNL database more directly than Verilog and is
useful for round-tripping internal analyses and transformations. Schema 0.3.0
preserves attributes on designs, scalar/bus ports and nets, individual bus bits,
instances, and instance terminals, including attribute order, duplicate names,
and string/number value types. This also preserves the attributes returned by
``get_attributes()`` after a dump/load round trip.

Snapshots written with older schema revisions must be regenerated from the
original input. Older dumps omitted attributes; upgrading cannot recover them.
The loader checks the schema revision and producer identity before reading the
payloads.

Verilog output
--------------

Use native dump methods when you need Verilog output from the underlying SNL
database.  The high-level package currently exposes Naja interchange directly;
for raw Verilog dump control, see :doc:`raw_api`.

Canonical primitive snapshot round trips
----------------------------------------

NajaIF snapshots include ``db0_interface.snl``, encoded with the existing
NajaIF database-interface schema. It preserves the canonical NLDB0 model
interfaces and parameter defaults independently of explicit instance parameters.
Loading recreates those models through NLDB0 before restoring instances, so
inherited memory reset settings, shift dimensions and table-select dimensions
survive a fresh-universe reload. Initialization data and other explicit instance
parameters remain attached to their instances; shared models remain shared.

The payload includes unused canonical models already created in DB0, because
compound-model IDs follow creation order. This retains original IDs even when
instances first reference models in a different order. The loader validates
recreated interfaces and rejects conflicting identities in an existing universe;
use ``netlist.reset()`` before loading an independent snapshot. It also rejects
missing definitions or incomplete signatures instead of guessing defaults.
Regenerate snapshots that do not contain the DB0 payload.

C++ callers using the separate interface/implementation operations must dump
DB0's interface as well as the user database and load DB0's interface first.
Canonical primitive terminals, timing arcs, roles and sequential/memory modeling
are reconstructed by Naja's existing primitive constructors.


VHDL RAM initialization from text files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

VHDL signal and constant initializers can call supported impure functions that
read local text files during elaboration. Supported operations are read-only
``text`` file declarations inside functions, local ``line`` variables,
``endfile``, and two-positional-argument ``readline``, ``read`` and ``hread`` calls
for logic vectors. String arguments, static loops, conditionals and local array
updates are evaluated before constructing the memory. Runtime file I/O and
unsupported TextIO overloads remain errors.

Relative filenames resolve against the process working directory. File errors
and malformed data are diagnosed; initialization is never silently discarded.
The initializer function controls zero-fill, address order and EOF handling.
Arrays accepted by memory inference keep their contents in the writable memory's
initialization parameters instead of becoming register banks. Their data is
embedded in NajaIF snapshots, so reloading a snapshot does not require the file.
If an initialization file changes after its design has been cached in a library,
load into a fresh library rather than reusing the cached design. Existing array
size and elaboration-step limits continue to apply.
