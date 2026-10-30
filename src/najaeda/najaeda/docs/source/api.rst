High-Level API Reference
========================

This is the recommended Python API for scripts and applications.  It wraps
the native SNL database with path-aware Python objects and provides safer
editing behavior for hierarchical designs.

``Net.Type.ASSIGNX`` and ``Net.Type.ASSIGNZ`` preserve structural X and Z
constants.  Query them with ``Net.is_constx()`` and ``Net.is_constz()``;
``Net.is_const()`` covers all four constant values.

For the lower-level compiled extension module, see :doc:`raw_api`.
For the optional ``najaeda.schematic`` viewer API, see :doc:`schematic`.

``load_gate_level_verilog`` and ``GateLevelVerilogConfig`` load gate-level (structural) Verilog
netlists. For Verilog or SystemVerilog RTL, including ``.v`` files, use
``load_system_verilog`` and ``SystemVerilogConfig`` within that frontend's
supported subset. ``Instance.dump_gate_level_verilog`` and ``GateLevelVerilogDumpConfig`` export
structural netlists, including after RTL elaboration; they do not reconstruct
the original behavioral RTL. See :doc:`loading`.

Lowering efficiency audit
-------------------------

``Instance.get_lowering_efficiency_report(warn_threshold=256)`` (also available
as ``najaeda.stats.analyze_lowering_efficiency(instance, warn_threshold=256)``)
inspects reachable model connectivity independently of the loading frontend.
It returns a JSON-compatible dictionary with model-local and hierarchy-expanded
mux instance/bit counts, and per-model findings grouped by code and first source
origin (or ``None`` when unavailable). Codes are ``identical_mux_inputs``,
``duplicate_mux``, ``repeated_mux_arm`` (adjacent selections choosing the same
true or false value, with the inner vector feeding only the corresponding
outer data pins), and ``unconsumed_mux``. A source is a five-element sequence
``(file, line, column, end_line, end_column)``. Model rows include their stable
``model_id`` and hierarchy ``occurrences``; findings contain ``instances``,
``bits`` and ``expanded_bits``.

Warnings use the ``najaeda.stats`` logger once per model/code when the expanded
avoidable bit count reaches the threshold. Pass a positive integer to adjust
it, or ``None`` to collect results silently. The audit does not modify the
netlist. Categories overlap and must not be summed. Non-mux consumers are
conservative live roots: this is an optimization audit, not comprehensive dead
logic analysis or a correctness check. Large necessary mux networks alone do
not produce warnings. Recursive model hierarchies are rejected. The PULP runner
runs this audit automatically and saves ``lowering-efficiency.json``.

The RTL loaders combine exclusive, complete-vector mux chains selecting
the same true value (using OR) or false value (using AND). Shared, reordered,
partially connected and feedback paths are retained. This preserves the
canonical mux network's X/Z behavior; it does not extend the frontend's supported
language semantics. Source origins from both selections are retained on the
surviving mux and new selector logic, including across NajaIF round trips.

Source information
------------------

``Net``, ``Term`` and ``Instance`` provide the allocation-free
``has_source_references()`` presence check. It checks for a source declaration
or source origins, matching the explicit reference getters.
Instance checks apply to the instantiation; query its model explicitly for a
model definition. Concatenated nets return ``False``.

These classes also provide ``get_source_declaration()`` and
``get_source_origins()``. The first returns an immutable ``SourceReference``
or ``None``; the second returns a list in insertion order with exact duplicates
removed. ``SourceReference.range`` is a ``SourceRange`` (``file``, ``line``,
``end_line``, ``column``, ``end_column``). Optional ``provider``, ``language`` and
``representation`` strings are ``None`` when unknown. Paths are opaque; language
and representation are never inferred from extensions. Coordinates are one-based
byte positions with inclusive endpoints; zero means unknown. Lines and columns
support values through 4,294,967,295. ``get_source_range()`` also preserves these
wider columns without changing its return shape. NajaIF snapshots preserve the
full coordinate width, declarations, origins, and qualifiers. Verilog legacy
source attributes also support the full coordinate width.

An instance declaration identifies its instantiation in the loaded input.
``Instance.get_model_source_declaration()`` explicitly queries the model
definition (for the top instance, its top design). Origins identify upstream
inputs, potentially several RTL locations for a synthesized gate. No retained
text or AST is needed. Concatenated nets have no declaration or origins.

``get_source_range()`` continues to return the independent ``SourceLoc`` range.
It does not fall back to declarations or origins. SystemVerilog loading also
populates declarations for module definitions,
ports, named signals and source instantiations. Lowered logic receives origins
with provider ``slang``, language ``systemverilog`` and representation ``rtl``.
These queries work with ``keep_ast_link=False``. Structural Verilog loading
populates declarations and can decode Yosys origins explicitly; see :doc:`loading`.
VHDL population remains pending. Python source access is read-only.

New references support in-memory access, cloning, and NajaIF snapshot round trips.
The snapshot schema revision is owned by naja-if; incompatible revisions and
producer identities are rejected before loading payloads. Earlier snapshots must
be regenerated. Verilog export does not encode the new declarations and origins as attributes.
Use NajaIF to preserve these references. Source loading and queries are available
for viewer integration, including naja-schematic. Existing raw Verilog attributes
and legacy source-attribute export retain their established behavior.

Term roles
----------

Scalar terms and individual bus bits provide ``get_role()`` and
``get_reset_active_level()``, returning raw ``SNLTermRole`` and
``SNLActiveLevel`` values. ``is_clock()``, ``is_async_reset()``,
``is_async_set()``, ``is_sync_reset()``, ``is_sync_set()``, ``is_reset()``,
``is_enable()``, ``is_data_input()``, ``is_data_output()``, and ``is_data()``
classify the resolved role. ``is_reset()`` includes reset roles only;
use the set predicates for set pins. Whole-bus queries raise ``ValueError``;
use ``term.get_bit(index)`` or iterate ``term.get_bits()``.

Instance terms resolve parameter-dependent roles in their path context;
top-level terms use static roles. Configure vendor models with the raw
``SNLDesign.setRolesFromParameters()`` API described in :doc:`raw_api`.
For example, ``ff.get_term("R").is_reset()`` returns false when the selected
configuration explicitly assigns no role to ``R``. ``get_role()`` then
returns ``SNLTermRole.Other``.

.. automodule:: najaeda.netlist
    :members:
    :undoc-members:
    :show-inheritance:

``GateLevelVerilogConfig.yosys_source_attributes`` defaults to ``False``. Set it to
``True`` to interpret Yosys ``src`` attributes as upstream origins without
inferring a language, representation, or AST link. Structural Verilog now
populates declarations independently of this option; see :doc:`loading`.
