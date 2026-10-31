Expert Raw API
==============

Design parameters returned by ``SNLDesign.getParameters()`` and overrides returned
by ``SNLInstance.getInstParameters()`` iterate in lexical parameter-name order.
This order is independent of name interning and is preserved across fresh-process
NajaIF reloads.

``najaeda`` ships two Python API levels:

* :mod:`najaeda.netlist` is the supported high-level API for most tools.
* :mod:`najaeda.naja` is the raw compiled extension module, historically also
  importable as top-level ``naja`` in some build layouts.

The raw layer is useful, but it is intentionally not the first documentation
entry point.  It exposes native SNL objects directly and assumes that callers
understand database ownership, object lifetimes, uniquification, and the SNL
hierarchy model.

Raw methods and module functions use camelCase, for example
:meth:`isClock() <najaeda.naja.SNLBitTerm.isClock>`,
:meth:`getNodes() <najaeda.naja.LogicCone.getNodes>`,
:meth:`createBinary() <najaeda.naja.SNLParameter.createBinary>`,
:meth:`fromString() <najaeda.naja.NLID.fromString>`, and
:func:`snapshotManifest() <najaeda.naja.snapshotManifest>`.
The source-intent helpers follow the same convention:
:func:`liveCompilation() <najaeda.naja.liveCompilation>`,
:func:`astSymbolOf() <najaeda.naja.astSymbolOf>`,
:func:`snlObjectsOf() <najaeda.naja.snlObjectsOf>`,
:func:`intentAvailable() <najaeda.naja.intentAvailable>`,
:func:`intentTypeOf() <najaeda.naja.intentTypeOf>`,
:func:`intentParametersOf() <najaeda.naja.intentParametersOf>`, and
:func:`intentPackageMember() <najaeda.naja.intentPackageMember>`.
Callers of the former snake_case spellings must switch to these names; those spellings are no longer exported. High-level
:mod:`najaeda.netlist` methods continue to use snake_case, such as
``Term.is_clock()``. Dictionary keys returned by raw functions retain their
existing spellings.

When to use the raw API
-----------------------

Use :mod:`najaeda.naja` when you need to:

* construct SNL objects using native factory methods;
* call a native method that is not wrapped by :mod:`najaeda.netlist`;
* inspect exact SNL IDs, paths, occurrences, libraries, or databases;
* use live SystemVerilog frontend intent helpers;
* debug the high-level API or compare wrapper behavior against native state.

Prefer :mod:`najaeda.netlist` for application code that edits hierarchical
designs.  If you edit raw objects directly, automatic high-level
uniquification does not protect you.

Importing the raw module
------------------------

Use the package export in user code:

.. code-block:: python

   from najaeda import naja

   print(naja.getVersion())
   universe = naja.NLUniverse.get()

Some development and test layouts also place the extension on ``PYTHONPATH``
as top-level ``naja``.  Prefer ``from najaeda import naja`` in documentation
and reusable scripts because it works with bundled package layouts too.  The
package-local extension takes precedence over a top-level module with the same
name, so an application file such as ``naja.py`` cannot shadow it.

Importing the native module does not write a performance report by default.
Set ``NAJA_PERF=1`` to write ``naja_perf.log``, or set ``NAJA_PERF`` to a file
path to select another destination. An unset or empty value disables the
performance report, which is appropriate when embedding Naja in a host Python
process.

Verilog dumps and primitive models
----------------------------------

``SNLDesign.dumpGateLevelVerilog`` and ``NLDB.dumpGateLevelVerilog`` emit ``naja_primitives.v``
beside the structural output when Naja primitive models are needed. Include
that generated file when linting or simulating the dump. Use the models from
the same dump rather than a separately maintained primitive library.

For built-in Naja primitives, the dump preserves effective parameter values:
instance overrides take precedence over model defaults, and parameters are
emitted when those values differ from the generated module's defaults. This
applies to mux and sequential widths, flop initialization, memory dimensions
and controls, table-select dimensions, and division/remainder width and
signedness. Explicit overrides equal to a generated module default may be
omitted even when the specialized model has a different default. Ordinary
modules retain their own declared defaults.

Read-only memories use the canonical NLDB0 memory family with ``WR_PORTS=0``.
Their models expose only ``RADDR`` and ``RDATA``; ``isSequential()`` returns
false because clocked read registers remain separate. Instance ``INIT_ENABLE=1``
and sized ``INIT`` literals hold the contents, with address zero occupying the
least-significant word. Binary images use hexadecimal; X/Z images use binary.
The existing ``getInstParameter`` API exposes these values. The model is shared
by equal memory dimensions, while contents belong to each instance. NajaIF
snapshots and Verilog dumps preserve the initialization. Unknown or out-of-range
ROM read addresses produce X in the generated model. C++ ``MemoryInterface``
accepts read-only interfaces with no write ports; NLDB0 clock/reset/write-term
getters return null for these models.

The generated division/remainder model implements signed and unsigned
quotient and remainder operations separately. The high-level
:meth:`najaeda.netlist.Instance.dump_gate_level_verilog` uses the same dumper; raw API
calls are not required for this behavior.

Deprecated names
----------------

Some raw names were renamed to camelCase to match the rest of the API.  The old
names still work, but they emit a ``DeprecationWarning`` saying they will be
removed in the next release, and forward to the new name:

* ``naja.SNLParameter.create_string``, ``create_decimal``, ``create_binary``,
  ``create_boolean`` ->
  :meth:`createString <najaeda.naja.SNLParameter.createString>`,
  :meth:`createDecimal <najaeda.naja.SNLParameter.createDecimal>`,
  :meth:`createBinary <najaeda.naja.SNLParameter.createBinary>`,
  :meth:`createBoolean <najaeda.naja.SNLParameter.createBoolean>`
* ``naja.NLID.from_string`` -> :meth:`fromString <najaeda.naja.NLID.fromString>`
* ``naja.snapshot_manifest`` -> :func:`snapshotManifest <najaeda.naja.snapshotManifest>`
* ``naja.live_compilation`` -> :func:`liveCompilation <najaeda.naja.liveCompilation>`
* ``naja.ast_symbol_of`` -> :func:`astSymbolOf <najaeda.naja.astSymbolOf>`
* ``naja.snl_objects_of`` -> :func:`snlObjectsOf <najaeda.naja.snlObjectsOf>`
* ``naja.intent_available`` -> :func:`intentAvailable <najaeda.naja.intentAvailable>`
* ``naja.intent_type_of`` -> :func:`intentTypeOf <najaeda.naja.intentTypeOf>`
* ``naja.intent_parameters_of`` -> :func:`intentParametersOf <najaeda.naja.intentParametersOf>`
* ``naja.intent_package_member`` -> :func:`intentPackageMember <najaeda.naja.intentPackageMember>`

Memory interface inspection
---------------------------

Experts can inspect existing memory models through the raw API only:
``design.hasMemoryInterface()`` returns a boolean, and
``design.getMemoryInterface()`` and ``instance.getMemoryInterface()`` return
fresh dictionaries. These delegate to ``SNLDesignModeling``; they do not infer
memory behavior from terminal or design names. Use them for primitive modeling
and downstream tools that need the native memory interface. There is no
high-level ``najaeda.netlist`` wrapper.

The dictionary has the following fields (keys follow the C++ member names):

* ``width``, ``depth``, ``abits``: Python integers.
* ``resetMode``: one of ``"none"``, ``"async_low"``, ``"async_high"``,
  ``"sync_low"``, or ``"sync_high"``.
* ``clock``, ``reset``: raw ``SNLBitTerm`` objects, or ``None`` when absent.
* ``readPorts``: an ordered list of dictionaries with ``address``, ``data``,
  and ``enables``, each an ordered list of raw ``SNLBitTerm`` objects.
* ``writePorts``: an ordered list of dictionaries with ``address``, ``data``,
  ``mask``, and ``enables`` (ordered bit-term lists), ``extraWriteInputs``
  (an ordered list of ordered bit-term lists), and ``clock`` (a raw bit term
  or ``None``). A port clock of ``None`` inherits the shared ``clock``.

Empty collections remain empty lists. All port and bit ordering is preserved
exactly from C++; there is no sorting or conversion to names or IDs. In
particular, Liberty bus order and canonical DB0 memory bus order can differ.
Read-only memories have an empty ``writePorts`` list and may have no clock or
reset. The interface describes terminals and dimensions, not stored contents.

The instance getter uses the C++ **instance overload**: it resolves canonical
DB0 memory parameter overrides for dimensions and reset mode, clears an
unconnected shared clock/reset, and omits ports whose required terminals are
not connected. Consequently its interface may differ from the model's.
The returned terminals still belong to the **model design**, including for the
instance getter; they are not ``SNLInstTerm`` objects or parent-design nets.
Parameter overrides do not resize or replace these model terminals. To obtain
an instance connection, use ``instance.getInstTerm(term)``. Wrappers retain the
usual raw-object lifetime rules; dictionaries do not take ownership of designs
or terminals. Editing the returned lists/dictionaries does not alter C++ metadata.

For a non-memory design ``hasMemoryInterface()`` returns ``False``. Both getters
raise ``RuntimeError`` if the design/model has no memory interface; they do not
return ``None`` or an empty interface. Native modeling errors (including invalid
instance parameters) are translated to Python exceptions using the standard raw
binding exception handling.

.. code-block:: python

   if instance.getModel().hasMemoryInterface():
       memory = instance.getMemoryInterface()
       for port in memory["readPorts"]:
           connections = [instance.getInstTerm(bit) for bit in port["data"]]

Object identity and hashing
---------------------------

Raw SNL objects returned by repeated accessor calls are not guaranteed to be
the same Python wrapper object.  Do not use ``id(obj)`` to identify nets,
terms, instances, paths, occurrences, or other wrapped SNL objects across
calls.  Use ``==`` for equality and use the wrapped object itself as a
``dict`` key or ``set`` element when the object is hashable.  Objects with a
native ``NLID`` hash from that ``NLID``.  ``SNLPath`` and ``SNLOccurrence``
are hashable by composing the ``NLID`` values of their referenced objects.
``SNLAttribute`` is hashable from its name and typed value.  Other raw value
wrappers without an ``NLID`` are intentionally unhashable.

Relationship to high-level wrappers
-----------------------------------

High-level wrappers store or recover native objects internally:

.. list-table::
   :header-rows: 1
   :widths: 28 36 36

   * - High-level object
     - Typical raw object
     - Notes
   * - :class:`najaeda.netlist.Instance`
     - :class:`najaeda.naja.SNLInstance` plus :class:`najaeda.naja.SNLPath`
     - Carries occurrence context; edits may uniquify.
   * - :class:`najaeda.netlist.Term`
     - :class:`najaeda.naja.SNLTerm`, :class:`najaeda.naja.SNLBitTerm`, or
       :class:`najaeda.naja.SNLInstTerm`
     - May represent top terms or instance terms.
   * - :class:`najaeda.netlist.Net`
     - :class:`najaeda.naja.SNLNet`, :class:`najaeda.naja.SNLBitNet`, or
       concatenated bits
     - Use high-level wrappers for hierarchical net edits.
   * - :class:`najaeda.netlist.Equipotential`
     - :class:`najaeda.naja.SNLEquipotential`
     - Native object used for flat connectivity.
   * - :class:`najaeda.netlist.Attribute`
     - :class:`najaeda.naja.SNLAttribute`
     - Metadata attached to SNL design objects.

Raw construction example
------------------------

The exact factory signatures are native CPython bindings.  The following
pattern shows the intended ownership flow:

.. code-block:: python

   from najaeda import naja

   universe = naja.NLUniverse.create()
   db = naja.NLDB.create(universe)
   lib = naja.NLLibrary.create(db, "work")
   design = naja.SNLDesign.create(lib, "top")
   universe.setTopDesign(design)

   a = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "a")
   y = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "y")
   n = naja.SNLScalarNet.create(design, "n")

   a.setNet(n)
   y.setNet(n)

