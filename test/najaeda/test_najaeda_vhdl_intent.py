# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""VHDL semantic intent contract, exercised exclusively through raw bindings."""

from pathlib import Path
import tempfile
import unittest
import warnings

from najaeda import naja


PACKAGE = """
package types_pkg is
  type phase_t is (resting, working, finished);
  subtype phase_alias is phase_t;
end package;
"""

SOURCE = """
use work.types_pkg.all;
entity leaf is
  generic (width : positive := 3; extra : natural := width + 2);
  port (a : in bit_vector(0 to width-1); phase : in phase_alias;
        y : out bit_vector(width-1 downto 0); phase_out : out phase_t);
end;
use work.types_pkg.all;
architecture rtl of leaf is
  constant limit_value : natural := extra + 1;
  subtype word_t is bit_vector(width-1 downto 0);
  type pair_t is record flag : bit; data : word_t; end record;
  signal pair_value : pair_t;
  signal current_phase : phase_alias;
begin
  pair_value.flag <= '1'; pair_value.data <= a;
  y <= pair_value.data; current_phase <= phase; phase_out <= current_phase;
end;
entity top is port(a : in bit_vector(5 downto 0); y : out bit_vector(5 downto 0)); end;
use work.types_pkg.all;
architecture rtl of top is
  signal p, q : phase_t;
begin
  p <= resting;
  first: entity work.leaf generic map(width => 2) port map(a(1 downto 0), p, y(1 downto 0), q);
  second: entity work.leaf generic map(width => 4) port map(a(5 downto 2), p, y(5 downto 2), open);
end;
"""