For Verilator verification, ``SNLDesign.dumpGateLevelVerilog(path, top_file_name,
verilatorSplitPackedSignals=True)`` annotates internal bus nets and packed
child-module ports with ``/* verilator split_var */``. Public top-module ports
are excluded. The keyword defaults to ``False``; comments affect Verilator's
dependency tracking and do not change the netlist logic or suppress warnings.
The high-level API exposes the same option through
``najaeda.netlist.GateLevelVerilogDumpConfig``. Library-only dumps annotate internal nets
but leave module ports unannotated because no single public top is specified.

Object lifetime and safety
--------------------------

Raw objects are views of native C++ objects.  After calling ``destroy()`` or
after resetting/destroying the owning universe, database, library, or design,
do not keep using Python objects that referenced the destroyed native object.

The raw API also exposes shared model state.  Renaming, reconnecting, or
destroying an object through a raw handle mutates the underlying SNL object
directly.  If the same model is instantiated in multiple places, that change
can affect all occurrences unless you explicitly uniquify first.

SystemVerilog frontend intent helpers
-------------------------------------

The raw module includes expert helpers for live SystemVerilog frontend data:

* :func:`najaeda.naja.intentAvailable`
* :func:`najaeda.naja.liveCompilation`
* :func:`najaeda.naja.astSymbolOf`
* :func:`najaeda.naja.snlObjectsOf`
* :func:`najaeda.naja.intentParametersOf`
* :func:`najaeda.naja.intentTypeOf`
* :func:`najaeda.naja.intentPackageMember`

These helpers return plain Python data or capsules tied to the latest retained
SystemVerilog frontend state.  Treat capsules as opaque handles.

.. _raw-vhdl-intent:

VHDL semantic intent (first increment)
--------------------------------------

Use the raw API directly when a tool needs source declaration identities and
elaboration values. There is currently no high-level wrapper for these queries.
Existing SystemVerilog query dictionaries and Slang capsules are unchanged.

.. code-block:: python

   from najaeda import naja

   universe = naja.NLUniverse.create()
   db = naja.NLDB.create(universe)
   top = db.loadVHDL(files=["types.vhd", "design.vhd"], top="top",
                     keep_ast_link=True, diagnostics_report_path=None)
   catalogue = naja.intentDescribe(top)
   if catalogue["status"] == "available":
       declarations = catalogue["declarations"]  # ordered list, not a name-keyed map
   signal = top.getNet("current_phase")
   declared = naja.intentDescribe(signal)["declared_type"]
   resolved = naja.intentTypeOf(signal)
   parameters = naja.intentParametersOf(top)

``keep_ast_link`` defaults to ``False``. For VHDL it retains copied semantic
records, not a parser AST or a Slang compilation. This reuses the existing
source-intent retention option without introducing a second public option.
Capture reads the RTL elaborator's resolved shapes, declaration bindings and
values; Python does not parse or search source text to reconstruct semantics.

``intentDescribe(object)`` accepts an ``SNLDesign``, an ``SNLDesignObject`` or
an ``NLLibrary``. It returns an explicit ``status`` envelope. A retained design
has ``language="vhdl"``, ``entity``, ``architecture``, a specialization
``context`` and an ordered ``declarations`` list. An explicit instance also
has ``instance``, ``model_context`` and ``generic_actuals``; its context is
relative to its owning design. Compose parent instance contexts when walking
occurrences. Instances with identical actuals may share a model; distinct
specializations carry their own effective values. A signal or port has its
declaration record, ``declared_type`` and ``resolved_type``. Querying a bus bit
returns the whole declaration's intent, rather than inventing a VHDL bit type.
Generated helper objects without a declaration report ``status="unavailable"``.
This new envelope query currently provides VHDL data only; use the existing
intent queries for SystemVerilog.

Declaration records separate ``id`` from ``name``, ``canonical_name``, ``source``
and ownership (``library``, ``owner_kind``, ``owner``). IDs are opaque,
session-local identifiers for parser declarations, shared by specializations
of that declaration. They are not filenames, display names, persistent IDs or
SNL IDs. Use a declaration ID together with its elaboration context. Do not key
catalogues by display name: process-local declarations can shadow outer ones.
``lexical_scopes`` identifies enclosing processes and subprograms; ``scope``
contains the elaborated generate prefix where applicable. Basic identifiers
retain the frontend's case-insensitive canonical spelling; extended identifiers
retain case. Library ownership and ``work`` resolution use the same root-library
bindings as elaboration, including package imports.

For VHDL, ``intentTypeOf(signal_or_port)`` returns the resolved shape, for example
(the declaration and source dictionaries are omitted here):

.. code-block:: python

   {"status": "available", "name": "phase_t", "kind": "enumeration",
    "width": 2, "ranges": [], "encoding": "unsigned_binary_ordinal",
    "literals": [{"name": "resting", "ordinal": 0, "representation": 0},
                 {"name": "working", "ordinal": 1, "representation": 1},
                 {"name": "finished", "ordinal": 2, "representation": 2}]}

Enumeration encoding describes Naja's lowering, not a user-specified VHDL
encoding attribute. An enum type alone is not evidence of an FSM. Record shapes
contain ordered ``fields``, each with a resolved ``type`` and a ``declared_type``
where retained. Array shapes contain ordered ``ranges`` with ``left``, ``right``,
``direction`` (``to`` or ``downto``), ``length`` and ``index_kind`` (and an ``index_type`` declaration for enum indices), plus a
recursive ``element_type``. Widths come from resolved shapes, not from guessing
nominal types from SNL bus widths. The declared type preserves subtype aliases;
``subtype_constraint`` contains resolved subtype bounds when retained.

``intentParametersOf(design_or_instance)`` returns
``{"status": "available", "language": "vhdl", "params": [...]}`` with generic
and constant declaration records. It does not use SV-specific ``localparam``
fields. For example, a generic overridden from 3 to 5 has:

.. code-block:: python

   {"name": "width", "kind": "generic", "overridden": True,
    "default_value": 3, "effective_value": 5,
    "default_expression": {"text": "3", "dependencies": [], "source": {...}},
    ...}

Defaults are evaluated in the declaration's effective preceding-generic
environment. Additional evaluation of an overridden default is restricted to
side-effect-free scalar expressions and supported string expressions. Other
defaults carry ``default_value_status``. Non-overridden defaults reuse the value
already computed by elaboration. Vector/record/enum constants carry bits in
Naja's declaration order, identified by ``value_format``; integer and boolean
values remain Python integers and booleans. ``expression`` and
``default_expression`` retain parser-delimited source and source coordinates.
Dependencies use declaration IDs when the elaborator's active binding is
retained. Calls, selected names and unavailable bindings report
``unsupported`` or ``unresolved`` explicitly; these are not text-search guesses.

Lifecycle and limits
~~~~~~~~~~~~~~~~~~~~

* The database owns a load session; private properties on live objects own the
  copied records and hold weak session references. No AST, netlist or Python
  pointers occur in returned dictionaries. Record storage is released with its
  owner. Returned Python dictionaries are independent historical copies.
* Every attempted raw VHDL load invalidates the previous VHDL session in that
  database, including argument errors, missing files and elaboration failures.
  A later non-retained load reports intent unavailable. Database/universe
  destruction (including reset) invalidates the session; querying a destroyed
  Python wrapper raises ``RuntimeError``. Object deletion removes its record.
* Incremental sources and package declarations still follow normal VHDL library
  retention. Each retained load describes its newly elaborated hierarchy.
  Package-only loads return ``None``; query the destination library for its
  declaration catalogue. Enum declarations and declared type/field information
  are available there, but constants and constraints needing elaboration carry
  an explicit unavailable status. Required-generic units deferred without a top
  have no elaborated intent until instantiated.
* An unchanged design returned from the existing model cache is not re-elaborated
  and its previous intent stays invalid. To enable intent for an existing
  non-retained or cached model, elaborate the source inputs in a fresh database
  with ``keep_ast_link=True``. Do not treat a cache hit as renewed semantic data.
* Semantic records are not serialized in NajaIF. Cold reload reports unavailable,
  even though source references survive. Re-elaborate the original VHDL sources
  in a fresh database with retention enabled to recover intent. Snapshot payloads,
  manifests, SNL ordering, IDs and connectivity are unaffected by retention.
* ``intentAvailable()`` is a global availability hint across retained SV and VHDL
  data; use ``intentDescribe(object)["status"]`` for VHDL object availability.
  ``intentTypeOf`` and ``intentParametersOf`` retain their legacy ``None`` result
  when no applicable live metadata exists. ``intentPackageMember`` remains an SV
  query; VHDL packages are selected explicitly through library catalogues and
  their ownership fields. Slang compilation/symbol capsule APIs remain SV-only.
* The supported VHDL language subset is unchanged. Unelaborated package records,
  function-local temporary evaluation frames, static integer tables, constrained
  enum subtypes and arbitrary dependency graphs are not a complete semantic model.
  The legacy unnamed-library C++ structural lowering path has no intent provider.

Integration for external intent tools
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A VHDL ``load_intent`` implementation should retain its own ordered input and
library load configuration, create a fresh raw database, and call ``loadVHDL``
with ``keep_ast_link=True`` for the final hierarchy elaboration. For package-only
loads it should query the destination library. A VHDL ``get_intent`` implementation
can resolve its structural target, call ``intentDescribe`` and, when available,
use ``intentTypeOf`` or ``intentParametersOf``. Keep declaration IDs, occurrence
paths, model contexts and source references separate. Propagate unavailable and
unsupported fields; do not fabricate SV schema fields or infer FSMs. After a
snapshot reload or cached/non-retained session, offer source re-elaboration in a
fresh database. No external-tool implementation is included in Naja.

.. _primitive-timing-modeling:

Primitive timing modeling
--------------------------

Use the raw API when defining a primitive library in Python.  The high-level
:mod:`najaeda.netlist` API loads and consumes primitive libraries, while the
raw objects expose the definition-time timing decorators.

Primitive designs are hierarchy leaves.  Their interface, parameters, and
timing metadata can be defined directly, but they cannot contain instances.
Accordingly, ``SNLInstance.create(owner, model, name)`` raises
``RuntimeError`` when ``owner`` is primitive.  Passing a primitive as
``model`` remains the normal way to instantiate that primitive in a
non-primitive design.

SystemVerilog loading can produce canonical ``naja_add__w...``,
``naja_sub__w...``, ``naja_mul__w...`` and ``naja_shift__...`` models.
Addition, subtraction and multiplication retain the low ``WIDTH`` result
bits after frontend operand sizing. Add/subtract models have width-bit ``A``,
``B`` and ``Y`` ports; their operation is fixed by their canonical family. These word-level models expose conservative
combinational dependencies through ``SNLDesign.getCombinatorialInputs`` and
``getCombinatorialOutputs``; they do not expose Boolean truth tables. Inspect
these models through the raw API when working on primitive-level analysis;
normal loading and exporting use the high-level API.

Sequential primitives should declare both their timing arcs and the role of
each bit term:

.. code-block:: python

   from najaeda import naja

   dff = naja.SNLDesign.createPrimitive(primitives, "DFFRN")
   clk = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Input, "CLK")
   data = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Input, "D")
   reset_n = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Input, "RESET_N")
   output = naja.SNLScalarTerm.create(
       dff, naja.SNLTerm.Direction.Output, "Q")

   naja.SNLDesign.addInputsToClockArcs([data, reset_n], clk)
   naja.SNLDesign.addClockToOutputsArcs(clk, output)
   clk.setRole(naja.SNLTermRole.Clock)
   data.setRole(naja.SNLTermRole.DataInput)
   reset_n.setRole(
       naja.SNLTermRole.AsyncReset, naja.SNLActiveLevel.Low)
   output.setRole(naja.SNLTermRole.DataOutput)

``SNLActiveLevel`` applies to asynchronous and synchronous reset/set roles.
The role and active level are inherited by instance terms.  Use
``getClockTerms()``, the other role-based design iterators, and the
``is_*`` term predicates to inspect the resulting model.

Parameter-dependent term roles
------------------------------

For vendor primitives whose pin meaning changes with instance parameters,
use ``SNLDesign.setRolesFromParameters(parameters, roles)``. Each entry has
an ordered unsigned integer ``values`` list and a ``roles`` dictionary
mapping scalar terms or bus bits to ``(SNLTermRole, SNLActiveLevel)`` or
``None``. ``None`` explicitly removes the role, resolving to
``SNLTermRole.Other`` and ``SNLActiveLevel.NA`` even if the static term has
a control role. Omitted terms retain their static role.

.. code-block:: python

   from itertools import product

   names = ["reset_present", "reset_synchronous", "reset_sets_value", "enable_present"]
   for name in names:
       naja.SNLParameter.createBinary(cell, name, 1, 0)
   reset = cell.getScalarTerm("R")
   load = cell.getScalarTerm("L")
   role, level = naja.SNLTermRole, naja.SNLActiveLevel
   entries = []
   for init, sync, typ, use_load in product((0, 1), repeat=4):
       reset_role = ((role.SyncSet if typ else role.SyncReset) if sync
                     else (role.AsyncSet if typ else role.AsyncReset))
       entries.append({
           "values": [init, sync, typ, use_load],
           "roles": {
               reset: (reset_role, level.High) if init else None,
               load: (role.Enable, level.NA) if use_load else None,
           },
       })
   cell.setRolesFromParameters(names, entries)

Selectors must be distinct names of Binary, Boolean, or Decimal parameters
on a primitive design. Instance overrides take precedence over defaults;
numeric HDL literals are resolved using the same parser as parameterized
sequential models. Missing required values, invalid literals, and unmatched
configurations raise ``RuntimeError`` on instance queries. Duplicate keys,
foreign terms, invalid enums, and malformed declarations are rejected without
replacing the existing table. Calling the setter again replaces the table.
``hasRolesFromParameters()`` reports whether a table is present.

``SNLInstTerm.getRole()``, ``getResetActiveLevel()``, and all ``is_*`` role
predicates resolve the current instance parameters on every query.
``SNLOccurrence.getRole()`` and ``getResetActiveLevel()`` do the same when
the occurrence references an instance term; bit-term occurrences report
static roles, and non-term occurrences report ``Other`` / ``NA``.
``SNLBitTerm.getRole()`` and design role iterators still report static roles.
Without a table, existing static-role behavior is preserved.

Declare tables through the raw API even when traversing with the high-level
API: ``najaeda.netlist.Term.get_role()``, ``get_reset_active_level()``, and
``isClock()``, ``isReset()``, ``isEnable()`` and the other role predicates
use the resolved instance term. There is no high-level declaration wrapper.
These tables are independent of sequential models; setting a sequential
model does not infer or replace roles. Like existing primitive modeling
metadata, custom role tables are runtime declarations; reattach them when
reconstructing primitive models after loading a snapshot.

The native equivalent is
``SNLDesignModeling::setRolesFromParameters(design, names, table)`` using
``TermRoleTable`` (numeric key vectors mapping to ``TermRoles``). An empty
``TermRole{}`` expresses Python's ``None``. Query through the
``SNLDesignModeling`` instance-term overloads.

Parameterized combinational timing selects one design parameter and records
an arc set for each value.  Two-argument ``addCombinatorialArcs`` calls record
the default value's arcs; the three-argument form takes the parameter value
first:

.. code-block:: python

   naja.SNLParameter.createString(cell, "MODE", "NORMAL")
   cell.setTimingModelParameter("MODE", "NORMAL")
   naja.SNLDesign.addCombinatorialArcs(input_a, output_a)
   naja.SNLDesign.addCombinatorialArcs(
       "CROSS", input_a, output_b)

The built-in loaders under :mod:`najaeda.primitives` use the same API, so
Python-defined primitives expose the same arc, role, and active-level
metadata as primitives constructed by the C++ DB0 and Liberty paths.

For primitives such as FPGA lookup tables whose function is held in an
instance parameter, use ``setTruthTableFromParameter(output, inputs,
parameter, bit_offset=0)``. The input list must follow design order and may
contain up to six terms. Naja resolves the instance override on first use and
caches the resulting truth table for that instance.

Liberty loading preserves every distinct sequential dependency declared by
setup, hold, or edge timing groups.  Consequently,
``getInputRelatedClocks()`` and ``getOutputRelatedClocks()`` can return more
than one clock for a term; repeated groups for the same term and clock are
deduplicated.  The generic timing-arc model does not retain Liberty ``when``
expressions, so conditionally enabled arcs are exposed conservatively as
unconditional dependencies.  The generic ``MemoryInterface`` remains a
single-clock abstraction: per-port clocks on a fully modeled multi-clock
memory are not represented by that interface.

Parameter-dependent sequential models
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``SNLDesign.setSequentialModelFromParameters(parameters, models)`` when
instance parameters select sequential behavior. ``parameters`` is an ordered
list of numeric parameter names declared on the primitive. ``models`` is a
list of dictionaries with ``values`` (unsigned integers in the same order),
``clocked_on``, ``states``, ``outputs``, and optional ``kind``. The latter four
fields use the same syntax as ``setSequentialModel``. Duplicate keys, invalid
expressions, foreign terms, and undeclared/string parameters are rejected.
A declaration replaces the previous static model or table atomically after
validation. A partial table is allowed; an unmatched instance is an error.

This table design reuses ordinary Boolean expression trees and supports both
clock polarities and optional asynchronous controls without callbacks or a new
expression language. C++ callers use
``SNLDesignModeling::setSequentialModelFromParameters(design, names, table)``;
``SequentialModelTable`` maps ``std::vector<uint64_t>`` keys to
``SequentialModel`` values. ``hasSequentialModel(design)`` includes tables;
``hasSequentialModelFromParameters(design)`` (Python
``design.hasSequentialModelFromParameters()``) distinguishes them. Resolve with
``SNLDesignModeling::getSequentialModel(instance)`` or Python
``instance.getSequentialModel()``. The C++ instance getter returns the exact
stored static model when no table is present. The design getter raises for a
table, preventing accidental use of a single model for every instance.

An instance override takes precedence over its parameter's default. A required
parameter without an override, unknown bits, malformed/overflowing unsigned
values, and an unmatched key raise ``NLException`` (Python ``RuntimeError``).
Diagnostics identify the primitive and instance, selector name and raw value,
whether the value came from an override or default, and available keys for an
unmatched selection. Declaration errors identify the model key, field, state,
output, node or operand index, expected counts/ranges, and conflicting term
ownership. Python parsing errors also include the supplied model entry and
field location while retaining the underlying parser error.
Numeric values accept decimal and Verilog based literals such as ``1'b1``;
unbased ``'0``/``'1`` denote zero/one. The selected table entry is cached per
instance under a mutex. Creating, changing, or destroying an override,
changing the instance model, destroying the instance, and redeclaring the
sequential model invalidate the selection. References remain valid until the
primitive's model is replaced or destroyed; finish declarations before parallel
read access and do not mutate the netlist concurrently with readers.

Python design and instance getters return fresh dictionaries with ``kind``,
``clocked_on``, ``states``, and ``outputs``. Expressions are trees: Boolean
constants, ``("term", SNLBitTerm)``, ``("state", index)``,
``("not", expression)``, or ``("and" | "or" | "xor", left, right)``.
State dictionaries contain ``next_state``, optional ``clear``/``preset``
(``None`` when absent), and ``clear_preset_value``. Outputs are
``(SNLBitTerm, expression)`` tuples. Term leaves refer to the primitive's
terms; use ``instance.getInstTerm(term)`` for connections. State indices refer
to the returned state list. These getters expose parsed trees rather than the
original declaration strings. This feature belongs in the raw API for primitive
library authors; general instance traversal uses ``najaeda.netlist``.

The following generic flip-flop example supports both clock edges, optional
load enable, and synchronous or asynchronous reset/set. ``FALLING_EDGE=0``
selects rising edges; ``USE_RESET`` and ``USE_ENABLE`` enable R and L;
``SYNC_RESET=0`` selects asynchronous reset; ``RESET_VALUE`` selects zero or
one. R is active high, and reset/set takes priority over load enable.

.. code-block:: python

   from itertools import product
   from najaeda import naja

   def create_parameterized_dff(primitives):
       ff = naja.SNLDesign.createPrimitive(primitives, "PARAM_DFF")
       pins = {}
       for name in ("I", "CK", "L", "R", "O"):
           direction = (naja.SNLTerm.Direction.Output if name == "O"
                        else naja.SNLTerm.Direction.Input)
           pins[name] = naja.SNLScalarTerm.create(ff, direction, name)
       names = ["FALLING_EDGE", "USE_RESET", "USE_ENABLE", "SYNC_RESET", "RESET_VALUE"]
       for name in names:
           naja.SNLParameter.createBinary(ff, name, 1, 0)
       models = []
       for edge, init, load, sync, typ in product((0, 1), repeat=5):
           next_state = "(L & I) | (!L & IQ)" if load else "I"
           state = {"name": "IQ"}
           if init:
               if sync:
                   next_state = (f"R | (!R & ({next_state}))" if typ
                                 else f"!R & ({next_state})")
               else:
                   state["preset" if typ else "clear"] = "R"
           state["next_state"] = next_state
           models.append({
               "values": [edge, init, load, sync, typ],
               "clocked_on": "!CK" if edge else "CK",
               "states": [state], "outputs": [(pins["O"], "IQ")],
           })
       ff.setSequentialModelFromParameters(names, models)
       ff.addClockToOutputsArcs(pins["CK"], [pins["O"]])
       ff.addInputsToClockArcs([pins[n] for n in ("I", "L", "R")], pins["CK"])
       return ff

Timing arcs describe conservative structural/timing relationships across all
configurations; they do not assert that every related pin updates every
instance. CK→O remains present so consumers can identify state outputs.
I/L/R→CK remains present even when an instance ignores L or R. Consumers must
use the resolved model's reachable expression term leaves to determine update
pins: an arc-only L (USE_ENABLE=0) or R (USE_RESET=0) is unused, not an unsupported update
pin. Include asynchronous clear/preset leaves in this analysis. Arc and term-role
queries remain primitive metadata; they do not change reset mode per instance.

As with static sequential models, these decorations are currently private
in-memory properties and are not serialized by NajaIF; reattach primitive
models after loading snapshots. Cloning a containing design retains its shared
primitive and parameter overrides, so its instances resolve the same models.
Primitive design cloning itself remains unsupported.

Raw module reference
--------------------