class VHDLIntentTests(unittest.TestCase):
    def setUp(self):
        naja.NLUniverse.create()
        self.db = naja.NLDB.create(naja.NLUniverse.get())
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.package = self.path("types.vhd", PACKAGE)
        self.source = self.path("design.vhd", SOURCE)

    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()

    def path(self, name, text):
        path = Path(self.directory.name) / name
        path.write_text(text)
        return str(path)

    def load(self, files=None, retain=True, **kwargs):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            return self.db.loadVHDL(
                files=files or [self.source, self.package], keep_ast_link=retain,
                diagnostics_report_path=None, **kwargs)

    @staticmethod
    def declarations(obj):
        return {item["name"]: item for item in naja.intentDescribe(obj)["declarations"]}

    def test_enums_records_ranges_and_specializations(self):
        top = self.load(top="top")
        self.assertTrue(naja.intentAvailable())
        first = top.getInstance("first")
        second = top.getInstance("second")
        first_info = naja.intentDescribe(first)
        second_info = naja.intentDescribe(second)
        self.assertNotEqual(first_info["context"], second_info["context"])
        self.assertNotEqual(first_info["model_context"], second_info["model_context"])
        for instance, width in [(first, 2), (second, 4)]:
            model = instance.getModel()
            declarations = self.declarations(model)
            self.assertEqual(width, declarations["width"]["effective_value"])
            self.assertTrue(declarations["width"]["overridden"])
            self.assertEqual(3, declarations["width"]["default_value"])
            self.assertFalse(declarations["extra"]["overridden"])
            self.assertEqual(width + 2, declarations["extra"]["effective_value"])
            limit = declarations["limit_value"]
            self.assertEqual(width + 3, limit["effective_value"])
            dependency = limit["expression"]["dependencies"][0]
            self.assertEqual("resolved", dependency["status"])
            self.assertEqual(declarations["extra"]["id"], dependency["declaration_id"])
            enum = naja.intentTypeOf(model.getNet("current_phase"))
            self.assertEqual("enumeration", enum["kind"])
            self.assertEqual(["resting", "working", "finished"], [m["name"] for m in enum["literals"]])
            self.assertEqual([0, 1, 2], [m["representation"] for m in enum["literals"]])
            self.assertEqual(2, enum["width"])
            alias = naja.intentDescribe(model.getTerm("phase"))["declared_type"]
            self.assertEqual("phase_alias", alias["name"])
            self.assertEqual(declarations["phase_alias"]["id"], alias["declaration_id"])
            self.assertEqual("package", enum["declaration"]["owner_kind"])
            a = naja.intentTypeOf(model.getTerm("a"))
            y = naja.intentTypeOf(model.getTerm("y"))
            self.assertEqual({"left": 0, "right": width-1, "direction": "to", "length": width,
                              "index_kind": "integer"}, a["ranges"][0])
            self.assertEqual("downto", y["ranges"][0]["direction"])
            record = naja.intentTypeOf(model.getNet("pair_value"))
            self.assertEqual(["flag", "data"], [f["name"] for f in record["fields"]])
            self.assertEqual(width, record["fields"][1]["type"]["width"])
            self.assertEqual(width+1, record["width"])
            self.assertEqual("vhdl", naja.intentParametersOf(instance)["language"])

    def test_package_only_incremental_and_cached_load(self):
        self.assertIsNone(self.load(files=[self.package]))
        info = naja.intentDescribe(self.db.getLibrary("DESIGN"))
        self.assertEqual("package_declarations", info["analysis"])
        self.assertEqual("phase_t", info["declarations"][0]["name"])
        top = self.load(files=[self.source], top="top")
        self.assertEqual("available", naja.intentDescribe(top)["status"])
        self.load(files=[self.source], top="top")
        self.assertEqual("unavailable", naja.intentDescribe(top)["status"])

    def test_disabled_failure_destroy_and_snapshot(self):
        top = self.load(top="top")
        saved = naja.intentDescribe(top)
        bad = self.path("invalid.vhd", "entity")
        with self.assertRaises(RuntimeError):
            self.load(files=[bad])
        self.assertFalse(naja.intentAvailable())
        self.assertEqual("unavailable", naja.intentDescribe(top)["status"])
        self.assertEqual("available", saved["status"])
        self.load(top="top", retain=False)
        self.assertFalse(naja.intentAvailable())
        snapshot = str(Path(self.directory.name) / "snapshot")
        self.db.dumpNajaIF(snapshot)
        naja.NLUniverse.get().destroy()
        db = naja.NLDB.loadNajaIF(snapshot)
        self.assertEqual("unavailable", naja.intentDescribe(db.getTopDesign())["status"])
        self.assertFalse(naja.intentAvailable())
        naja.NLUniverse.get().destroy()
        naja.NLUniverse.create()
        self.db = naja.NLDB.create(naja.NLUniverse.get())
        top = self.load(top="top")
        self.assertEqual("available", naja.intentDescribe(top)["status"])
        top.destroy()
        with self.assertRaises(RuntimeError):
            naja.intentDescribe(top)

    def test_libraries_work_shadowing_and_extended_identifiers(self):
        support = self.path("support.vhd", """
package base_pkg is
  type state_t is (empty_state, full_state);
  constant amount : natural := 5;
end;
use work.base_pkg.all;
package relay_pkg is
  subtype state_alias is state_t;
  constant doubled : natural := amount * 2;
end;
""")
        self.assertIsNone(self.load(files=[support], library="Support"))
        design = self.path("scopes.vhd", r"""
library support; use support.relay_pkg.all; use support.base_pkg.all;
entity scoped is port(a : in bit; y : out bit); end;
library support; use support.relay_pkg.all; use support.base_pkg.all;
architecture rtl of scoped is
  constant amount_local : natural := doubled + 1;
  signal \Flag\, \flag\ : bit;
  signal state : state_alias;
begin
  \Flag\ <= a; \flag\ <= not a; state <= empty_state;
  process(all)
    constant amount_local : natural := 2;
    constant nested_value : natural := amount_local + 1;
  begin
    if nested_value = 3 then y <= \Flag\; else y <= \flag\; end if;
  end process;
end;
""")
        top = self.load(files=[design], top="scoped")
        items = naja.intentDescribe(top)["declarations"]
        doubled = next(d for d in items if d["name"] == "doubled")
        self.assertEqual("Support", doubled["library"])
        self.assertEqual(10, doubled["effective_value"])
        definitions = [d for d in items if d["name"] == "amount_local"]
        self.assertEqual(2, len(definitions))
        self.assertNotEqual(definitions[0]["id"], definitions[1]["id"])
        nested = next(d for d in items if d["name"] == "nested_value")
        self.assertEqual(definitions[1]["id"], nested["expression"]["dependencies"][0]["declaration_id"])
        upper = naja.intentDescribe(top.getNet("\\Flag\\"))
        lower = naja.intentDescribe(top.getNet("\\flag\\"))
        self.assertNotEqual(upper["canonical_name"], lower["canonical_name"])
        self.assertNotEqual(upper["id"], lower["id"])
        state = naja.intentTypeOf(top.getNet("state"))
        self.assertEqual("Support", state["declaration"]["library"])

    def test_nominal_types_and_user_array(self):
        path = self.path("nominal.vhd", """
entity nominal is generic(width : positive := 3); port(a : in bit; y : out bit); end;
architecture rtl of nominal is
  type first_t is (zero_a, one_a, two_a);
  type second_t is (zero_b, one_b, two_b);
  type indices_t is (slot_a, slot_b);
  type matrix_t is array(indices_t) of bit;
  signal matrix : matrix_t;
  type words_t is array(2 to 3) of bit_vector(width-1 downto 0);
  subtype count_t is natural range 0 to 7;
  signal first : first_t;
  signal second : second_t;
  signal words : words_t;
  signal count : count_t;
begin
  first <= zero_a; second <= zero_b; count <= 2;
  words(2) <= (others => a); words(3) <= (others => a);
  matrix(slot_a) <= a; matrix(slot_b) <= not a;
  y <= words(2)(0) xor matrix(slot_b);
end;
""")
        top = self.load(files=[path], top="nominal")
        first = naja.intentTypeOf(top.getNet("first"))
        second = naja.intentTypeOf(top.getNet("second"))
        self.assertEqual(first["width"], second["width"])
        self.assertNotEqual(first["declaration"]["id"], second["declaration"]["id"])
        words = next(d for d in naja.intentDescribe(top)["declarations"] if d["name"] == "words")
        shape = words["declared_type"]["resolved"]
        self.assertEqual(6, shape["width"])
        self.assertEqual("to", shape["ranges"][0]["direction"])
        self.assertEqual("downto", shape["element_type"]["ranges"][0]["direction"])
        count = naja.intentDescribe(top.getNet("count"))["declared_type"]
        self.assertEqual(7, count["subtype_constraint"]["right"])
        self.assertEqual(3, count["resolved"]["width"])
        matrix = next(d for d in naja.intentDescribe(top)["declarations"] if d["name"] == "matrix")
        bounds = matrix["declared_type"]["resolved"]["ranges"][0]
        self.assertEqual("enumeration", bounds["index_kind"])
        self.assertEqual("indices_t", bounds["index_type"]["name"])

    def test_record_field_types_bind_at_definition(self):
        path = self.path("record_scopes.vhd", """
entity record_scopes is port(a : in bit; y : out bit); end;
architecture rtl of record_scopes is
  subtype word_t is bit_vector(2 downto 0);
  type outer_t is record data : word_t; end record;
begin
  process(all)
    subtype word_t is bit_vector(4 downto 0);
    type inner_t is record item : outer_t; end record;
    variable value : inner_t;
  begin
    value.item.data := (others => a);
    y <= value.item.data(0);
  end process;
end;
""")
        top = self.load(files=[path], top="record_scopes")
        declarations = naja.intentDescribe(top)["declarations"]
        words = [d for d in declarations if d["name"] == "word_t"]
        inner = next(d for d in declarations if d["name"] == "inner_t")
        field = inner["resolved_type"]["fields"][0]["type"]["fields"][0]
        self.assertEqual(words[0]["id"], field["declared_type"]["declaration_id"])
        self.assertNotEqual(words[1]["id"], field["declared_type"]["declaration_id"])
        self.assertEqual(3, field["type"]["width"])

    def test_generic_boolean_string_and_vector_values(self):
        path = self.path("domains.vhd", """
entity value_leaf is
  generic(flag : boolean := true; label_text : string := "unit";
          bits : bit_vector(2 downto 0) := "101");
  port(a : in bit; y : out bit);
end;
architecture rtl of value_leaf is begin
  y <= a xor bits(0) when flag else a;
end;
entity domains is port(a : in bit; y : out bit_vector(1 downto 0)); end;
architecture rtl of domains is begin
  first: entity work.value_leaf port map(a, y(0));
  second: entity work.value_leaf generic map(flag => false, label_text => "changed", bits => "010")
    port map(a, y(1));
end;
""")
        top = self.load(files=[path], top="domains")
        first = self.declarations(top.getInstance("first"))
        second = self.declarations(top.getInstance("second"))
        self.assertIs(True, first["flag"]["effective_value"])
        self.assertIs(False, second["flag"]["effective_value"])
        self.assertIs(True, second["flag"]["default_value"])
        self.assertEqual("unit", second["label_text"]["default_value"])
        self.assertEqual("changed", second["label_text"]["effective_value"])
        self.assertEqual("101", first["bits"]["default_value"])
        self.assertEqual("010", second["bits"]["effective_value"])
        self.assertEqual("unsupported_without_additional_elaboration", second["bits"]["default_value_status"])
        self.assertEqual(3, second["bits"]["declared_type"]["resolved"]["width"])

    def test_package_array_and_record_declarations(self):
        path = self.path("package_shapes.vhd", """
package shapes is
  type words_t is array(natural range <>) of bit_vector(1 downto 0);
  type pair_t is record flag, valid : bit; data : words_t(0 to 1); end record;
end;
""")
        self.assertIsNone(self.load(files=[path]))
        declarations = self.declarations(self.db.getLibrary("DESIGN"))
        words = declarations["words_t"]
        self.assertEqual("bit_vector", words["element_type"]["name"])
        self.assertEqual("unavailable_without_elaboration", words["resolution_status"])
        pair = declarations["pair_t"]
        self.assertEqual(["flag", "valid", "data"], [f["name"] for f in pair["fields"]])
        self.assertEqual("words_t", pair["fields"][2]["declared_type"]["name"])
        self.assertEqual("unavailable_without_elaboration", pair["resolution_status"])

    def test_constant_domains_and_integer_table_types(self):
        path = self.path("constants.vhd", """
entity constants is port(y : out bit_vector(3 downto 0)); end;
architecture rtl of constants is
  type fixed_t is array(3 downto 2) of integer;
  type table_t is array(natural range <>) of integer;
  type bits_t is array(natural range <>) of bit;
  constant fixed_values : fixed_t := (4, -2);
  constant values : table_t(0 to 1) := (2, 5);
  constant flag : boolean := false;
  constant bits : bit_vector(3 downto 0) := "1010";
  constant message_text : string := "example";
begin y <= bits; end;
""")
        declarations = self.declarations(self.load(files=[path], top="constants"))
        self.assertIs(False, declarations["flag"]["effective_value"])
        self.assertEqual("1010", declarations["bits"]["effective_value"])
        self.assertEqual("bits_in_declaration_order", declarations["bits"]["value_format"])
        self.assertEqual(4, declarations["bits"]["resolved_type"]["width"])
        for name in ["message_text", "values", "fixed_values"]:
            self.assertEqual("unsupported", declarations[name]["value_status"])
        for name in ["fixed_t", "table_t"]:
            self.assertEqual("static_integer_table", declarations[name]["resolution_status"])
            self.assertEqual("integer", declarations[name]["element_type"]["name"])
        self.assertEqual({"left": 3, "right": 2, "direction": "downto"},
                         declarations["fixed_t"]["index_range"])
        self.assertNotIn("index_range", declarations["table_t"])
        self.assertEqual("unconstrained", declarations["bits_t"]["resolution_status"])

    def test_overridden_unevaluable_scalar_defaults(self):
        path = self.path("defaults.vhd", """
entity defaults_leaf is generic(n : integer := -(1 / 0)); port(y : out bit); end;
architecture rtl of defaults_leaf is begin y <= '1'; end;
entity defaults is port(y : out bit); end;
architecture rtl of defaults is begin
  child: entity work.defaults_leaf generic map(n => 2) port map(y);
end;
""")
        top = self.load(files=[path], top="defaults")
        generic = self.declarations(top.getInstance("child"))["n"]
        self.assertEqual(2, generic["effective_value"])
        self.assertTrue(generic["overridden"])
        self.assertEqual("unavailable", generic["default_value_status"])
        self.assertNotIn("default_value", generic)

    def test_required_generic_has_no_default_expression(self):
        path = self.path("required_generic.vhd", """
entity required_leaf is generic(n : positive); port(y : out bit); end;
architecture rtl of required_leaf is begin y <= '1'; end;
entity required_generic is port(y : out bit); end;
architecture rtl of required_generic is begin
  child: entity work.required_leaf generic map(n => 2) port map(y);
end;
""")
        top = self.load(files=[path], top="required_generic")
        generic = self.declarations(top.getInstance("child"))["n"]
        self.assertEqual(2, generic["effective_value"])
        self.assertTrue(generic["overridden"])
        self.assertEqual("absent", generic["default_value_status"])
        self.assertIsNone(generic["default_expression"])
        self.assertNotIn("default_value", generic)

    def test_functions_in_processes_and_generate_alternatives(self):
        path = self.path("functions.vhd", """
entity functions is port(a : in bit; y : out bit); end;
architecture rtl of functions is begin
  choice: if false generate
    process(all)
      function unused(n : integer) return integer is begin return n; end;
    begin y <= not a; end process;
  else generate
    process(all)
      function increment(n : integer) return integer is begin return n + 1; end;
      constant amount : integer := increment(1);
    begin if amount = 2 then y <= a; else y <= not a; end if; end process;
  end generate;
end;
""")
        top = self.load(files=[path], top="functions")
        amount = self.declarations(top)["amount"]
        self.assertEqual(2, amount["effective_value"])
        self.assertEqual(["process"], [s["kind"] for s in amount["lexical_scopes"]])

    def test_type_declaration_diagnostics(self):
        cases = [
            ("type index_t is (first_slot, last_slot); "
             "type table_t is array(index_t) of integer;",
             "enumeration-indexed integer constant tables are not yet supported"),
            ("type pair_t is record flag : bit; end record; "
             "type pair_t is record data : bit; end record;",
             "duplicate or shadowing record type: pair_t"),
            ("type large_t is (" + ",".join(f"item_{i}" for i in range(4097)) + ");",
             "enumeration must contain 1 to 4096 identifier literals"),
        ]
        for declarations, diagnostic in cases:
            with self.subTest(diagnostic=diagnostic):
                naja.NLUniverse.get().destroy()
                naja.NLUniverse.create()
                self.db = naja.NLDB.create(naja.NLUniverse.get())
                path = self.path("invalid_types.vhd", "entity invalid_types is end; "
                                 "architecture rtl of invalid_types is " + declarations + " begin end;")
                with self.assertRaisesRegex(RuntimeError, diagnostic):
                    self.load(files=[path], top="invalid_types")

    def test_explicit_unsupported_dependencies_and_failed_arguments(self):
        path = self.path("dependencies.vhd", """
entity dependencies is port(a : in bit; y : out bit); end;
architecture rtl of dependencies is
  function increase(value : natural) return natural is
  begin return value + 1; end;
  constant size_value : natural := increase(2);
begin
  y <= a when size_value = 3 else not a;
end;
""")
        top = self.load(files=[path], top="dependencies")
        constant = self.declarations(top)["size_value"]
        self.assertEqual(3, constant["effective_value"])
        self.assertIn("unsupported", {d["status"] for d in constant["expression"]["dependencies"]})
        with self.assertRaises(TypeError):
            self.db.loadVHDL(42, keep_ast_link=True)
        self.assertEqual("unavailable", naja.intentDescribe(top)["status"])
        self.assertFalse(naja.intentAvailable())
        with self.assertRaises(RuntimeError):
            naja.intentDescribe(None)

    def test_retention_has_identical_snapshot_and_live_objects(self):
        results = []
        for retain in [False, True]:
            if results:
                naja.NLUniverse.get().destroy()
                naja.NLUniverse.create()
                self.db = naja.NLDB.create(naja.NLUniverse.get())
            top = self.load(top="top", retain=retain)
            live = [(d.getID(), d.getName(), [(n.getNLID().toTuple(), n.getName()) for n in d.getNets()])
                    for d in self.db.getLibrary("DESIGN").getSNLDesigns()]
            snapshot = Path(self.directory.name) / str(retain)
            self.db.dumpNajaIF(str(snapshot))
            artifacts = {p.name: p.read_bytes() for p in snapshot.iterdir() if p.is_file()}
            results.append((live, artifacts))
        self.assertEqual(*results)


if __name__ == "__main__":
    unittest.main()