The compiled extension exposes the following public objects in the current
binding.  Read this table as an expert index; the C++ SNL API remains the
semantic source of truth.

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Object
     - Main public methods and values
   * - :class:`najaeda.naja.SNLParameter`
     - :meth:`createDecimal <najaeda.naja.SNLParameter.createDecimal>`, :meth:`createBinary <najaeda.naja.SNLParameter.createBinary>`, :meth:`createBoolean <najaeda.naja.SNLParameter.createBoolean>`, :meth:`createString <najaeda.naja.SNLParameter.createString>` (optional default value), :meth:`hasDefaultValue <najaeda.naja.SNLParameter.hasDefaultValue>`, :meth:`getValue <najaeda.naja.SNLParameter.getValue>`, :meth:`getName <najaeda.naja.SNLParameter.getName>`, :meth:`getDesign <najaeda.naja.SNLParameter.getDesign>`, :meth:`destroy <najaeda.naja.SNLParameter.destroy>`
   * - :class:`najaeda.naja.NLClock`
     - :meth:`createPrimary <najaeda.naja.NLClock.createPrimary>`, :meth:`createGenerated <najaeda.naja.NLClock.createGenerated>`, :meth:`getName <najaeda.naja.NLClock.getName>`, :meth:`getKind <najaeda.naja.NLClock.getKind>`, :meth:`getDesign <najaeda.naja.NLClock.getDesign>`, :meth:`getPeriod <najaeda.naja.NLClock.getPeriod>`, :meth:`getFrequency <najaeda.naja.NLClock.getFrequency>`, :meth:`getTimeUnit <najaeda.naja.NLClock.getTimeUnit>`, :meth:`getRiseAt <najaeda.naja.NLClock.getRiseAt>`, :meth:`getFallAt <najaeda.naja.NLClock.getFallAt>`, :meth:`setWaveform <najaeda.naja.NLClock.setWaveform>`, :meth:`getSources <najaeda.naja.NLClock.getSources>`, :meth:`getMaster <najaeda.naja.NLClock.getMaster>`, :meth:`getMasterSource <najaeda.naja.NLClock.getMasterSource>`, :meth:`getDivideBy <najaeda.naja.NLClock.getDivideBy>`, :meth:`getMultiplyBy <najaeda.naja.NLClock.getMultiplyBy>`, :meth:`isInverted <najaeda.naja.NLClock.isInverted>`, :meth:`getRootClock <najaeda.naja.NLClock.getRootClock>`, :meth:`isSynchronousWith <najaeda.naja.NLClock.isSynchronousWith>`, :meth:`destroy <najaeda.naja.NLClock.destroy>`
   * - :class:`najaeda.naja.NLUniverse`
     - :meth:`create <najaeda.naja.NLUniverse.create>`, :meth:`destroy <najaeda.naja.NLUniverse.destroy>`, :meth:`get <najaeda.naja.NLUniverse.get>`, :meth:`getDB <najaeda.naja.NLUniverse.getDB>`, :meth:`getTopDB <najaeda.naja.NLUniverse.getTopDB>`, :meth:`setTopDB <najaeda.naja.NLUniverse.setTopDB>`, :meth:`getTopDesign <najaeda.naja.NLUniverse.getTopDesign>`, :meth:`setTopDesign <najaeda.naja.NLUniverse.setTopDesign>`, :meth:`getUserDBs <najaeda.naja.NLUniverse.getUserDBs>`, :meth:`getSNLDesign <najaeda.naja.NLUniverse.getSNLDesign>`, :meth:`getObject <najaeda.naja.NLUniverse.getObject>`, :meth:`applyDLE <najaeda.naja.NLUniverse.applyDLE>`, :meth:`applyConstantPropagation <najaeda.naja.NLUniverse.applyConstantPropagation>`, :meth:`getMaxFanout <najaeda.naja.NLUniverse.getMaxFanout>`, :meth:`getMaxLogicLevel <najaeda.naja.NLUniverse.getMaxLogicLevel>`
   * - :class:`najaeda.naja.NLDB`
     - :meth:`create <najaeda.naja.NLDB.create>`, :meth:`destroy <najaeda.naja.NLDB.destroy>`, :meth:`getID <najaeda.naja.NLDB.getID>`, :meth:`getNLID <najaeda.naja.NLDB.getNLID>`, :meth:`isTopDB <najaeda.naja.NLDB.isTopDB>`, :meth:`getLibraries <najaeda.naja.NLDB.getLibraries>`, :meth:`getLibrary <najaeda.naja.NLDB.getLibrary>`, :meth:`getGlobalLibraries <najaeda.naja.NLDB.getGlobalLibraries>`, :meth:`getPrimitiveLibraries <najaeda.naja.NLDB.getPrimitiveLibraries>`, :meth:`getTopDesign <najaeda.naja.NLDB.getTopDesign>`, :meth:`loadGateLevelVerilog <najaeda.naja.NLDB.loadGateLevelVerilog>`, :meth:`loadVerilog (compatibility alias) <najaeda.naja.NLDB.loadVerilog>`, :meth:`loadSystemVerilog <najaeda.naja.NLDB.loadSystemVerilog>`, :meth:`loadVHDL <najaeda.naja.NLDB.loadVHDL>`, :meth:`loadLibertyPrimitives <najaeda.naja.NLDB.loadLibertyPrimitives>`, :meth:`loadNajaIF <najaeda.naja.NLDB.loadNajaIF>`, :meth:`dumpNajaIF <najaeda.naja.NLDB.dumpNajaIF>`, :meth:`dumpGateLevelVerilog <najaeda.naja.NLDB.dumpGateLevelVerilog>`, :meth:`dumpVerilog (compatibility alias) <najaeda.naja.NLDB.dumpVerilog>`, :meth:`getClocks <najaeda.naja.NLDB.getClocks>`
   * - :class:`najaeda.naja.NLLibrary`
     - :meth:`create <najaeda.naja.NLLibrary.create>`, :meth:`createPrimitives <najaeda.naja.NLLibrary.createPrimitives>`, :meth:`getDB <najaeda.naja.NLLibrary.getDB>`, :meth:`getID <najaeda.naja.NLLibrary.getID>`, :meth:`getNLID <najaeda.naja.NLLibrary.getNLID>`, :meth:`getName <najaeda.naja.NLLibrary.getName>`, :meth:`setName <najaeda.naja.NLLibrary.setName>`, :meth:`isStandard <najaeda.naja.NLLibrary.isStandard>`, :meth:`isPrimitives <najaeda.naja.NLLibrary.isPrimitives>`, :meth:`getSNLDesign <najaeda.naja.NLLibrary.getSNLDesign>`, :meth:`getSNLDesigns <najaeda.naja.NLLibrary.getSNLDesigns>`, :meth:`getLibrary <najaeda.naja.NLLibrary.getLibrary>`, :meth:`getLibraries <najaeda.naja.NLLibrary.getLibraries>`
   * - :class:`najaeda.naja.SNLDesign`
     - :meth:`create <najaeda.naja.SNLDesign.create>`, :meth:`createPrimitive <najaeda.naja.SNLDesign.createPrimitive>`, :meth:`clone <najaeda.naja.SNLDesign.clone>`, :meth:`destroy <najaeda.naja.SNLDesign.destroy>`, :meth:`getName <najaeda.naja.SNLDesign.getName>`, :meth:`setName <najaeda.naja.SNLDesign.setName>`, :meth:`getDB <najaeda.naja.SNLDesign.getDB>`, :meth:`getLibrary <najaeda.naja.SNLDesign.getLibrary>`, :meth:`getID <najaeda.naja.SNLDesign.getID>`, :meth:`getNLID <najaeda.naja.SNLDesign.getNLID>`, :meth:`getRevisionCount <najaeda.naja.SNLDesign.getRevisionCount>`, :meth:`getTerms <najaeda.naja.SNLDesign.getTerms>`, :meth:`getTerm <najaeda.naja.SNLDesign.getTerm>`, :meth:`getTermByID <najaeda.naja.SNLDesign.getTermByID>`, :meth:`getScalarTerms <najaeda.naja.SNLDesign.getScalarTerms>`, :meth:`getBusTerms <najaeda.naja.SNLDesign.getBusTerms>`, :meth:`getBundleTerms <najaeda.naja.SNLDesign.getBundleTerms>`, :meth:`getNets <najaeda.naja.SNLDesign.getNets>`, :meth:`getNet <najaeda.naja.SNLDesign.getNet>`, :meth:`getScalarNets <najaeda.naja.SNLDesign.getScalarNets>`, :meth:`getBusNets <najaeda.naja.SNLDesign.getBusNets>`, :meth:`getInstances <najaeda.naja.SNLDesign.getInstances>`, :meth:`getInstance <najaeda.naja.SNLDesign.getInstance>`, :meth:`getInstanceByID <najaeda.naja.SNLDesign.getInstanceByID>`, :meth:`getInstanceByIDList <najaeda.naja.SNLDesign.getInstanceByIDList>`, :meth:`getParameters <najaeda.naja.SNLDesign.getParameters>`, :meth:`getParameter <najaeda.naja.SNLDesign.getParameter>`, :meth:`addCombinatorialArcs <najaeda.naja.SNLDesign.addCombinatorialArcs>`, :meth:`addInputsToClockArcs <najaeda.naja.SNLDesign.addInputsToClockArcs>`, :meth:`addClockToOutputsArcs <najaeda.naja.SNLDesign.addClockToOutputsArcs>`, :meth:`setTimingModelParameter <najaeda.naja.SNLDesign.setTimingModelParameter>`, :meth:`getCombinatorialInputs <najaeda.naja.SNLDesign.getCombinatorialInputs>`, :meth:`getCombinatorialOutputs <najaeda.naja.SNLDesign.getCombinatorialOutputs>`, :meth:`getClockRelatedInputs <najaeda.naja.SNLDesign.getClockRelatedInputs>`, :meth:`getClockRelatedOutputs <najaeda.naja.SNLDesign.getClockRelatedOutputs>`, :meth:`getInputRelatedClocks <najaeda.naja.SNLDesign.getInputRelatedClocks>`, :meth:`getOutputRelatedClocks <najaeda.naja.SNLDesign.getOutputRelatedClocks>`, :meth:`getClockTerms <najaeda.naja.SNLDesign.getClockTerms>`, :meth:`getAsyncResetTerms <najaeda.naja.SNLDesign.getAsyncResetTerms>`, :meth:`getAsyncSetTerms <najaeda.naja.SNLDesign.getAsyncSetTerms>`, :meth:`getSyncResetTerms <najaeda.naja.SNLDesign.getSyncResetTerms>`, :meth:`getSyncSetTerms <najaeda.naja.SNLDesign.getSyncSetTerms>`, :meth:`getDataInputTerms <najaeda.naja.SNLDesign.getDataInputTerms>`, :meth:`getOutputTerms <najaeda.naja.SNLDesign.getOutputTerms>`, :meth:`setTruthTable <najaeda.naja.SNLDesign.setTruthTable>`, :meth:`setSequentialModel <najaeda.naja.SNLDesign.setSequentialModel>`, :meth:`setSequentialModelFromParameters <najaeda.naja.SNLDesign.setSequentialModelFromParameters>`, :meth:`setRolesFromParameters <najaeda.naja.SNLDesign.setRolesFromParameters>`, :meth:`hasRolesFromParameters <najaeda.naja.SNLDesign.hasRolesFromParameters>`, :meth:`hasSequentialModel <najaeda.naja.SNLDesign.hasSequentialModel>`, :meth:`hasSequentialModelFromParameters <najaeda.naja.SNLDesign.hasSequentialModelFromParameters>`, :meth:`getSequentialModel <najaeda.naja.SNLDesign.getSequentialModel>`, :meth:`hasMemoryInterface <najaeda.naja.SNLDesign.hasMemoryInterface>`, :meth:`getMemoryInterface <najaeda.naja.SNLDesign.getMemoryInterface>`, :meth:`setTruthTableFromParameter <najaeda.naja.SNLDesign.setTruthTableFromParameter>`, :meth:`setTruthTables <najaeda.naja.SNLDesign.setTruthTables>`, :meth:`getTruthTable <najaeda.naja.SNLDesign.getTruthTable>`, :meth:`getTruthTableByOutputID <najaeda.naja.SNLDesign.getTruthTableByOutputID>`, :meth:`isConst0 <najaeda.naja.SNLDesign.isConst0>`, :meth:`isConst1 <najaeda.naja.SNLDesign.isConst1>`, :meth:`isConst <najaeda.naja.SNLDesign.isConst>`, :meth:`isBuf <najaeda.naja.SNLDesign.isBuf>`, :meth:`isInv <najaeda.naja.SNLDesign.isInv>`, :meth:`isAnd <najaeda.naja.SNLDesign.isAnd>`, :meth:`isNand <najaeda.naja.SNLDesign.isNand>`, :meth:`isOr <najaeda.naja.SNLDesign.isOr>`, :meth:`isNor <najaeda.naja.SNLDesign.isNor>`, :meth:`isXor <najaeda.naja.SNLDesign.isXor>`, :meth:`isXnor <najaeda.naja.SNLDesign.isXnor>`, :meth:`isMux <najaeda.naja.SNLDesign.isMux>`, :meth:`isAddSub <najaeda.naja.SNLDesign.isAddSub>`, :meth:`isSubtract <najaeda.naja.SNLDesign.isSubtract>`, :meth:`isMultiply <najaeda.naja.SNLDesign.isMultiply>`, :meth:`isShift <najaeda.naja.SNLDesign.isShift>`, :meth:`isDivMod <najaeda.naja.SNLDesign.isDivMod>`, :meth:`isShiftRight <najaeda.naja.SNLDesign.isShiftRight>`, :meth:`isShiftArithmetic <najaeda.naja.SNLDesign.isShiftArithmetic>`, :meth:`dumpGateLevelVerilog <najaeda.naja.SNLDesign.dumpGateLevelVerilog>`, :meth:`dumpVerilog (compatibility alias) <najaeda.naja.SNLDesign.dumpVerilog>`, :meth:`dumpFullDotFile <najaeda.naja.SNLDesign.dumpFullDotFile>`, :meth:`dumpContextDotFile <najaeda.naja.SNLDesign.dumpContextDotFile>`, :meth:`getClocks <najaeda.naja.SNLDesign.getClocks>`
   * - :class:`najaeda.naja.SNLInstance`
     - :meth:`create <najaeda.naja.SNLInstance.create>`, :meth:`destroy <najaeda.naja.SNLInstance.destroy>`, :meth:`getName <najaeda.naja.SNLInstance.getName>`, :meth:`setName <najaeda.naja.SNLInstance.setName>`, :meth:`getID <najaeda.naja.SNLInstance.getID>`, :meth:`getNLID <najaeda.naja.SNLInstance.getNLID>`, :meth:`getDesign <najaeda.naja.SNLInstance.getDesign>`, :meth:`getModel <najaeda.naja.SNLInstance.getModel>`, :meth:`getSequentialModel <najaeda.naja.SNLInstance.getSequentialModel>`, :meth:`getMemoryInterface <najaeda.naja.SNLInstance.getMemoryInterface>`, :meth:`getInstTerm <najaeda.naja.SNLInstance.getInstTerm>`, :meth:`getInstTerms <najaeda.naja.SNLInstance.getInstTerms>`, :meth:`getInstParameter <najaeda.naja.SNLInstance.getInstParameter>`, :meth:`getInstParameters <najaeda.naja.SNLInstance.getInstParameters>`, :meth:`getCombinatorialInputs <najaeda.naja.SNLInstance.getCombinatorialInputs>`, :meth:`getCombinatorialOutputs <najaeda.naja.SNLInstance.getCombinatorialOutputs>`, :meth:`getClockRelatedInputs <najaeda.naja.SNLInstance.getClockRelatedInputs>`, :meth:`getClockRelatedOutputs <najaeda.naja.SNLInstance.getClockRelatedOutputs>`, :meth:`getInputRelatedClocks <najaeda.naja.SNLInstance.getInputRelatedClocks>`, :meth:`getOutputRelatedClocks <najaeda.naja.SNLInstance.getOutputRelatedClocks>`
   * - :class:`najaeda.naja.SNLTerm` and term subclasses
     - :class:`Direction <najaeda.naja.SNLTerm.Direction>`, :meth:`getName <najaeda.naja.SNLTerm.getName>`, :meth:`setName <najaeda.naja.SNLTerm.setName>`, :meth:`getDirection <najaeda.naja.SNLTerm.getDirection>`, :meth:`getDesign <najaeda.naja.SNLTerm.getDesign>`, :meth:`getNet <najaeda.naja.SNLTerm.getNet>`, :meth:`setNet <najaeda.naja.SNLTerm.setNet>`, :meth:`getBits <najaeda.naja.SNLTerm.getBits>`, :meth:`getWidth <najaeda.naja.SNLTerm.getWidth>`, :meth:`getNLID <najaeda.naja.SNLTerm.getNLID>`, :meth:`getSourceLoc <najaeda.naja.SNLTerm.getSourceLoc>`, :meth:`hasSourceLoc <najaeda.naja.SNLTerm.hasSourceLoc>`; bit terms also expose :meth:`setRole <najaeda.naja.SNLBitTerm.setRole>`, :meth:`getRole <najaeda.naja.SNLBitTerm.getRole>`, :meth:`getResetActiveLevel <najaeda.naja.SNLBitTerm.getResetActiveLevel>`, :meth:`isClock <najaeda.naja.SNLBitTerm.isClock>`, :meth:`isAsyncReset <najaeda.naja.SNLBitTerm.isAsyncReset>`, :meth:`isAsyncSet <najaeda.naja.SNLBitTerm.isAsyncSet>`, :meth:`isSyncReset <najaeda.naja.SNLBitTerm.isSyncReset>`, :meth:`isSyncSet <najaeda.naja.SNLBitTerm.isSyncSet>`, :meth:`isReset <najaeda.naja.SNLBitTerm.isReset>`, :meth:`isEnable <najaeda.naja.SNLBitTerm.isEnable>`, :meth:`isData <najaeda.naja.SNLBitTerm.isData>`, :meth:`isDataInput <najaeda.naja.SNLBitTerm.isDataInput>`, :meth:`isDataOutput <najaeda.naja.SNLBitTerm.isDataOutput>`
   * - :class:`najaeda.naja.SNLNet` and net subclasses
     - :class:`Type <najaeda.naja.SNLNet.Type>`, :meth:`getName <najaeda.naja.SNLNet.getName>`, :meth:`setName <najaeda.naja.SNLNet.setName>`, :meth:`getDesign <najaeda.naja.SNLNet.getDesign>`, :meth:`getBits <najaeda.naja.SNLNet.getBits>`, :meth:`getWidth <najaeda.naja.SNLNet.getWidth>`, :meth:`getType <najaeda.naja.SNLBitNet.getType>`, :meth:`setType <najaeda.naja.SNLNet.setType>`, :meth:`getTypeAsString <najaeda.naja.SNLBitNet.getTypeAsString>`, :meth:`isConstant <najaeda.naja.SNLNet.isConstant>`, :meth:`isConstant0 <najaeda.naja.SNLNet.isConstant0>`, :meth:`isConstant1 <najaeda.naja.SNLNet.isConstant1>`, :meth:`getComponents <najaeda.naja.SNLBitNet.getComponents>`, :meth:`getInstTerms <najaeda.naja.SNLBitNet.getInstTerms>`, :meth:`getBitTerms <najaeda.naja.SNLBitNet.getBitTerms>`
   * - :class:`najaeda.naja.SNLInstTerm`
     - :meth:`getInstance <najaeda.naja.SNLInstTerm.getInstance>`, :meth:`getBitTerm <najaeda.naja.SNLInstTerm.getBitTerm>`, :meth:`getNet <najaeda.naja.SNLInstTerm.getNet>`, :meth:`setNet <najaeda.naja.SNLInstTerm.setNet>`, :meth:`getDirection <najaeda.naja.SNLInstTerm.getDirection>`, :meth:`getRole <najaeda.naja.SNLInstTerm.getRole>`, :meth:`getResetActiveLevel <najaeda.naja.SNLInstTerm.getResetActiveLevel>`, :meth:`isClock <najaeda.naja.SNLInstTerm.isClock>`, :meth:`isAsyncReset <najaeda.naja.SNLInstTerm.isAsyncReset>`, :meth:`isAsyncSet <najaeda.naja.SNLInstTerm.isAsyncSet>`, :meth:`isSyncReset <najaeda.naja.SNLInstTerm.isSyncReset>`, :meth:`isSyncSet <najaeda.naja.SNLInstTerm.isSyncSet>`, :meth:`isReset <najaeda.naja.SNLInstTerm.isReset>`, :meth:`isEnable <najaeda.naja.SNLInstTerm.isEnable>`, :meth:`isData <najaeda.naja.SNLInstTerm.isData>`, :meth:`isDataInput <najaeda.naja.SNLInstTerm.isDataInput>`, :meth:`isDataOutput <najaeda.naja.SNLInstTerm.isDataOutput>`
   * - :class:`najaeda.naja.SNLTermRole`
     - :attr:`Clock <najaeda.naja.SNLTermRole.Clock>`, :attr:`DataInput <najaeda.naja.SNLTermRole.DataInput>`, :attr:`DataOutput <najaeda.naja.SNLTermRole.DataOutput>`, :attr:`AsyncReset <najaeda.naja.SNLTermRole.AsyncReset>`, :attr:`AsyncSet <najaeda.naja.SNLTermRole.AsyncSet>`, :attr:`SyncReset <najaeda.naja.SNLTermRole.SyncReset>`, :attr:`SyncSet <najaeda.naja.SNLTermRole.SyncSet>`, :attr:`Enable <najaeda.naja.SNLTermRole.Enable>`, :attr:`ScanInput <najaeda.naja.SNLTermRole.ScanInput>`, :attr:`ScanEnable <najaeda.naja.SNLTermRole.ScanEnable>`, :attr:`MemoryReadAddress <najaeda.naja.SNLTermRole.MemoryReadAddress>`, :attr:`MemoryReadData <najaeda.naja.SNLTermRole.MemoryReadData>`, :attr:`MemoryWriteAddress <najaeda.naja.SNLTermRole.MemoryWriteAddress>`, :attr:`MemoryWriteData <najaeda.naja.SNLTermRole.MemoryWriteData>`, :attr:`MemoryWriteEnable <najaeda.naja.SNLTermRole.MemoryWriteEnable>`, :attr:`Other <najaeda.naja.SNLTermRole.Other>`
   * - :class:`najaeda.naja.SNLActiveLevel`
     - :attr:`High <najaeda.naja.SNLActiveLevel.High>`, :attr:`Low <najaeda.naja.SNLActiveLevel.Low>`, :attr:`NA <najaeda.naja.SNLActiveLevel.NA>`
   * - :class:`najaeda.naja.SNLPath`
     - :meth:`empty <najaeda.naja.SNLPath.empty>`, :meth:`size <najaeda.naja.SNLPath.size>`, :meth:`getInstances <najaeda.naja.SNLPath.getInstances>`, :meth:`getInstanceIDs <najaeda.naja.SNLPath.getInstanceIDs>`, :meth:`getHeadInstance <najaeda.naja.SNLPath.getHeadInstance>`, :meth:`getTailInstance <najaeda.naja.SNLPath.getTailInstance>`, :meth:`getHeadPath <najaeda.naja.SNLPath.getHeadPath>`, :meth:`getTailPath <najaeda.naja.SNLPath.getTailPath>`, :meth:`getDesign <najaeda.naja.SNLPath.getDesign>`, :meth:`getModel <najaeda.naja.SNLPath.getModel>`
   * - :class:`najaeda.naja.SNLOccurrence`
     - :meth:`getRole <najaeda.naja.SNLOccurrence.getRole>`, :meth:`getResetActiveLevel <najaeda.naja.SNLOccurrence.getResetActiveLevel>`, :meth:`isInstanceOccurrence <najaeda.naja.SNLOccurrence.isInstanceOccurrence>`, :meth:`getPath <najaeda.naja.SNLOccurrence.getPath>`, :meth:`getInstance <najaeda.naja.SNLOccurrence.getInstance>`, :meth:`getInstTerm <najaeda.naja.SNLOccurrence.getInstTerm>`, :meth:`getNetComponent <najaeda.naja.SNLOccurrence.getNetComponent>`, :meth:`getDesign <najaeda.naja.SNLOccurrence.getDesign>`
   * - :class:`najaeda.naja.SNLEquipotential`
     - :class:`Mode <najaeda.naja.SNLEquipotential.Mode>`, :meth:`getTerms <najaeda.naja.SNLEquipotential.getTerms>`, :meth:`getInstTermOccurrences <najaeda.naja.SNLEquipotential.getInstTermOccurrences>`, :meth:`isConst0 <najaeda.naja.SNLEquipotential.isConst0>`, :meth:`isConst1 <najaeda.naja.SNLEquipotential.isConst1>`, :meth:`dumpDotFile <najaeda.naja.SNLEquipotential.dumpDotFile>`
   * - :class:`najaeda.naja.NLID`
     - :meth:`fromString <najaeda.naja.NLID.fromString>`, :meth:`toTuple <najaeda.naja.NLID.toTuple>`, :meth:`getType <najaeda.naja.NLID.getType>`, :meth:`getDBID <najaeda.naja.NLID.getDBID>`, :meth:`getLibraryID <najaeda.naja.NLID.getLibraryID>`, :meth:`getDesignID <najaeda.naja.NLID.getDesignID>`, :meth:`getDesignObjectID <najaeda.naja.NLID.getDesignObjectID>`, :meth:`getInstanceID <najaeda.naja.NLID.getInstanceID>`, :meth:`getBit <najaeda.naja.NLID.getBit>`, :meth:`isDesign <najaeda.naja.NLID.isDesign>`, :meth:`isInstance <najaeda.naja.NLID.isInstance>`, :meth:`isNet <najaeda.naja.NLID.isNet>`, :meth:`isTerm <najaeda.naja.NLID.isTerm>`
   * - :class:`najaeda.naja.LogicCone`
     - :attr:`FanIn <najaeda.naja.LogicCone.FanIn>`, :attr:`FanOut <najaeda.naja.LogicCone.FanOut>`, :meth:`getDirection <najaeda.naja.LogicCone.getDirection>`, :meth:`getRoot <najaeda.naja.LogicCone.getRoot>`, :meth:`getNodes <najaeda.naja.LogicCone.getNodes>`, :meth:`getLeaves <najaeda.naja.LogicCone.getLeaves>`, :meth:`getNodeCount <najaeda.naja.LogicCone.getNodeCount>`
   * - Module functions
     - :func:`najaeda.naja.getVersion`, :func:`najaeda.naja.getGitHash`,
       :func:`najaeda.naja.snapshotManifest`,
       :func:`najaeda.naja.setLogLevel`, :func:`najaeda.naja.addLogFile`,
       :func:`najaeda.naja.clearLogSinks`,
       :func:`najaeda.naja.installLoggingHandler`, :func:`najaeda.naja.log`,
       :func:`najaeda.naja.logInfo`, :func:`najaeda.naja.logWarn`,
       :func:`najaeda.naja.logCritical`, :func:`najaeda.naja.intentDescribe` plus the source intent helpers
       listed above


Detailed raw reference
----------------------

The following class and method references are generated from the compiled
extension. Building this documentation requires an importable extension from
the same checkout.

.. py:module:: najaeda.naja

Core database objects
~~~~~~~~~~~~~~~~~~~~~

.. autosummary::

   NLUniverse
   NLDB
   NLLibrary
   SNLDesign

.. autoclass:: najaeda.naja.NLUniverse
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.NLDB
   :members:
   :undoc-members:
   :inherited-members:

The former ``NLDB.loadVerilog``, ``NLDB.dumpVerilog`` and
``SNLDesign.dumpVerilog`` names remain compatibility aliases. Each emits
``FutureWarning`` stating that it will be deprecated soon and naming its
``*GateLevelVerilog`` replacement. See the migration table in :doc:`loading`.

``NLDB.loadGateLevelVerilog`` loads **gate-level (structural) Verilog netlists**; it does
not provide general behavioral Verilog RTL support. For Verilog or SystemVerilog
RTL, including ``.v`` files, use ``NLDB.loadSystemVerilog`` within its supported
RTL subset. The normal high-level equivalents are ``netlist.load_gate_level_verilog`` and
``netlist.load_system_verilog``; see :doc:`loading`. ``NLDB.dumpGateLevelVerilog`` and
``SNLDesign.dumpGateLevelVerilog`` export structural netlists rather than reconstructing
the original behavioral RTL.

``NLDB.loadSystemVerilog(..., frontend_options={"slang": ["--relax-enum-conversions"]})``
passes backend-specific options through a frontend-neutral mapping. The high-level
counterpart is ``netlist.SystemVerilogConfig.frontend_options``. The default is
``None``; an empty dictionary or ``{"slang": []}`` adds no options. Currently only
the active frontend ``"slang"`` is accepted; unknown or inactive frontend names
raise an error rather than being silently ignored. Future adapters can define
their own option tokens in their own namespace.

For Slang each nonempty string is one ordered driver argument, without shell
parsing or quoting, and must contain no NUL bytes. Arguments follow typed options
and precede sources/filelists; Slang diagnoses unknown options and handles
repeated options. Use separate tokens for an option and its value, including
paths containing spaces. The example permits integral-to-enum conversions using
the enum base representation, including unnamed values. Other useful Slang
options are ``--allow-use-before-declare``, ``--timescale=1ns/1ps`` and
``--single-unit``. Strict frontend defaults remain in effect when no options are
supplied. These settings do not add unsupported Naja lowering capabilities.
Experts can pass them directly without creating a command file solely for
additional frontend settings. Common loader settings retain their typed APIs;
see :doc:`loading` for configuration examples.

``NLDB.loadSystemVerilog`` writes incremental frontend diagnostics to
``naja_sv_diagnostics.log`` by default. Pass ``diagnostics_report_path=None``
to disable the report file and keep diagnostics console-only.

``NLDB.loadSystemVerilog`` infers shared memories for direct nonblocking indexed
writes from multiple clocked processes, with the same supported forms and
collision restrictions as :doc:`loading`. No opt-in flag is required. Experts
can inspect the inferred primitive through ``SNLInstance.getModel()``:
``MULTI_CLOCK=1`` selects independent write clocks and masks, with ``WCLK``
(one bit per write action) and ``WMASK`` (``WR_PORTS * WIDTH`` bits).
``WADDR``/``WDATA``/``WMASK`` use port 0 in the least-significant slice;
write port i uses ``WE[i]``, with port 0 at bit 0, consistent with that
least-significant slice convention. ``CLK`` and ``RST`` are tied off for this variant.
In C++, ``SNLDesignModeling::MemoryInterface.clock`` is null for this variant;
clients must use each ``MemoryWritePort.clock`` instead of assuming one clock.
A write port with a null clock inherits the common interface clock. ``RADDR``/``RDATA``
remain asynchronous read ports; registered reads are separate flops.
Existing generic parameter, bus-term, and modeling queries expose this model;
no new Python construction API is required. ``WCLK`` bits have the ``Clock``
role and ``WMASK`` bits the ``MemoryWriteEnable`` role. Timing queries relate
each write port's address, data, enable and mask only to its own clock, while
all read outputs depend on every write clock. The model and connections survive
NajaIF snapshot round trips. Existing shared-clock memory signatures keep their
interface and identity.

``NLDB.loadSystemVerilog(..., blackbox_multi_writer_memories=True)`` preserves
unsupported multi-writer memory modules as port-only ``UserBlackBox`` models
and emits ``multi_writer_memory_blackbox`` diagnostics. The default is ``False``
(strict rejection). This omits the entire containing module's behavior for both
shared-clock and independent-clock writers. Experts can use the raw keyword
directly; the high-level ``SystemVerilogConfig`` exposes the same option.
``defines=["SYNTHESIS_MEMORY_BLACK_BOXING"]`` also works with OpenTitan/Pavona
memory models that honor this macro. See :doc:`loading` for examples and limits.

The same loader supports combinational ``casez``/``casex`` result-selection
functions in continuous assignments, including escaped function names and
concatenated arguments. Combinational compound shifts and fixed-size streaming
assignment targets are also supported, with the same supported forms and
explicit rejection of dynamic/overlapping streaming targets described in
:doc:`loading`.
Independent guarded ``always_latch`` writes also support static nested loops,
array elements, partial writes, and source-order priority, subject to the
dependency and scheduling restrictions in :doc:`loading`.
The high-level ``netlist.load_system_verilog`` API exposes the same support;
using the raw API is not required. See :doc:`loading` for matching semantics.

   Both ``NLDB.loadSystemVerilog`` and ``NLDB.loadVHDL`` accept
   ``library="DESIGN"`` to select a root ``NLLibrary`` in this database. Missing
   destinations are created. Basic names match case-insensitively, extended
   names exactly; ambiguous matches fail. Existing positional arguments retain
   their meaning. The high-level loaders expose the same keyword-only option.
   VHDL named-library references resolve only within these roots; ``work`` is
   the library owning the source unit, including imported packages. SV uses
   this option for destination storage, without cross-library source binding.

   ``NLDB.loadVHDL(file, top=None, diagnostics_report_path="naja_vhdl_diagnostics.log", library="DESIGN", keep_ast_link=False)`` accepts a single path or a ``list[str]`` and returns the selected top's
   ``SNLDesign``. ``files=`` is an alias for the legacy ``file=`` keyword; the
   legacy positional argument order is unchanged. All files are parsed before
   elaboration, and packages, entities, and architectures resolve independently
   of file order. With ``top=None``, a batch must have one uninstantiated entity
   in the destination library; multiple roots require an explicit ``top``.
   Missing dependencies and dependency cycles report the referencing file, line,
   and column after all inputs have been considered.
   A package-only load returns ``None`` and preserves the current top design.
   A file containing one entity with required generic values also returns
   ``None`` when ``top`` is omitted: its source is retained until a parent
   supplies those values through a generic map. Pass ``top`` explicitly to
   require immediate elaboration and diagnose any missing generic values.
   Arithmetic feeding unsigned ``resize`` or a static expression slice is
   elaborated at the required width. Loading preserves dead RTL objects and
   can retain unused logic across intermediate signals; call
   ``NLUniverse.get().applyDLE()`` after selecting the top for a pruned netlist.
   Vector casts also accept ``std_ulogic_vector`` and preserve operand width and
   bit order. Static ``boolean(...)`` conversions require boolean operands and
   retain short-circuit evaluation, including in generic defaults and package
   functions. See :doc:`loading` for conversion visibility, resize semantics,
   and the remaining limits.
   Array bounds, length, direction, range, and reverse-range attributes use
   declared subtype metadata without reading signal values. These attributes
   also support bounds derived from vector generics in direct-entity and component
   interfaces. See :doc:`loading` for supported prefixes and index-type limits.
   Pure binary scalar/vector functions lower to combinational hardware, including
   local variables, bounded loops, nested calls, and named/default arguments.
   Package and architecture declaration scopes are preserved. Integer/boolean
   actuals must be static; unsupported return paths and dynamic result bounds
   are diagnosed. See :doc:`loading` for the runtime function subset.
   Each Python call emits a ``RuntimeWarning`` because the VHDL parser is in
   Beta mode; its supported subset and behavior may change between releases.
   RTL source files and package declarations are retained in the live design
   library for subsequent loads; they are not serialized in NajaIF. Load these
   dependencies through the raw API; the high-level loader expects a completed
   top design. RTL lowering errors include the original source file, line, and
   column, including errors in a dependency elaborated by a later load.
   Package function declarations and bodies are also retained for later loads.
   Pure integer/boolean helpers can elaborate constants and bounds with static
   arguments; overload resolution and runtime/vector function evaluation are
   not yet supported. A package-only load validates parsing, not every function
   body's evaluability. Unsupported calls are diagnosed when elaboration needs
   them. The high-level loader uses the same evaluator once a top is available.
   Supported record ports become single ``SNLBusTerm`` objects with descending
   indices from total width minus one to zero. Fields occupy consecutive bits in
   declaration order, recursively; record field metadata is not exposed as raw
   SNL objects. Use :doc:`loading` for the supported record and unresolved-logic
   subset and current NEORV32 limitations.
   The RTL path infers a unique uninstantiated root entity, including references
   inside nested generate statements. Otherwise, specify ``top`` explicitly.
   The basic structural hierarchy path still requires an explicit ``top``.

   Architecture-local pure functions also support static integer/boolean evaluation.
   Bodies and default arguments use declarations visible at the function declaration,
   including enclosing generics and scalar constants. Runtime/vector evaluation,
   overloading, and architecture function forward declarations remain unsupported.
   Static function loops support unlabeled ``exit`` and ``exit when``; labeled
   exits and process-loop exits remain unsupported.

   The loader excludes simulation regions marked by ``pragma``, ``synthesis``,
   or ``synopsys`` ``translate_off/on`` comments, preserving source locations.
   Assertion and report statements are ignored, including conditions and
   ``severity failure``; configuration assertions do not validate generics.
   Each load logs the first occurrence of each warning code to the console.
   The diagnostics report records all occurrences with source path, line, and
   column, and is overwritten per load. Set ``diagnostics_report_path`` to a
   string path to choose the report, or ``None`` for console-only output.
   Warning codes currently include ``ignored-assertion`` and ``ignored-report``.
   Retained dependency sources are not reported again during internal re-parsing.
   The high-level ``netlist.load_vhdl`` exposes the same option and also accepts
   ``os.PathLike`` paths.
   Nested or unbalanced exclusion directives are errors.

   Concurrent ``with ... select`` assignments support static/grouped choices
   and ``others``, with the same coverage checks as case statements.

   Input port maps accept binary character, string, and binary/octal/hex
   bit-string literals, checked against the formal port type and width.
   Output literals and general expression/aggregate actuals remain unsupported.

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
   Whole output ports may also be omitted from named maps or left off the end
   of positional maps. Inputs without defaults must be associated; explicit
   ``open`` inputs remain unsupported. Individual formal elements or slices
   cannot be associated with ``open``, and partial associations must cover every
   element of the port.
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

   A single dynamically indexed whole-word clocked write site can infer an
   uninitialized array as an NLDB0 RAM primitive. Read registers remain explicit
   DFFs, preserving enables and old-data read/write collisions. Address reset
   leaves memory contents intact. See :doc:`loading` for inference limits and
   cases that retain register/mux lowering.

   The experimental two-state RTL subset supports constrained arrays, package
   array types and constant aggregates, binary/octal/hex literals,
   static slices and loops, nested ``for generate`` and static
   ``if``/``elsif``/``else generate`` statements containing assignments, clocked
   processes, and component/direct-entity instances. Generate-local signals,
   constants, arrays, records, and enumerations have separate bindings and nets per elaborated
   body; local arrays use register/mux lowering. Internal enum signals, record
   fields, and arrays preserve nominal type checking and explicit initialization;
   enum-valued ports are not yet supported. The subset also supports
   nested sequential cases with static choices, ranges, and ``others``, plus
   combinational processes with complete assignment coverage and explicit or
   ``all`` sensitivity. Explicit sensitivity entries accept nested record fields
   and validate coverage per signal bit. The subset includes integer-indexed
   ROM/RAM reads and clocked writes, and multiple clocked
   processes with synchronous reset/enable and constant asynchronous reset/set.
   Asynchronous state uses canonical NLDB0 DFFRN, DFFR, or DFFS primitives;
   bits omitted from the reset branch hold while reset is active. See
   :doc:`loading` for the supported reset idiom and restrictions.
   Both simple and structured processes
   accept ``rising_edge(clk)`` and ``clk'event and clk = '1'`` guards, including
   parentheses around the guard or its event/level operands. The event and level
   names must match the same scalar input clock in the sensitivity list; falling
   edges and extra Boolean conditions on the guard are rejected.
   Nonnegative constrained integer
   counters, ``std_logic_unsigned`` addition/subtraction and
   ``conv_integer(std_logic_vector)`` (with ``std_logic_arith`` imported) are
   supported. Component binding requires a matching visible declaration and
   supports named or positional ports (including static indices and slices)
   and integer/boolean generic specialization. Boolean defaults and actuals retain
   their type (integers are not implicitly converted), including across logical
   libraries. A generic can omit its default when its
   value is supplied by the instance. Architecture/package integer constants
   and one-dimensional integer constant tables are evaluated statically.
   Positional aggregates can constrain an otherwise unconstrained array type;
   inferred indices begin at the index subtype's left bound (0 for ``natural``,
   1 for ``positive``, and -2147483648 for the supported 32-bit ``integer``).
   Explicit bounds preserve their direction and are checked against the subtype.
   Constrained array aggregates accept positional elements or named static
   index/range choices (including grouped choices with ``|``), with an optional
   final ``others`` association. Positional and named index choices cannot mix.
   This also applies to nested arrays and integer constant tables.
   Index choices follow the declared bounds and direction; overlapping choices,
   missing coverage, and invalid association ordering are rejected. Named
   aggregates require explicit bounds rather than inferred unconstrained bounds.
   Boolean scalar/array objects retain their language type and use one hardware
   bit per element. Boolean array constant selections can drive scalar constant
   expressions and generate conditions; dynamic selections use read-only memory
   primitives. Nested and enumeration-indexed constants are supported.
   Architecture-local scalar functions can capture integer-indexed boolean
   arrays; static package-function array lookups remain unsupported. Intermediate
   integer addition/subtraction retains 32-bit width before assignment to a
   constrained hardware integer.
   Scalar-returning pure functions also accept named one-dimensional integer-array
   constants and forwarded array parameters. Array type identity, element subtypes,
   and constrained-formal lengths are checked; constrained formals map by position,
   while unconstrained formals retain actual bounds. Parameter indexing, bounds
   attributes, and ``'range`` loops are evaluated statically. Architecture-local
   functions capture preceding integer-array constants, including in defaults.
   Array-valued returns, local array variables, aggregate/slice actuals, and
   enumeration-indexed integer-array parameters remain unsupported.
   Hardware operates on legal subtype/index values; simulation bounds checks
   and nine-valued initialization are not implemented. Unsupported nonbinary literals,
   conflicting drivers, unsupported package bodies, and unsupported operations
   are rejected. ``numeric_std.unsigned`` and ``numeric_std.signed`` vector addition, subtraction,
   multiplication, equality/inequality, ordering comparisons and concatenation are supported, including
   mixed operand widths for arithmetic and comparisons. Multiplication produces
   the sum of operand widths; addition/subtraction produce their maximum width.
   Arithmetic and comparisons also accept integer operands on either side:
   static signed 32-bit integers or supported nonnegative hardware integers.
   Unsigned operations require natural operands. Scalar arithmetic converts the
   integer to the vector width, giving that width for addition/subtraction and
   twice that width for multiplication. Comparisons retain the full integer value.
   ``std_logic``/``std_ulogic`` literals preserve ``X`` and ``Z`` as SNL constant
   nets in scalar/vector assignments, aggregates, constants, input port actuals,
   mux inputs, and register data. ``bit`` remains binary; other nonbinary values
   and lowercase ``x``/``z`` are rejected. Explicit ``X``/``Z`` operands in
   arithmetic and comparisons are rejected. Register initialization and
   asynchronous reset parameters require binary values.
   Explicit binary signal initializers on locally clocked registers become DFF
   ``INIT`` parameters. Initialization on other drivers is rejected.
   Static ``numeric_std.to_unsigned(value, size)`` accepts natural values through
   2147483647 and sizes from 1 through 65536; values wider than the result retain
   their low bits, as specified by the conversion. Dynamic conversions and null
   vectors are rejected. Conversions between ``unsigned``, ``signed``, and
   ``std_logic_vector`` preserve bit positions. Signed arithmetic sign-extends
   operands; mixed signed/unsigned operands require explicit conversions.
   With ``std_logic_signed`` visible, vector addition, subtraction, multiplication
   and equality/inequality use signed operands;
   ambiguous numeric operations importing both signed and unsigned overloads
   are rejected.
   Component declarations can also appear in the architecture declarative part,
   and port declarations may explicitly specify the ``signal`` class.

   These features are also available through the high-level
   ``najaeda.netlist.load_vhdl`` loader.

   The raw ``NLDB`` Verilog, SystemVerilog, VHDL, and Liberty loaders report malformed
   Python arguments with standard :class:`TypeError` or :class:`ValueError`
   exceptions.  List-entry errors include the option name and index, such as
   ``files[1]`` or ``defines[0]``.  Native parser and elaboration failures
   remain ``RuntimeError`` subclasses; SystemVerilog failures use the more
   specific ``SystemVerilog*`` exception classes and expose structured
   diagnostic details where available.
   Verilog syntax errors include the failing input path and the parser's line
   and column range (positions in preprocessed text when preprocessing is enabled).

.. autoclass:: najaeda.naja.NLLibrary
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLDesign
   :members:
   :undoc-members:
   :inherited-members:

Design objects
~~~~~~~~~~~~~~

.. autosummary::

   SNLInstance
   SNLTerm
   SNLScalarTerm
   SNLBusTerm
   SNLBundleTerm
   SNLBitTerm
   SNLBusTermBit
   SNLNet
   SNLScalarNet
   SNLBusNet
   SNLBitNet
   SNLInstTerm
   SNLInstParameter
   SNLParameter
   NLClock
   SNLAttribute

.. autoclass:: najaeda.naja.SNLInstance
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLTerm
   :exclude-members: Direction
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLScalarTerm
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLBusTerm
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLBundleTerm
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLBitTerm
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLBusTermBit
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLNet
   :exclude-members: Type
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLScalarNet
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLBusNet
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLBitNet
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLInstTerm
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLInstParameter
   :members:
   :undoc-members:
   :inherited-members:

Use the raw API to declare model parameters and set instance values; these
operations do not currently have high-level ``najaeda.netlist`` wrappers.
The final value argument of ``createDecimal(design, name[, value])``,
``createString(design, name[, value])``,
``createBoolean(design, name[, value])``, and
``createBinary(design, name, size[, value])`` is optional. Omitting it creates
a required parameter with no default. The binary creator retains its existing
``size`` argument.

.. code-block:: python

   width = naja.SNLParameter.createDecimal(model, "WIDTH")
   assert not width.hasDefaultValue()
   naja.SNLInstParameter.create(instance, width, "8")

``hasDefaultValue()`` distinguishes an absent default from an explicit zero,
false, or empty string. ``getValue()`` returns the default as a string, or
``None`` if there is none. Instance values are obtained separately with
``SNLInstParameter.getValue()``. Cloning and NajaIF snapshots preserve default
presence, including empty string defaults.

Instances may be constructed before their required values are attached. Verilog
export rejects an instance missing a required value, and always emits supplied
values for required parameters. The current Verilog writer cannot emit a model
declaration without a default and reports an error for such declarations;
it does not emit SystemVerilog parameter port lists. A model supplied externally
may still be instantiated when its declaration is excluded from the export.
Parameter-derived truth tables require a model default. Changing an instance
parameter does not re-elaborate the model's ports or topology.

.. autoclass:: najaeda.naja.SNLParameter
   :members:
   :undoc-members:
   :inherited-members:

Clocks
~~~~~~

An ``NLClock`` is an SDC-style clock definition owned by the ``SNLDesign`` it
belongs to.  :meth:`najaeda.naja.SNLDesign.getClocks` lists the clocks of one
design, and :meth:`najaeda.naja.NLDB.getClocks` lists every clock in the
database.

``createPrimary`` follows SDC ``create_clock``: a period in a time unit, and
sources.  ``createGenerated`` follows ``createGenerated_clock``: its period is
``master period * divide_by / multiply_by``, and it is synchronous with its
master.  ``isSynchronousWith`` is true when two clocks share the same root
primary clock.

First-pass restrictions: sources and the master source must be top-level
terminals (``SNLBitTerm``) of the owning design.  Instance terminals are
rejected until instance-terminal occurrences are supported.  The master clock
must belong to the same design.  Clock names are unique within a design.

.. autoclass:: najaeda.naja.NLClock
   :members:
   :undoc-members:
   :inherited-members:

Attributes
~~~~~~~~~~

NajaIF schema 0.3.0 preserves ``SNLAttribute`` values on designs, terms, nets,
bus bits, instances, and instance terminals. Attribute order, duplicate names,
and string/number types survive ``NLDB.dumpNajaIF()`` / ``NLDB.loadNajaIF()``.
As in the live SNL model, empty string values and valueless attributes both have
``hasValue() == False``. Use the existing high-level ``get_attributes()`` methods
for routine queries; no raw API calls are needed to retain attributes in a
snapshot. Older snapshots must be regenerated from their original inputs.

.. autoclass:: najaeda.naja.SNLAttribute
   :members:
   :undoc-members:
   :inherited-members:

Hierarchy and connectivity
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. autosummary::

   SNLPath
   SNLOccurrence
   SNLEquipotential
   SNLNetComponent
   NLID
   LogicCone

.. autoclass:: najaeda.naja.SNLPath
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLOccurrence
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLEquipotential
   :exclude-members: Mode
   :members:
   :undoc-members:
   :inherited-members:

   Pass ``mode=SNLEquipotential.Mode.TraverseAssigns`` to the constructor
   to cross Assign instances and omit their instance terminals.  The
   default is ``SNLEquipotential.Mode.Standard``.

.. autoclass:: najaeda.naja.SNLNetComponent
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.NLID
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.LogicCone
   :members:
   :undoc-members:
   :inherited-members:

Enum-like values
~~~~~~~~~~~~~~~~

.. autosummary::

   SNLTerm.Direction
   SNLNet.Type
   SNLEquipotential.Mode
   SNLTermRole
   SNLActiveLevel

.. autoclass:: najaeda.naja.SNLTerm.Direction
   :members:
   :undoc-members:

.. autoclass:: najaeda.naja.SNLNet.Type
   :members:
   :undoc-members:

.. autoclass:: najaeda.naja.SNLEquipotential.Mode
   :members:
   :undoc-members:

``SNLNet.Type.AssignX`` and ``SNLNet.Type.AssignZ`` represent structural
four-state constants.  Use ``SNLNet.isConstantX()`` and
``SNLNet.isConstantZ()`` to distinguish them; ``isConstant()`` accepts all
four assign values while the zero and one predicates remain binary-only.

.. autoclass:: najaeda.naja.SNLTermRole
   :members:
   :undoc-members:
   :inherited-members:

.. autoclass:: najaeda.naja.SNLActiveLevel
   :members:
   :undoc-members:
   :inherited-members:

Module functions
~~~~~~~~~~~~~~~~

.. autosummary::

   getVersion
   getGitHash
   snapshotManifest
   setLogLevel
   addLogFile
   clearLogSinks
   installLoggingHandler
   log
   logInfo
   logWarn
   logCritical
   intentAvailable
   intentDescribe
   liveCompilation
   astSymbolOf
   snlObjectsOf
   intentParametersOf
   intentTypeOf
   intentPackageMember

.. autofunction:: najaeda.naja.getVersion
.. autofunction:: najaeda.naja.getGitHash

CMake and root source Bazel builds report the checkout's abbreviated Git
revision through ``getGitHash()``. Bazel collects it before build actions and
configures the version header with hermetic Python. Source archives without
Git use ``unknown`` unless an explicit fallback identity is supplied. Dependency
builds use their module-derived producer identity, never the consuming project's
workspace identity. Matching source builds can exchange snapshots when their
producer and primitive-layout identities agree; semantic intent still requires
source re-elaboration after cold reload.

``snapshotManifest(path)`` reads only a snapshot's ``snl.mf`` manifest;
it does not load the Cap'n Proto payload or create an ``NLUniverse``.  It
returns the schema version and the Naja producer version / Git hash.  At
present, loading a snapshot requires an exact match of both producer
values with the reader build; regenerate snapshots after changing builds.
The manifest also records ``D <db0-library-id-version>``. Loading requires
this canonical primitive library ID layout to match the reader; snapshots
without this record use the legacy layout and must be regenerated. This
layout version is independent of the wire schema version owned by ``naja-if``.

.. autofunction:: najaeda.naja.snapshotManifest
.. autofunction:: najaeda.naja.setLogLevel
.. autofunction:: najaeda.naja.addLogFile
.. autofunction:: najaeda.naja.clearLogSinks
.. autofunction:: najaeda.naja.installLoggingHandler
.. autofunction:: najaeda.naja.log
.. autofunction:: najaeda.naja.logInfo
.. autofunction:: najaeda.naja.logWarn
.. autofunction:: najaeda.naja.logCritical
.. autofunction:: najaeda.naja.intentAvailable
.. autofunction:: najaeda.naja.intentDescribe
.. autofunction:: najaeda.naja.liveCompilation
.. autofunction:: najaeda.naja.astSymbolOf
.. autofunction:: najaeda.naja.snlObjectsOf
.. autofunction:: najaeda.naja.intentParametersOf
.. autofunction:: najaeda.naja.intentTypeOf
.. autofunction:: najaeda.naja.intentPackageMember

Memory reset roles
------------------

For DB0 ``naja_mem__*`` primitives and other designs with a memory interface,
``SNLBitTerm.getRole()`` reports ``SNLTermRole.SyncReset`` for synchronous
resets and ``SNLTermRole.AsyncReset`` for asynchronous resets. Both satisfy
``isReset()``; synchronous resets also satisfy ``isSyncReset()``.
``getResetActiveLevel()`` returns ``SNLActiveLevel.Low`` for either active-low
reset mode and ``SNLActiveLevel.High`` for either active-high mode. With no
reset enabled, the reset pin has role ``Other`` and active level ``NA``.
The same queries on ``SNLInstTerm`` follow the model's bit term.

DB0 memory parameters ``RST_ENABLE``, ``RST_ASYNC``, ``RST_ACTIVE_LOW``,
``INIT_ENABLE``, and ``MULTI_CLOCK`` (when present) have Boolean type, as does
``SIGNED`` on DB0 division/remainder models. Their canonical values are the
strings ``"0"`` and ``"1"``. Verilog export emits numeric flags for DB0
primitives, while Boolean parameters on ordinary models retain the existing
``"FALSE"``/``"TRUE"`` string convention.

Word-level operator predicates
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``SNLDesign.isAddSub()`` identifies NLDB0 addition and subtraction models;
``isSubtract()`` distinguishes subtraction. ``isMultiply()``, ``isShift()``,
and ``isDivMod()`` identify multiplication, shifts, and division/modulo.
``isShiftRight()`` and ``isShiftArithmetic()`` expose shift direction and
arithmetic mode. All seven return booleans and return ``False`` for unrelated
models, including Liberty cells, gates, blackboxes, and hierarchical designs.
The shift-detail predicates also return ``False`` for invalid shift metadata.

Use the raw predicates when a viewer needs shift direction or arithmetic mode.
For instance classification, the high-level API provides ``Instance.is_add()``,
``is_sub()``, ``is_mul()``, ``is_shift()``, and ``is_divmod()``.
``is_add()`` excludes subtraction.

Gate predicates and truth-table errors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``SNLDesign.isConst0()``, ``isConst1()``, ``isConst()``, ``isInv()``,
``isBuf()``, and the gate-family predicates return ``False`` when a model
has per-output truth tables instead of a single truth table (for example,
a full adder). ``getTruthTable()`` requires a single truth table; native
errors raise ``RuntimeError`` with the native diagnostic preserved.
Use ``getTruthTableByOutputID()`` to query an individual output. Its native
errors also raise ``RuntimeError``.


Language-neutral source references
----------------------------------

``SNLDesign`` and all ``SNLDesignObject`` subclasses (instances, nets, terms and
bits) expose the following expert APIs. These require no retained source or AST.

.. list-table:: Source reference methods
   :header-rows: 1

   * - Method
     - Contract
   * - ``hasSourceReferences()``
     - Allocation-free presence check for a declaration or origins.
   * - ``getSourceDeclaration()``
     - Reference for this object's declaration, or ``None``.
   * - ``getSourceOrigins()``
     - List of upstream references, in insertion order.

A reference is ``((file, line, column, end_line, end_column), provider, language,
representation)``. All identifiers are strings; empty strings mean unknown.
Lines and columns fit unsigned 32-bit integers (maximum 4,294,967,295). Coordinates
are one-based byte positions with inclusive ends; zero means unknown. Paths are
kept verbatim, and language/representation are never inferred from a path or src
attribute. References are populated by native loaders and are read-only in Python.
For example::

   declaration = gate.getSourceDeclaration()
   origins = gate.getSourceOrigins()

The instance's declaration identifies its instantiation; query
``instance.getModel().getSourceDeclaration()`` for its model definition.
``hasSourceLoc()`` and ``getSourceLoc()`` retain their independent legacy meaning;
new roles neither reclassify nor overwrite that slot. SystemVerilog loading
dual-writes the legacy slot and typed declarations for module definitions, ports,
named signals and source instantiations; lowered logic gets origins labelled
``slang`` / ``systemverilog`` / ``rtl``. Retaining an AST is optional.
VHDL loading populates the same roles, labelled ``naja-vhdl`` / ``vhdl`` /
``rtl``. Designs point to entity declarations; their origins identify architecture
bodies. Explicit instances point to instantiations, and generated logic points
to expressions or processes. High-level query wrappers are documented in
:doc:`api`. Python does not expose source metadata mutation APIs.

References are deep-copied by supported SNL clones and preserved by NajaIF,
including bus-term bits and instance terminals. Legacy locations remain
independent from declarations and origins after loading. Metadata-enabled
Verilog export does not synthesize source-reference carriers from declarations,
origins, bus-bit references, or instance-terminal references. Use NajaIF for source
metadata persistence. Existing raw attributes are exported normally, and legacy
source locations retain their established compact/verbose export behavior.

Custom source-reference attributes have no special import or export handling.
Native loaders populate the source queries used by viewers such as naja-schematic.


Source coordinate width
~~~~~~~~~~~~~~~~~~~~~~~

Both legacy ``getSourceLoc()`` and new source-reference ranges support 32-bit
columns; their Python tuple shapes are unchanged. Native C++ consumers must
rebuild for the widened ``SNLSourceLoc`` layout. SystemVerilog loading and
legacy source-location Verilog export preserve columns above 65,535.

NajaIF schemas since 0.2.0 preserve the full coordinate width;
the original 16-bit fields retain their wire types. The manifest revision is
now sourced from naja-if. Snapshot loading still requires an exact schema and
producer match, so snapshots from earlier revisions must be regenerated.


Source presence and legacy population
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``object.hasSourceReferences()`` to compute viewer source-availability flags
without constructing reference tuples or lists. It checks declarations and
origins. The independent legacy location is excluded.
This matches C++ ``SNLRTLInfos::hasSourceReferences()``.
The check concerns the queried object; it does not traverse descendants or fall
back from an instance to its model definition.

Source locations and references are populated by native loaders and restored
from NajaIF. Python exposes query methods only.


Term-role compatibility
~~~~~~~~~~~~~~~~~~~~~~~

Raw ``SNLBitTerm`` (scalar terms and bus bits) and ``SNLInstTerm`` expose canonical
camelCase predicates such as ``isClock()`` and ``isAsyncReset()``. The pre-0.7.29
snake_case spellings remain compatibility aliases: ``is_clock``, ``is_async_reset``,
``is_async_set``, ``is_sync_reset``, ``is_sync_set``, ``is_reset``, ``is_enable``,
``is_data``, ``is_data_input`` and ``is_data_output``. New raw callers should use
camelCase. Each alias emits ``DeprecationWarning`` and will be removed in a
future release; no removal version is scheduled. Python normally hides this
warning in imported code; use ``-Wd`` to display it. The high-level ``Term`` API
keeps its snake_case convention and is not deprecated.

Structural Verilog provenance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``NLDB.loadGateLevelVerilog(files, keep_assigns=True, blackbox_unknown_modules=False,
preprocess_enabled=False, conflicting_design_name_policy=None,
yosys_source_attributes=False)`` appends an explicit Yosys provenance option.
Existing positional arguments keep their meanings. Use the high-level
``GateLevelVerilogConfig(yosys_source_attributes=True)`` for normal loading workflows.
Experts can use the raw option with ``getSourceDeclaration()`` and
``getSourceOrigins()`` directly; no extra high-level source wrapper is needed.

Unpreprocessed structural inputs populate declarations (provider
``naja-verilog``, language ``verilog``, unknown representation) without touching
the legacy source slot. Instance declarations and model header declarations
remain separate. With the option enabled, string ``src`` attributes add ordered,
exactly deduplicated Yosys origins. Their language and representation remain
empty strings; generic attributes are opaque by default. Numeric suffixes are
parsed after the last colon; absent/zero coordinates remain unknown, invalid or
overflowing entries warn and are skipped independently. Raw attribute values
remain available unchanged. See :doc:`loading` for span and persistence limits.
