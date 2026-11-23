# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import gzip
import re
import shutil
import tempfile
import time
import threading
import unittest
import warnings
import zipfile
import naja
import faulthandler 

INTENT_MINI_SV = """\
package mini_pkg;
  typedef logic [7:0] byte_t;
  typedef enum logic [1:0] {
    ST_IDLE = 2'b00,
    ST_RUN  = 2'b01,
    ST_DONE = 2'b11
  } state_e;
  typedef struct packed {
    logic   valid;
    byte_t  data;
    state_e state;
  } payload_t;
  typedef union packed {
    logic [7:0] raw;
    byte_t      data;
  } overlay_t;
  localparam int PLEN = (32 == 32) ? 34 : 56;
endpackage

module intent_mini #(
    parameter int DEPTH = 4
) (
    input  logic clk,
    input  logic rst_n
);
  import mini_pkg::*;
  localparam int IDX_W = $clog2(DEPTH);

  mini_pkg::state_e state_q;
  byte_t             byte_q;
  logic [3:0]        plain_q;
  payload_t          payload_q;
  overlay_t          overlay_q;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) state_q <= ST_IDLE;
    else        state_q <= ST_RUN;
  end
endmodule
"""

class SNLDBTest(unittest.TestCase):
  def _load_specializations(self, language, source, reserved=None, **sv_options):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    if reserved:
      library = naja.NLLibrary.create(db, "DESIGN")
      naja.SNLDesign.create(library, reserved)
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "synthetic." + language)
      with open(path, "w") as stream:
        stream.write(source)
      if language == "sv":
        top = db.loadSystemVerilog([path], diagnostics_report_path=None, **sv_options)
      else:
        top = db.loadVHDL([path], top="top")
    return db, top

  def test_selected_top_retains_source_name_with_parameter_overrides(self):
    source = """
module tile #(parameter int WIDTH=3)(input [WIDTH-1:0] a,output [WIDTH-1:0] y);
  assign y=a;
endmodule
module top #(parameter int WIDTH=3, parameter logic [255:0] MASK=0)
            (input [WIDTH-1:0] a,output [WIDTH-1:0] y);
  tile #(.WIDTH(WIDTH)) only_child(a,y);
endmodule
"""
    wide = "256'h" + "123456789ABCDEF0" * 4
    for options, width in (([], 3), (["-GWIDTH=5"], 5),
                           (["-GWIDTH=5", "-GMASK=" + wide], 5)):
      with self.subTest(options=options):
        db, top = self._load_specializations(
            "sv", source, frontend_options={"slang": ["--top=top", *options]})
        self.assertEqual("top", top.getName())
        self.assertEqual(str(width), top.getParameter("WIDTH").getValue())
        self.assertEqual(width, top.getBusTerm("a").getWidth())
        self.assertEqual("top", dict((a.getName(),a.getValue()) for a in top.getAttributes())["naja.source_module"])
        child = top.getInstance("only_child").getModel()
        self.assertEqual("tile", child.getName())
        if len(options) == 2:
          self.assertEqual(wide, top.getParameter("MASK").getValue())
        with tempfile.TemporaryDirectory() as directory:
          top.dumpGateLevelVerilog(directory, "netlist.v")
          with open(os.path.join(directory, "netlist.v")) as stream:
            dumped = stream.read()
          self.assertRegex(dumped, r"(?m)^module top\(")
          self.assertNotIn("module top__", dumped)
          snapshot = os.path.join(directory, "snapshot")
          db.dumpNajaIF(snapshot)
          naja.NLUniverse.get().destroy()
          naja.NLUniverse.create()
          restored = naja.NLDB.loadNajaIF(snapshot).getTopDesign()
          self.assertEqual("top", restored.getName())
          self.assertEqual(str(width), restored.getParameter("WIDTH").getValue())
          self.assertEqual("tile",
                           restored.getInstance("only_child").getModel().getName())

  def test_selected_top_reserves_source_name_in_finite_recursive_hierarchy(self):
    source = """
module top #(parameter int N=0)(input a,output y);
  if(N>0) begin:branch
    top #(.N(N-1)) inner(a,y);
  end else assign y=a;
endmodule
"""
    _, top = self._load_specializations(
        "sv", source, frontend_options={"slang": ["--top=top", "-GN=2"]})
    self.assertEqual("top", top.getName())
    child = next(iter(top.getNonPrimitiveInstances())).getModel()
    self.assertEqual("top__N_1", child.getName())
    default_child = next(iter(child.getNonPrimitiveInstances())).getModel()
    self.assertEqual("0", default_child.getParameter("N").getValue())
    self.assertRegex(default_child.getName(), r"^top__h[0-9a-f]{8}$")
    self.assertEqual(0, len(list(default_child.getNonPrimitiveInstances())))

  def test_single_specialization_retains_source_name_and_resolved_metadata(self):
    sources = {
      "sv": """
module tile #(parameter int WIDTH=2, parameter string LABEL="base")
             (input [WIDTH-1:0] a,output [WIDTH-1:0] y);assign y=a;endmodule
module top(input [4:0] a,output [4:0] y,z);
  tile #(.WIDTH(5),.LABEL("punctuation !")) first(a,y);
  tile #(.LABEL("punctuation !"),.WIDTH(5)) repeated(a,z);
endmodule
""",
      "vhd": """
entity tile is generic(width:positive:=2;label_value:string:="base");
  port(a:in bit_vector(width-1 downto 0);y:out bit_vector(width-1 downto 0));end;
architecture rtl of tile is begin y<=a;end;
entity top is port(a:in bit_vector(4 downto 0);y,z:out bit_vector(4 downto 0));end;
architecture rtl of top is begin
  first:entity work.tile generic map(5,"punctuation !") port map(a,y);
  repeated:entity work.tile generic map(label_value=>"punctuation !",width=>5) port map(a,z);
end;
"""}
    for language, source in sources.items():
      with self.subTest(language=language):
        db, top = self._load_specializations(language, source)
        model = top.getInstance("first").getModel()
        self.assertEqual(model, top.getInstance("repeated").getModel())
        self.assertEqual("tile", model.getName())
        self.assertEqual(5, model.getBusTerm("a").getWidth())
        values = {p.getName(): p.getValue() for p in model.getParameters()}
        self.assertEqual("5", values["WIDTH" if language == "sv" else "width"])
        with tempfile.TemporaryDirectory() as directory:
          top.dumpGateLevelVerilog(directory, "netlist.v")
          with open(os.path.join(directory, "netlist.v")) as stream:
            dumped = stream.read()
          self.assertRegex(dumped, r"(?m)^module tile\(")
          self.assertNotIn("naja.elaboration_", dumped)
          snapshot = os.path.join(directory, "snapshot")
          db.dumpNajaIF(snapshot)
          naja.NLUniverse.get().destroy()
          naja.NLUniverse.create()
          restored = naja.NLDB.loadNajaIF(snapshot).getTopDesign().getInstance("first").getModel()
          self.assertEqual("tile", restored.getName())
          self.assertEqual(values, {p.getName():p.getValue() for p in restored.getParameters()})

  def test_elaborated_specializations_share_names_and_values(self):
    sources = {
      "sv": ("""
module tile #(parameter int COUNT=3, parameter logic [15:0] MASK=0,
              parameter bit ACTIVE=0)(input logic a, output logic y);
  assign y=a;
endmodule
module top(input logic a, output wire y);
  tile #(.COUNT(6), .MASK(16'hA590), .ACTIVE(1)) first(a,y);
  tile #(.COUNT(6), .MASK(16'h1830), .ACTIVE(1)) second(a,);
  tile implicit_default(a,);
  tile #(.COUNT(6), .MASK(16'hA590), .ACTIVE(1)) repeated(a,);
  tile #(.COUNT(3)) explicit_default(a,);
endmodule
""", "COUNT_6__MASK_hA590__ACTIVE_1", {
        "COUNT": "6", "MASK": "16'hA590", "ACTIVE": "1'b1"}),
      "vhd": ("""
library ieee; use ieee.std_logic_1164.all;
entity tile is generic(COUNT:integer:=3;
  MASK:std_ulogic_vector(15 downto 0):=(others=>'0'); ACTIVE:boolean:=false);
  port(a:in bit;y:out bit);end;
architecture rtl of tile is begin y<=a;end;
entity top is port(a:in bit;y:out bit);end;
architecture rtl of top is begin
  first:entity work.tile generic map(6,x"A590",true) port map(a,y);
  second:entity work.tile generic map(6,x"1830",true) port map(a,open);
  implicit_default:entity work.tile port map(a,open);
  repeated:entity work.tile generic map(6,x"A590",true) port map(a,open);
  explicit_default:entity work.tile generic map(COUNT=>3) port map(a,open);
end;
""", "count_6__mask_hA590__active_true", {
        "count": "6", "mask": 'x"A590"', "active": "true"}),
    }
    for language, (source, suffix, expected) in sources.items():
      with self.subTest(language=language):
        db, top = self._load_specializations(language, source)
        models = {inst.getName(): inst.getModel() for inst in top.getInstances()}
        self.assertEqual("tile__" + suffix, models["first"].getName())
        self.assertEqual(models["first"], models["repeated"])
        self.assertEqual(models["implicit_default"], models["explicit_default"])
        self.assertEqual("tile", models["implicit_default"].getName())
        self.assertNotEqual(models["first"], models["second"])
        self.assertEqual(expected, {p.getName(): p.getValue() for p in models["first"].getParameters()})
        self.assertEqual("tile", dict((a.getName(), a.getValue()) for a in models["first"].getAttributes())["naja.source_module"])
        names = {key: model.getName() for key, model in models.items()}
        with tempfile.TemporaryDirectory() as directory:
          top.dumpGateLevelVerilog(directory, "netlist.v")
          with open(os.path.join(directory, "netlist.v")) as stream:
            dumped = stream.read()
          self.assertIn("module tile__" + suffix, dumped)
          self.assertNotIn("parameter ", dumped)
          snapshot = os.path.join(directory, "snapshot")
          db.dumpNajaIF(snapshot)
          naja.NLUniverse.get().destroy()
          naja.NLUniverse.create()
          reloaded = naja.NLDB.loadNajaIF(snapshot)
          model = next(d for lib in reloaded.getLibraries() for d in lib.getSNLDesigns()
                       if d.getName() == names["first"])
          self.assertEqual(expected, {p.getName():p.getValue() for p in model.getParameters()})
          self.assertEqual("tile", dict((a.getName(),a.getValue()) for a in model.getAttributes())["naja.source_module"])
        # Moving the default instance first must not rename any specialization.
        lines = source.splitlines()
        first = next(i for i,line in enumerate(lines) if "first" in line)
        default = next(i for i,line in enumerate(lines) if "implicit_default" in line)
        lines[first], lines[default] = lines[default], lines[first]
        _, reordered = self._load_specializations(language, "\n".join(lines))
        self.assertEqual(names, {inst.getName():inst.getModel().getName() for inst in reordered.getInstances()})

  def test_single_interface_specialization_retains_source_name(self):
    source = """
interface channel_if #(parameter int W=2);
  logic [W-1:0] value,result;
  modport endpoint(input value,output result);
endinterface
module endpoint(channel_if.endpoint bus);assign bus.result=~bus.value;endmodule
module top(input [4:0] a,output [4:0] y);
  channel_if #(.W(5)) channel();
  assign channel.value=a;
  assign y=channel.result;
  endpoint worker(channel);
endmodule
"""
    _, top = self._load_specializations("sv", source)
    model = top.getInstance("worker").getModel()
    self.assertEqual("endpoint", model.getName())
    self.assertEqual("endpoint", dict((a.getName(),a.getValue()) for a in model.getAttributes())["naja.source_module"])

  def test_elaborated_specialization_hash_generate_and_collision(self):
    sources = {
      "sv": """
module tile #(parameter int N=0)(input a, output y);assign y=a;endmodule
module tile__N_2(input a, output y);assign y=~a;endmodule
module wide #(parameter logic [255:0] MASK=0)(input a, output y);assign y=a;endmodule
module top(input a, output [4:0] y);
  for(genvar i=0;i<3;i++) begin:lanes tile #(.N(i)) u(a,y[i]);end
  tile__N_2 user_name(a,y[3]);
  wide #(.MASK(256'h123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0)) hashed(a,y[4]);
endmodule
""",
      "vhd": """
entity tile is generic(n:integer:=0);port(a:in bit;y:out bit);end;
architecture rtl of tile is begin y<=a;end;
entity tile__n_2 is port(a:in bit;y:out bit);end;
architecture rtl of tile__n_2 is begin y<=not a;end;
entity wide is generic(mask:bit_vector(255 downto 0):=(others=>'0'));port(a:in bit;y:out bit);end;
architecture rtl of wide is begin y<=a;end;
entity top is port(a:in bit;y:out bit_vector(4 downto 0));end;
architecture rtl of top is begin
  lanes:for i in 0 to 2 generate u:entity work.tile generic map(i) port map(a,y(i));end generate;
  user_name:entity work.tile__n_2 port map(a,y(3));
  hashed:entity work.wide generic map(x"123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0") port map(a,y(4));
end;
"""}
    for language, source in sources.items():
      with self.subTest(language=language):
        # VHDL basic identifiers cannot contain adjacent underscores. Reserve
        # the generated spelling through the netlist API for this frontend.
        if language == "vhd":
          source = source.replace("entity tile__n_2 is port(a:in bit;y:out bit);end;", "")
          source = source.replace("architecture rtl of tile__n_2 is begin y<=not a;end;", "")
          source = source.replace("user_name:entity work.tile__n_2 port map(a,y(3));", "y(3)<=a;")
        reserved = "tile__n_2" if language == "vhd" else None
        _, top = self._load_specializations(language, source, reserved)
        models = {inst.getName():inst.getModel() for inst in top.getInstances()}
        hashed = models["hashed"].getName()
        self.assertEqual("wide", hashed)
        lane = (lambda i: "top_lanes_" + str(i) + "_u") if language == "sv" else (lambda i: "lanes[" + str(i) + "].u")
        self.assertEqual("tile", models[lane(0)].getName())
        self.assertEqual("tile__" + ("N" if language == "sv" else "n") + "_1", models[lane(1)].getName())
        self.assertRegex(models[lane(2)].getName(), r"^tile__h[0-9a-f]{8}$")
        _, repeated = self._load_specializations(language, source, reserved)
        self.assertEqual(hashed, repeated.getInstance("hashed").getModel().getName())

  def test_elaborated_specialization_value_kinds(self):
    source = """
package choices; typedef enum {slow,fast} mode_t; endpackage
module values #(parameter int OFFSET=0, parameter logic [3:0] MASK=0,
  parameter string LABEL="base", parameter real RATE=1.0,
  parameter choices::mode_t MODE=choices::slow,
  parameter type DATA=logic, parameter int TABLE[2]='{1,2})(input a,output y);
  assign y=a;
endmodule
module top(input a, output [7:0] y);
  values #(.OFFSET(-7)) negative(a,y[0]);
  values #(.MASK(4'b1xz0)) unknowns(a,y[1]);
  values #(.LABEL("other_core")) text_value(a,y[2]);
  values #(.RATE(2.5)) real_value(a,y[3]);
  values #(.MODE(choices::fast)) enumeration(a,y[4]);
  values #(.DATA(logic[2:0])) type_value(a,y[5]);
  values #(.TABLE('{3,4})) array_value(a,y[6]);
  values #(.LABEL("other-core")) lossy_string(a,y[7]);
endmodule
"""
    _, top = self._load_specializations("sv", source)
    models = {i.getName():i.getModel() for i in top.getInstances()}
    for instance, suffix in (("negative", "OFFSET_n7"), ("unknowns", "MASK_b1xz0"),
                             ("text_value", "LABEL_other_core"), ("real_value", "RATE_2p5"),
                             ("enumeration", "MODE_fast")):
      self.assertEqual("values__" + suffix, models[instance].getName())
    for instance in ("type_value", "array_value", "lossy_string"):
      self.assertRegex(models[instance].getName(), r"^values__h[0-9a-f]{8}$")
    self.assertEqual("4'b1xz0", models["unknowns"].getParameter("MASK").getValue())
    self.assertEqual('"other_core"', models["text_value"].getParameter("LABEL").getValue())
    self.assertEqual("choices::fast", models["enumeration"].getParameter("MODE").getValue())
    source = """
library ieee;use ieee.std_logic_1164.all;
entity values is generic(offset:integer:=0;mask:std_ulogic_vector(3 downto 0):="0000";
  label_value:string:="base";flag:std_ulogic:='0');port(a:in bit;y:out bit);end;
architecture rtl of values is begin y<=a;end;
entity top is port(a:in bit;y:out bit_vector(4 downto 0));end;
architecture rtl of top is begin
  negative:entity work.values generic map(offset=>-7) port map(a,y(0));
  unknowns:entity work.values generic map(mask=>"1XZ0") port map(a,y(1));
  text_value:entity work.values generic map(label_value=>"other_core") port map(a,y(2));
  bit_value:entity work.values generic map(flag=>'1') port map(a,y(3));
  lossy_string:entity work.values generic map(label_value=>"other-core") port map(a,y(4));
end;
"""
    _, top = self._load_specializations("vhd", source)
    models = {i.getName():i.getModel() for i in top.getInstances()}
    for instance, suffix in (("negative", "offset_n7"), ("unknowns", "mask_b1xz0"),
                             ("text_value", "label_value_other_core"), ("bit_value", "flag_1")):
      self.assertEqual("values__" + suffix, models[instance].getName())
    self.assertRegex(models["lossy_string"].getName(), r"^values__h[0-9a-f]{8}$")
    self.assertEqual('"1XZ0"', models["unknowns"].getParameter("mask").getValue())
    source = """
package choices is type mode_t is (slow,fast);end;
use work.choices.all;
entity values is generic(rate:real:=1.0;mode:mode_t:=slow);port(a:in bit;y:out bit);end;
architecture rtl of values is begin y<=a when mode=fast else not a;end;
use work.choices.all;
entity top is port(a:in bit;y:out bit_vector(2 downto 0));end;
architecture rtl of top is begin
  real_value:entity work.values generic map(rate=>2.5) port map(a,y(0));
  computed:entity work.values generic map(rate=>1.25*2.0) port map(a,y(1));
  enumeration:entity work.values generic map(mode=>fast) port map(a,y(2));
end;
"""
    _, top = self._load_specializations("vhd", source)
    self.assertEqual("values__rate_2p5", top.getInstance("real_value").getModel().getName())
    self.assertEqual(top.getInstance("real_value").getModel(), top.getInstance("computed").getModel())
    self.assertEqual("values__mode_fast", top.getInstance("enumeration").getModel().getName())
    self.assertEqual("2.5", top.getInstance("real_value").getModel().getParameter("rate").getValue())
    self.assertEqual("fast", top.getInstance("enumeration").getModel().getParameter("mode").getValue())
    # Unsupported std_ulogic values retain an explicit diagnostic.
    with self.assertRaisesRegex(RuntimeError, "RTL literals require"):
      self._load_specializations("vhd", """
library ieee;use ieee.std_logic_1164.all;
entity top is generic(mask:std_ulogic_vector(3 downto 0):="1-X0");end;
architecture rtl of top is begin end;
""")

  def test_elaborated_specialization_types_and_string_identity(self):
    source = r"""
package choices;
  typedef logic [2:0] word_t;
  typedef word_t alias_t;
  typedef struct packed {logic [2:0] value;} record_t;
endpackage
module tile #(parameter type DATA=logic, parameter logic [2:0] MASK=0,
              parameter string LABEL="base")(input a, output y);
  assign y=a;
endmodule
module typed_default #(parameter type DATA=choices::record_t)(input a,output y);
  assign y=a;
endmodule
module enum_default #(parameter type DATA=enum {idle,busy})(input a,output y);
  assign y=a;
endmodule
module top(input a,output [7:0] y);
  tile #(.DATA(choices::word_t)) first(a,y[0]);
  tile #(.DATA(choices::alias_t)) equivalent(a,y[1]);
  tile #(.LABEL("a-b")) punctuation(a,y[2]);
  tile #(.LABEL("a b")) space(a,y[3]);
  tile #(.MASK(3'b001)) narrow(a,y[4]);
  tile #(.LABEL("quote\"line\n\001")) controls(a,y[5]);
  typed_default default_record(a,y[6]);
  enum_default default_enum(a,y[7]);
endmodule
"""
    _, top = self._load_specializations("sv", source)
    models = {i.getName():i.getModel() for i in top.getInstances()}
    self.assertEqual(models["first"], models["equivalent"])
    self.assertEqual("logic[2:0]", models["first"].getParameter("DATA").getValue())
    self.assertEqual("typed_default", models["default_record"].getName())
    self.assertEqual("enum_default", models["default_enum"].getName())
    self.assertEqual("tile__MASK_h1", models["narrow"].getName())
    self.assertNotEqual(models["punctuation"].getName(), models["space"].getName())
    self.assertEqual(r'"quote\"line\n\001"', models["controls"].getParameter("LABEL").getValue())
    for instance in ("punctuation", "space", "controls"):
      self.assertRegex(models[instance].getName(), r"^tile__h[0-9a-f]{8}$")
    names = {key:value.getName() for key,value in models.items()}
    # Type declaration allocation and specialization discovery must not enter
    # canonical identities. Alias spelling must not split a specialization.
    reordered = source.replace("  typedef logic [2:0] word_t;",
                               "  typedef logic unrelated_t;\n  typedef logic [2:0] word_t;")
    lines = reordered.splitlines()
    first = next(i for i,line in enumerate(lines) if ")) first" in line)
    equivalent = next(i for i,line in enumerate(lines) if ")) equivalent" in line)
    punctuation = next(i for i,line in enumerate(lines) if ")) punctuation" in line)
    space = next(i for i,line in enumerate(lines) if ")) space" in line)
    lines[first], lines[equivalent] = lines[equivalent], lines[first]
    lines[punctuation], lines[space] = lines[space], lines[punctuation]
    _, top = self._load_specializations("sv", "\n".join(lines))
    self.assertEqual(names, {i.getName():i.getModel().getName() for i in top.getInstances()})

  def test_clock_gate_wrapper_does_not_create_an_extra_root(self):
    source = '''module phase_cell(input source, request, output result);
      reg held;
      always_latch if (!source) held <= request;
      assign result = source & held;
    endmodule
    module sample(input clk, en, d, output reg q);
      wire gated;
      phase_cell control(clk, en, gated);
      always @(posedge gated) q <= d;
    endmodule'''
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "sample.sv")
      flist = os.path.join(directory, "sources.f")
      with open(path, "w") as stream:
        stream.write(source)
      with open(flist, "w") as stream:
        stream.write(f'--top sample\n"{path}"\n')
      for use_flist in (False, True):
        for keep_ast in (False, True):
          with self.subTest(flist=use_flist, keep_ast=keep_ast):
            naja.NLUniverse.get().destroy()
            universe = naja.NLUniverse.create()
            db = naja.NLDB.create(universe)
            kwargs = {"flist": flist} if use_flist else {}
            top = db.loadSystemVerilog([] if use_flist else [path],
                keep_ast_link=keep_ast, diagnostics_report_path=None, **kwargs)
            self.assertEqual("sample", top.getName())
            self.assertEqual(top, universe.getTopDesign())
            self.assertEqual(top, db.getTopDesign())
            wrapper = top.getLibrary().getSNLDesign("phase_cell")
            self.assertIsNotNone(wrapper)
            self.assertEqual(wrapper, top.getInstance("control").getModel())
            self.assertFalse(wrapper.isPrimitive())
            self.assertEqual(1, sum(i.getModel().isClockGate() for i in wrapper.getInstances()))

  def test_frontend_options_validation(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    for value in ([], {1: []}, {"slang": "--relax-enum-conversions"}, {"slang": [1]}):
      with self.subTest(value=value), self.assertRaisesRegex(TypeError, "frontend_options"):
        db.loadSystemVerilog([], frontend_options=value)
    for value in ({"other": []}, {"slang": [""]}, {"slang": ["--top=bad\0name"]}):
      with self.subTest(value=value), self.assertRaisesRegex(ValueError, "frontend_options"):
        db.loadSystemVerilog([], frontend_options=value)

  def test_relax_enum_conversions(self):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "source.sv")
      flist = os.path.join(directory, "sources.f")
      with open(path, "w", encoding="utf-8") as source:
        source.write("module top(input logic d, output logic q);\n"
                     "typedef enum logic { A=0, B=1 } kind_t; kind_t state;\n"
                     "assign state=d; assign q=state; endmodule\n")
      with open(flist, "w", encoding="utf-8") as output:
        output.write(f'"{path}"\n')
      for use_flist in (False, True):
        naja.NLUniverse.get().destroy()
        db = naja.NLDB.create(naja.NLUniverse.create())
        kwargs = dict(flist=flist) if use_flist else {}
        files = [] if use_flist else [path]
        for strict in ({}, dict(frontend_options={"slang": []})):
          with self.assertRaisesRegex(naja.SystemVerilogSyntaxError, "no implicit conversion"):
            db.loadSystemVerilog(files, diagnostics_report_path=None, **kwargs, **strict)
        top = db.loadSystemVerilog(files, keep_assigns=False, diagnostics_report_path=None,
                                  frontend_options={"slang": ["--relax-enum-conversions"]}, **kwargs)
        self.assertIsNotNone(top.getScalarTerm("d").getNet())
        self.assertEqual(top.getScalarTerm("d").getNet(), top.getScalarTerm("q").getNet())

  def test_allow_use_before_declare(self):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "source.sv")
      flist = os.path.join(directory, "sources.f")
      with open(path, "w", encoding="utf-8") as source:
        source.write("module top(input logic d, output logic q);\n"
                     "typedef struct packed { logic flag; } item_t;\n"
                     "assign q = state.flag; item_t state;\n"
                     "assign state.flag = d; endmodule\n")
      with open(flist, "w", encoding="utf-8") as output:
        output.write(f'"{path}"\n')
      for use_flist in (False, True):
        naja.NLUniverse.get().destroy()
        db = naja.NLDB.create(naja.NLUniverse.create())
        kwargs = dict(flist=flist) if use_flist else {}
        files = [] if use_flist else [path]
        for strict in ({}, dict(frontend_options={"slang": []})):
          with self.assertRaisesRegex(naja.SystemVerilogSyntaxError, "used before its declaration"):
            db.loadSystemVerilog(files, diagnostics_report_path=None, **kwargs, **strict)
        top = db.loadSystemVerilog(files, keep_assigns=False, diagnostics_report_path=None,
                                  frontend_options={"slang": ["--allow-use-before-declare"]}, **kwargs)
        self.assertIsNotNone(top.getScalarTerm("d").getNet())
        self.assertEqual(top.getScalarTerm("d").getNet(), top.getScalarTerm("q").getNet())

  def test_default_timescale(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "source.sv")
      with open(path, "w", encoding="utf-8") as source:
        source.write("module leaf(output wire y); assign y = 1'b1; endmodule\n"
                     "module top(output wire y); timeunit 1ns/1ps; leaf u(y); endmodule\n")
      with self.assertRaisesRegex(naja.SystemVerilogSyntaxError, "time scale"):
        db.loadSystemVerilog([path], frontend_options=None)
      for value in ("invalid", "1ns/1us"):
        with self.assertRaises(naja.SystemVerilogSyntaxError):
          db.loadSystemVerilog([path], frontend_options={"slang": ["--timescale=" + value]})
      top = db.loadSystemVerilog([path], frontend_options={"slang": ["--timescale=1ns/1ps"]})
      self.assertEqual("top", top.getName())

  def test_loader_progress_cancels_inside_large_design(self):
    count = 160
    wires = ",".join("n" + str(i) for i in range(count))
    assignments = " ".join(
        "assign n%d = %s;" % (i, "a" if i == 0 else "n" + str(i - 1))
        for i in range(count))
    verilog = ("module top(input a, output y); wire " + wires + "; " +
               assignments + " assign y=n159; endmodule")
    vhdl = ("entity top is port(a: in bit; y: out bit); end; "
            "architecture rtl of top is signal " + wires + ": bit; begin " +
            " ".join("n%d <= %s;" % (i, "a" if i == 0 else "n" + str(i - 1))
                     for i in range(count)) + " y <= n159; end;")
    liberty = "library(synthetic) {" + "".join(
        'cell(c%d) {pin(A) {direction: input;} '
        'pin(Y) {direction: output; function: "A";}}' % i for i in range(count)) + "}"
    with tempfile.TemporaryDirectory() as directory:
      paths = {}
      for kind, source in (("v", verilog), ("sv", verilog), ("vhd", vhdl), ("lib", liberty)):
        paths[kind] = os.path.join(directory, "source." + kind)
        with open(paths[kind], "w") as stream:
          stream.write(source)
      if naja.NLUniverse.get():
        naja.NLUniverse.get().destroy()
      db = naja.NLDB.create(naja.NLUniverse.create())
      db.loadGateLevelVerilog([paths["v"]])
      paths["if"] = os.path.join(directory, "snapshot")
      db.dumpNajaIF(paths["if"])
      for kind in paths:
        with self.subTest(loader=kind):
          if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()
          db = naja.NLDB.create(naja.NLUniverse.create())
          stage = {"v": "second_pass", "sv": "construct", "vhd": "construct", "lib": "liberty", "if": "construct"}[kind]
          started = False
          cancelled = []
          worker = threading.get_ident()
          def cancel(**event):
            nonlocal started
            self.assertEqual(worker, threading.get_ident())
            if event["stage"] != stage:
              return True
            if kind == "if" and not started and event["detail"] != "snapshot connectivity":
              return True
            if not started:
              started = True
              # Make elapsed time deterministic; cancellation still requires 64 work checks.
              time.sleep(0.06)
              return True
            cancelled.append(event)
            return False
          with self.assertRaises(naja.NajaCancelled):
            if kind == "v":
              db.loadGateLevelVerilog([paths[kind]], progress=cancel)
            elif kind == "sv":
              db.loadSystemVerilog([paths[kind]], progress=cancel, diagnostics_report_path=None)
            elif kind == "vhd":
              db.loadVHDL([paths[kind]], progress=cancel, diagnostics_report_path=None)
            elif kind == "lib":
              db.loadLibertyPrimitives([paths[kind]], progress=cancel)
            else:
              naja.NLUniverse.get().destroy()
              naja.NLUniverse.create()
              naja.NLDB.loadNajaIF(paths[kind], progress=cancel)
          self.assertTrue(cancelled)
          # A named intermediate report proves we stopped inside construction,
          # before its unconditional completion report (whose detail is empty).
          self.assertTrue(cancelled[-1]["detail"])
          naja.NLUniverse.get().destroy()

  def test_loader_progress_contract(self):
    sources = {
      "v": "module top(input a, output y); assign y=a; endmodule",
      "sv": "module top(input logic a, output logic y); assign y=a; endmodule",
      "vhd": ("entity top is port(a: in bit; y: out bit); end; "
              "architecture rtl of top is begin y <= a; end;"),
      "lib": ('library(synthetic) { cell(buffer) { pin(A) { direction: input; } '
              'pin(Y) { direction: output; function: "A"; } } }'),
    }
    expected = {
      "v": ["parse", "first_pass", "second_pass", "done"],
      "sv": ["parse", "elaborate", "construct", "done"],
      "vhd": ["parse", "construct", "done"],
      "lib": ["parse", "liberty", "done"],
      "if": ["parse", "construct", "done"],
    }
    def reset():
      if naja.NLUniverse.get():
        naja.NLUniverse.get().destroy()
      return naja.NLDB.create(naja.NLUniverse.create())

    with tempfile.TemporaryDirectory() as directory:
      paths = {}
      for kind, text in sources.items():
        paths[kind] = os.path.join(directory, "synthetic." + kind)
        with open(paths[kind], "w") as stream:
          stream.write(text)
      snapshot = os.path.join(directory, "snapshot")
      db = reset()
      db.loadGateLevelVerilog([paths["v"]])
      db.dumpNajaIF(snapshot)

      def load(kind, db, **kwargs):
        if kind == "v":
          return db.loadGateLevelVerilog([paths[kind]], **kwargs)
        if kind == "sv":
          return db.loadSystemVerilog([paths[kind]], diagnostics_report_path=None, **kwargs)
        if kind == "vhd":
          return db.loadVHDL([paths[kind]], diagnostics_report_path=None, **kwargs)
        if kind == "lib":
          return db.loadLibertyPrimitives([paths[kind]], **kwargs)
        # Snapshot restores its original database ID in a fresh universe.
        naja.NLUniverse.get().destroy()
        naja.NLUniverse.create()
        return naja.NLDB.loadNajaIF(snapshot, **kwargs)

      for kind in expected:
        with self.subTest(loader=kind):
          load(kind, reset())
          baseline = naja.NLUniverse.get().getContentHash()
          events = []
          def record(**event):
            self.assertTrue(event["total"] == 0 or event["current"] <= event["total"])
            self.assertIsInstance(event["detail"], str)
            if event["stage"] == "done" and kind != "lib":
              self.assertIsNotNone(naja.NLUniverse.get().getTopDesign())
            events.append(event)
            return True
          load(kind, reset(), progress=record)
          self.assertEqual(baseline, naja.NLUniverse.get().getContentHash())
          stages = []
          for event in events:
            if not stages or stages[-1] != event["stage"]:
              stages.append(event["stage"])
          self.assertEqual(expected[kind], stages)
          cancelled = []
          def cancel(**event):
            cancelled.append(event)
            return event["stage"] in ("parse", "elaborate", "first_pass") or event["current"] == 0
          with self.assertRaises(naja.NajaCancelled):
            load(kind, reset(), progress=cancel)
          self.assertGreater(cancelled[-1]["current"], 0)
          self.assertNotEqual("done", cancelled[-1]["stage"])
          reset()  # Destruction after cancellation must remain safe.
          def fail(**event):
            raise ValueError("synthetic callback failure")
          with self.assertRaisesRegex(ValueError, "synthetic callback failure"):
            load(kind, reset(), progress=fail)
          reset()
          with self.assertRaisesRegex(TypeError, "progress must be callable"):
            load(kind, reset(), progress=7)

  def setUp(self):
    naja.NLUniverse.create()

  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def test(self):
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(db)
    naja.NLUniverse.get().setTopDB(db)
    self.assertEqual(naja.NLUniverse.get().getTopDB(), db)
    self.assertEqual(db.getID(), 1) # ID starts from 1, 0 is reserved for DB0
    with self.assertRaises(RuntimeError) as context: naja.NLUniverse.get().setTopDB("Error")
    del db    
  
  def testGateLevelVerilogNames(self):
    self.assertFalse(hasattr(naja.NLDB, "loadVerilog"))
    self.assertFalse(hasattr(naja.NLDB, "dumpVerilog"))
    self.assertFalse(hasattr(naja.SNLDesign, "dumpVerilog"))
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.TemporaryDirectory() as directory:
      source = os.path.join(directory, "input.v")
      with open(source, "w") as stream:
        stream.write("module top(input a, output y); assign y = a; endmodule\n")
      with warnings.catch_warnings():
        warnings.simplefilter("error", FutureWarning)
        top = db.loadGateLevelVerilog(files=[source], keep_assigns=False)
        self.assertEqual(top.getName(), "top")
        db.dumpGateLevelVerilog(os.path.join(directory, "new_db.v"))
        top.dumpGateLevelVerilog(directory, "new_design.v", dumpAssignsAsInstances=True)
      with self.assertRaisesRegex(TypeError, "NLDB.loadGateLevelVerilog: files"):
        db.loadGateLevelVerilog("invalid")

  def testVerilog(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadGateLevelVerilog(verilogs)
    db.dumpGateLevelVerilog("./test_verilog")
    with self.assertRaises(RuntimeError) as context: db.dumpGateLevelVerilog()
    with self.assertRaises(RuntimeError) as context: db.dumpGateLevelVerilog(-1)
    with self.assertRaisesRegex(
        TypeError, r"files must be a list\[str\], got str"):
      db.loadLibertyPrimitives("./error.lib")

  def testVHDL(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    source = """\
entity inverter is port(a : in bit; y : out bit); end;
architecture rtl of inverter is begin y <= not a; end;
"""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".vhd") as vhdl:
      vhdl.write(source)
      vhdl.flush()
      with self.assertWarnsRegex(RuntimeWarning, "VHDL parser is in Beta mode"):
        design = db.loadVHDL(vhdl.name)
    self.assertEqual(design.getName(), "inverter")
    self.assertEqual(db.getTopDesign(), design)
    self.assertTrue(db.isTopDB())

  def testVHDLIdentityTypeConversions(self):
    formats_path = os.environ['FORMATS_PATH']
    for top, source, target, width in (("conv", "a_i", "a_o", 1),
                                       ("conv_vec", "v_i", "v_o", 4)):
      with self.subTest(top=top):
        db = naja.NLDB.create(naja.NLUniverse.get())
        with warnings.catch_warnings():
          warnings.simplefilter("ignore", RuntimeWarning)
          design = db.loadVHDL([os.path.join(formats_path, "vhdl", top + ".vhd")],
                              top=top, diagnostics_report_path=None)
        inputs = ([design.getScalarTerm(source)] if width == 1 else
                  [design.getBusTerm(source).getBusTermBit(i) for i in range(width)])
        outputs = ([design.getScalarTerm(target)] if width == 1 else
                   [design.getBusTerm(target).getBusTermBit(i) for i in range(width)])
        assigns = list(design.getAssignInstances())
        self.assertEqual(len(assigns), width)
        for input_term, output_term in zip(inputs, outputs):
          drivers = [inst for inst in assigns
                     if any(term.getNet() == output_term.getNet() for term in inst.getInstTerms())]
          self.assertEqual(len(drivers), 1)
          self.assertTrue(any(term.getNet() == input_term.getNet()
                              for term in drivers[0].getInstTerms()))

  def testVHDLReadOnlyMemory(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    source = """\
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity lookup is port(a : in unsigned(1 downto 0); y : out std_logic_vector(3 downto 0)); end;
architecture rtl of lookup is
  type rows is array(0 to 2) of std_logic_vector(3 downto 0);
  constant data : rows := (x"1", x"A", x"3");
begin y <= data(to_integer(a)); end;
"""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".vhd") as vhdl:
      vhdl.write(source)
      vhdl.flush()
      with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        design = db.loadVHDL(vhdl.name, diagnostics_report_path=None)
    roms = [i for i in design.getInstances() if i.getInstParameter("WR_PORTS") is not None]
    self.assertEqual(len(roms), 1)
    rom = roms[0]
    self.assertEqual(rom.getInstParameter("WR_PORTS").getValue(), "0")
    self.assertEqual(rom.getInstParameter("INIT_ENABLE").getValue(), "1")
    self.assertEqual(rom.getInstParameter("INIT").getValue(), "12'h3a1")
    self.assertFalse(rom.getModel().isSequential())
    self.assertEqual({t.getName() for t in rom.getModel().getTerms()}, {"RADDR", "RDATA"})
    self.assertEqual(next(iter(rom.getModel().getBusTerm("RADDR").getBits())).getRole(), naja.SNLTermRole.MemoryReadAddress)
    self.assertEqual(next(iter(rom.getModel().getBusTerm("RDATA").getBits())).getRole(), naja.SNLTermRole.MemoryReadData)

  def testVHDLBatchFiles(self):
    # Deliberately put package declarations and entities after their users in
    # pathname order; the loader must resolve units, not merely sort files.
    units = [
      ("z_package.vhd", "package widths is constant width : integer := 3; end;\n"),
      ("y_leaf.vhd", "use work.widths.all;\n"
       "entity leaf is port(a : in bit_vector(width-1 downto 0); "
       "y : out bit_vector(width-1 downto 0)); end;\n"
       "architecture rtl of leaf is begin y <= not a; end;\n"),
      ("x_middle.vhd", "use work.widths.all;\n"
       "entity middle is port(a : in bit_vector(width-1 downto 0); "
       "y : out bit_vector(width-1 downto 0)); end;\n"
       "architecture rtl of middle is begin\n"
       "u: entity work.leaf port map(a, y); end;\n"),
      ("a_top.vhd", "use work.widths.all;\n"
       "entity top is port(a : in bit_vector(width-1 downto 0); "
       "y : out bit_vector(width-1 downto 0)); end;\n"
       "architecture rtl of top is begin\n"
       "u: entity work.middle port map(a, y); end;\n"),
    ]
    with tempfile.TemporaryDirectory() as directory:
      paths = []
      for name, text in units:
        path = os.path.join(directory, name)
        with open(path, "w") as output: output.write(text)
        paths.append(path)
      single = os.path.join(directory, "single.vhd")
      with open(single, "w") as output:
        output.write("".join(text for _, text in reversed(units)))
      reference = None
      for files in (paths, list(reversed(paths)), single):
        for explicit in (None, "top"):
          with self.subTest(files=files, top=explicit):
            db = naja.NLDB.create(naja.NLUniverse.get())
            design = db.loadVHDL(files=files, top=explicit, diagnostics_report_path=None)
            self.assertEqual(design.getName(), "top")
            self.assertEqual(design.getBusTerm("a").getWidth(), 3)
            middle = next(iter(design.getInstances())).getModel()
            self.assertEqual(middle.getName(), "middle")
            self.assertEqual(next(iter(middle.getInstances())).getModel().getName(), "leaf")
            design.dumpGateLevelVerilog(directory, "netlist.v")
            with open(os.path.join(directory, "netlist.v")) as output:
              # The banner date can change between otherwise identical dumps.
              netlist = re.sub(
                r"^// (?:Mon|Tue|Wed|Thu|Fri|Sat|Sun) .*\n", "", output.read(),
                flags=re.MULTILINE)
            if reference is None: reference = netlist
            else: self.assertEqual(netlist, reference)
            self.assertEqual(db.loadVHDL(files, top=explicit, diagnostics_report_path=None), design)
            db.destroy()

      db = naja.NLDB.create(naja.NLUniverse.get())
      with self.assertRaisesRegex(RuntimeError, r"missing entity: leaf.*x_middle.vhd.*line 4"):
        db.loadVHDL([paths[3], paths[2], paths[0]], diagnostics_report_path=None)
      self.assertIsNone(db.getLibrary("DESIGN").getSNLDesign("top"))
      self.assertIsNotNone(db.loadVHDL(paths, diagnostics_report_path=None))

  def testVHDLBatchDependenciesAndErrors(self):
    with tempfile.TemporaryDirectory() as directory:
      def write(name, source):
        path = os.path.join(directory, name)
        with open(path, "w") as output: output.write(source)
        return path

      # A package body may precede its declaration, including within a file.
      package = write("package.vhd", "package body sizes is "
                      "function size return integer is begin return 2; end; end;\n"
                      "package sizes is function size return integer; end;\n")
      entity = write("entity.vhd", "use work.sizes.all;\n"
                     "entity e is port(a : in bit_vector(size-1 downto 0); "
                     "y : out bit_vector(size-1 downto 0)); end;\n")
      architecture = write("architecture.vhd", "architecture rtl of e is begin y <= a; end;\n")
      db = naja.NLDB.create(naja.NLUniverse.get())
      top = db.loadVHDL([architecture, entity, package], diagnostics_report_path=None)
      self.assertEqual(top.getBusTerm("a").getWidth(), 2)
      db.destroy()

      for name, sources, expected in (
          ("packages", ["use work.b.all; package a is end;",
                        "use work.a.all; package b is end;"], "package dependency cycle"),
          ("entities", ["entity a is end; architecture rtl of a is begin\n"
                        "u: entity work.b port map(); end;",
                        "entity b is end; architecture rtl of b is begin\n"
                        "u: entity work.a port map(); end;"], "entity dependency cycle"),
          ("missing", ["use work.absent.all; package a is end;"], "missing package: absent"),
          ("ambiguous", ["entity a is end; architecture rtl of a is begin end;",
                         "entity b is end; architecture rtl of b is begin end;"], "unique RTL top")):
        with self.subTest(name=name):
          files = [write(f"{name}{i}.vhd", text) for i, text in enumerate(sources)]
          db = naja.NLDB.create(naja.NLUniverse.get())
          with self.assertRaisesRegex(RuntimeError, expected) as error:
            db.loadVHDL(files, diagnostics_report_path=None)
          self.assertIn(".vhd' at line", str(error.exception))
          if name == "entities":
            with self.assertRaisesRegex(RuntimeError, "entity dependency cycle"):
              db.loadVHDL(files, top="a", diagnostics_report_path=None)
          db.destroy()

  def testVHDLBatchDiagnostics(self):
    with tempfile.TemporaryDirectory() as directory:
      paths = []
      for name in ("first", "second"):
        path = os.path.join(directory, name + ".vhd")
        with open(path, "w") as output:
          output.write(f"entity {name} is port(y : out bit); end;\n"
                       f"architecture rtl of {name} is begin\n"
                       "  assert false; y <= '1'; end;\n")
        paths.append(path)
      report = os.path.join(directory, "warnings.log")
      db = naja.NLDB.create(naja.NLUniverse.get())
      db.loadVHDL(paths, top="second", diagnostics_report_path=report)
      with open(report) as output: warnings_text = output.read()
      self.assertEqual(warnings_text.count("[ignored-assertion]"), 2)
      for path in paths: self.assertIn(path + ":3:3:", warnings_text)
      db.destroy()

      # A file's incomplete syntax must not consume text from the next file.
      with open(paths[0], "w") as output: output.write("entity broken is\n")
      db = naja.NLDB.create(naja.NLUniverse.get())
      with self.assertRaisesRegex(RuntimeError, r"first.vhd.*line"):
        db.loadVHDL(paths, top="second", diagnostics_report_path=None)
      self.assertIsNone(db.getLibrary("DESIGN").getSNLDesign("second"))

  def testVHDLBatchArguments(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    for value, error in (([], ValueError), ([1], TypeError), ([""], ValueError)):
      with self.subTest(value=value), self.assertRaises(error):
        db.loadVHDL(value)
    with self.assertRaisesRegex(TypeError, "either file or files"):
      db.loadVHDL(file="a.vhd", files=["a.vhd"])

  def testVHDLBetaWarningAsError(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with warnings.catch_warnings():
      warnings.simplefilter("error", RuntimeWarning)
      with self.assertRaisesRegex(RuntimeWarning, "VHDL parser is in Beta mode"):
        db.loadVHDL("input.vhd")
    self.assertEqual(len(list(db.getLibraries())), 0)

  def testVHDLAndSystemVerilogAdditionPrimitives(self):
    sources = {
      ".vhd": """library ieee; use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
entity add4 is port(a, b : in unsigned(3 downto 0); s : out unsigned(3 downto 0)); end;
architecture rtl of add4 is begin s <= a + b; end;
""",
      ".v": "module add4(input [3:0] a, b, output [3:0] s); assign s = a + b; endmodule"
    }
    for suffix, source in sources.items():
      with self.subTest(frontend=suffix):
        db = naja.NLDB.create(naja.NLUniverse.get())
        with tempfile.NamedTemporaryFile(mode="w", suffix=suffix) as hdl:
          hdl.write(source)
          hdl.flush()
          if suffix == ".vhd":
            with warnings.catch_warnings():
              warnings.simplefilter("ignore", RuntimeWarning)
              top = db.loadVHDL(hdl.name, diagnostics_report_path=None)
          else:
            top = db.loadSystemVerilog([hdl.name], diagnostics_report_path=None)
        arithmetic = [i for i in top.getInstances() if not i.getModel().isAssign()]
        expected = ["naja_add__w4"] if suffix == ".v" else ["naja_fa"] * 4
        self.assertEqual([i.getModel().getName() for i in arithmetic], expected)
        db.destroy()

  def testVHDLTruncatedArithmetic(self):
    for operation, adders in [("a + b", 4), ("resize(a * b, 4)", 12)]:
      with self.subTest(operation=operation):
        db = naja.NLDB.create(naja.NLUniverse.get())
        source = ("library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; "
                  "entity math4 is port(a, b : in unsigned(3 downto 0); "
                  "s : out unsigned(3 downto 0)); end; "
                  "architecture rtl of math4 is begin s <= " + operation + "; end;")
        with tempfile.NamedTemporaryFile(mode="w", suffix=".vhd") as hdl:
          hdl.write(source)
          hdl.flush()
          with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            top = db.loadVHDL(hdl.name, diagnostics_report_path=None)
        instances = list(top.getInstances())
        self.assertEqual(sum(i.getModel().getName() == "naja_fa" for i in instances), adders)
        for instance in instances:
          if instance.getModel().getName() == "naja_fa":
            continue
          for term in instance.getInstTerms():
            if term.getDirection() != naja.SNLTerm.Direction.Output:
              continue
            net = term.getNet()
            self.assertIsNotNone(net)
            self.assertTrue(
                any(t.getDirection() != naja.SNLTerm.Direction.Output for t in net.getInstTerms()) or
                any(t.getDirection() != naja.SNLTerm.Direction.Input for t in net.getBitTerms()))
        db.destroy()

  def testVHDLWarningReport(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, "diagnostics.vhd")
      report = os.path.join(directory, "reports", "warnings.log")
      with open(path, "w") as output:
        output.write("entity e is port(y : out bit); end; "
                     "architecture rtl of e is begin assert false; assert false; "
                     "process(all) begin y <= '1'; end process; end;")
      self.assertIsNotNone(db.loadVHDL(path, diagnostics_report_path=report))
      with open(report) as output:
        warnings = output.read()
      self.assertEqual(warnings.count("[ignored-assertion]"), 2)
      self.assertIn(path + ":1:", warnings)
      # Re-parsing retained input does not duplicate warning occurrences.
      self.assertIsNotNone(db.loadVHDL(path, diagnostics_report_path=report))
      with open(report) as output:
        self.assertEqual(output.read().count("[ignored-assertion]"), 2)
      self.assertIsNotNone(db.loadVHDL(path, diagnostics_report_path=None))
      with self.assertRaises(TypeError):
        db.loadVHDL(path, diagnostics_report_path=1)
      with self.assertRaises(ValueError):
        db.loadVHDL(path, diagnostics_report_path="")

  def testVHDLRejectsUnexpectedKeyword(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with self.assertRaises(TypeError):
      db.loadVHDL("input.vhd", unsupported=True)

  def testVHDLHierarchy(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    source = """\
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture structural of top is begin
  u0: entity work.leaf port map(a, y);
end;
"""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".vhd") as vhdl:
      vhdl.write(source)
      vhdl.flush()
      design = db.loadVHDL(vhdl.name, top="TOP")
    self.assertEqual(design.getName(), "top")
    self.assertEqual(db.getTopDesign(), design)
    self.assertIsNotNone(db.getLibrary("DESIGN").getSNLDesign("leaf"))

  def testVHDLPackageAcrossLoads(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    sources = [
      "package bytes is type pair is array(0 to 1) of bit_vector(3 downto 0); "
      'constant rom : pair := (x"A", x"5"); end;',
      "use work.bytes.all; entity lookup is port(y : out bit_vector(3 downto 0)); end; "
      "architecture rtl of lookup is begin y <= rom(1); end;",
      "package empty_pkg is end;",
    ]
    with tempfile.TemporaryDirectory() as directory:
      results = []
      for index, source in enumerate(sources):
        path = os.path.join(directory, str(index) + ".vhd")
        with open(path, "w") as output:
          output.write(source)
        results.append(db.loadVHDL(path))
    self.assertIsNone(results[0])
    self.assertEqual(results[1].getName(), "lookup")
    self.assertIsNone(results[2])
    self.assertEqual(db.getTopDesign(), results[1])
    other = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.NamedTemporaryFile(mode="w", suffix=".vhd") as source:
      source.write(sources[1])
      source.flush()
      with self.assertRaises(RuntimeError):
        other.loadVHDL(source.name)

  def testVHDLRequiredGenericsAcrossFiles(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    dependency = (
      "entity child is\n"
      "generic(\n"
      "  Nin : positive);\n"
      "port(a : in bit_vector(Nin-1 downto 0); y : out bit_vector(Nin-1 downto 0)); end;\n"
      "architecture rtl of child is begin y <= a; end;\n")
    parent = (
      "entity parent is port(a : in bit_vector(2 downto 0); y : out bit_vector(2 downto 0)); end;\n"
      "architecture rtl of parent is begin\n"
      "u : entity work.child generic map(3) port map(a,y); end;\n")
    with tempfile.TemporaryDirectory() as directory:
      child_path = os.path.join(directory, "child.vhd")
      parent_path = os.path.join(directory, "parent.vhd")
      with open(child_path, "w") as source:
        source.write(dependency)
      with open(parent_path, "w") as source:
        source.write(parent)
      with self.assertRaisesRegex(RuntimeError, "missing generic value: nin.*line 3, column 3") as error:
        db.loadVHDL(child_path, top="child")
      self.assertIn(child_path, str(error.exception))
      self.assertIsNone(db.loadVHDL(child_path))
      design = db.loadVHDL(parent_path, top="parent")
      self.assertEqual(design.getName(), "parent")
      self.assertEqual(db.getTopDesign(), design)
      self.assertEqual(db.loadVHDL(parent_path, top="parent"), design)
      self.assertIsNone(db.loadVHDL(child_path))
      self.assertEqual(db.getTopDesign(), design)

  def testHDLDestinationLibraries(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.TemporaryDirectory() as directory:
      vhdl = os.path.join(directory, "leaf.vhd")
      sv = os.path.join(directory, "leaf.sv")
      with open(vhdl, "w") as source:
        source.write("entity leaf is port(a : in bit; y : out bit); end; "
                     "architecture rtl of leaf is begin y <= a; end;")
      with open(sv, "w") as source:
        source.write("module leaf(input a, output y); assign y = a; endmodule")
      for loader, files, name in ((db.loadVHDL, vhdl, "VHDLCells"),
                                  (db.loadSystemVerilog, [sv], "SVCells")):
        with self.subTest(loader=name):
          for invalid in (None, 4):
            with self.assertRaisesRegex(TypeError, "library must be a str"):
              loader(files, library=invalid)
          for invalid in ("", "   "):
            with self.assertRaisesRegex(ValueError, "library must not be empty"):
              loader(files, library=invalid)
          loader(files, library=name, diagnostics_report_path=None)
          self.assertIsNotNone(db.getLibrary(name).getSNLDesign("leaf"))
          self.assertIsNone(db.getLibrary("DESIGN"))
      self.assertEqual(db.loadVHDL(vhdl, library="vhdlcells", diagnostics_report_path=None),
                       db.getLibrary("VHDLCells").getSNLDesign("leaf"))
      naja.NLLibrary.create(db, "VHDLCELLS")
      with self.assertRaisesRegex(RuntimeError, "ambiguous HDL library"):
        db.loadVHDL(vhdl, library="vhdlcells", diagnostics_report_path=None)

  def testVHDLCrossLibraryDependencyDiagnostic(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.TemporaryDirectory() as directory:
      child = os.path.join(directory, "child.vhd")
      parent = os.path.join(directory, "parent.vhd")
      with open(child, "w") as source:
        source.write("entity leaf is generic(n : positive); port(y : out bit); end;\n"
                     "architecture rtl of leaf is begin\n"
                     "y <= missing; end;\n")
      with open(parent, "w") as source:
        source.write("library cells; entity top is port(y : out bit); end;\n"
                     "architecture rtl of top is begin\n"
                     "u: entity cells.leaf generic map(n => 1) port map(y); end;")
      self.assertIsNone(db.loadVHDL(child, library="cells", diagnostics_report_path=None))
      with self.assertRaises(RuntimeError) as error:
        db.loadVHDL(parent, library="DESIGN", diagnostics_report_path=None)
      self.assertIn(child, str(error.exception))
      self.assertIn("line 3", str(error.exception))
      self.assertIsNone(db.getLibrary("DESIGN").getSNLDesign("top"))
      self.assertIsNone(db.getLibrary("cells").getSNLDesign("leaf__n_1"))

  def testVHDLBooleanGenericDependencyAcrossLibraries(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.TemporaryDirectory() as directory:
      child = os.path.join(directory, "leaf.vhd")
      parent = os.path.join(directory, "parent.vhd")
      with open(child, "w") as source:
        source.write("entity leaf is generic(enabled : boolean); port(a : in bit; y : out bit); end; "
                     "architecture rtl of leaf is begin y <= a when enabled else not a; end;")
      with open(parent, "w") as source:
        source.write("package settings is function enabled_f(n : natural) return boolean; end; "
                     "package body settings is function enabled_f(n : natural) return boolean is "
                     "begin return n > 0; end; end; "
                     "library cells; use work.settings.all; "
                     "entity parent is generic(enabled : boolean := enabled_f(1)); "
                     "port(a : in bit; y : out bit); end; architecture rtl of parent is begin "
                     "u: entity cells.leaf generic map(not enabled) port map(a, y); end;")
      self.assertIsNone(db.loadVHDL(child, library="cells", diagnostics_report_path=None))
      top = db.loadVHDL(parent, library="DESIGN", diagnostics_report_path=None)
      model = db.getLibrary("cells").getSNLDesign("leaf")
      self.assertIsNotNone(model)
      self.assertEqual("false", model.getParameter("enabled").getValue())
      self.assertEqual(top.getInstance("u").getModel(), model)
      self.assertEqual(db.loadVHDL(parent, library="DESIGN", diagnostics_report_path=None), top)

  def testVHDLArgumentsAndFailureRollback(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with self.assertRaisesRegex(TypeError, "file must be a str path"):
      db.loadVHDL(1)
    with self.assertRaisesRegex(ValueError, "file must not be empty"):
      db.loadVHDL("")
    with self.assertRaisesRegex(TypeError, "top must be a str or None"):
      db.loadVHDL("missing.vhd", top=1)
    with self.assertRaisesRegex(ValueError, "top must not be empty"):
      db.loadVHDL("missing.vhd", top="")
    with self.assertRaisesRegex(RuntimeError, "cannot open VHDL file"):
      db.loadVHDL("missing.vhd")

    unsupported = """\
entity unsupported is port(a : in bit; y : out bit); end;
architecture rtl of unsupported is begin y <= a and missing; end;
"""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".vhd") as vhdl:
      vhdl.write(unsupported)
      vhdl.flush()
      with self.assertRaises(RuntimeError):
        db.loadVHDL(vhdl.name)
    self.assertIsNone(db.getLibrary("DESIGN").getSNLDesign("unsupported"))

  def testLoadLibertyPrimitivesRenamedFiles(self):
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    source = os.path.join(
      formats_path,
      "liberty",
      "benchmarks",
      "tests",
      "small.lib")
    self.assertTrue(os.path.exists(source))

    with tempfile.TemporaryDirectory() as tempdir:
      lib_suffix_path = os.path.join(tempdir, "small.lib_tt")
      gzip_path = os.path.join(tempdir, "small.memory")
      zip_path = os.path.join(tempdir, "small.bundle")

      shutil.copyfile(source, lib_suffix_path)
      with open(source, "rb") as input_file:
        with gzip.open(gzip_path, "wb") as gzip_file:
          shutil.copyfileobj(input_file, gzip_file)
      with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.write(source, "memories/small.lib_tt")

      for path in (lib_suffix_path, gzip_path, zip_path):
        with self.subTest(path=path):
          db = naja.NLDB.create(naja.NLUniverse.get())
          db.loadLibertyPrimitives([path])
          primitives = db.getLibrary("small_lib")
          self.assertIsNotNone(primitives)
          self.assertIsNotNone(primitives.getSNLDesign("and2"))
          db.destroy()

  def testVerilogLoadingOptions(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)

    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "conflicting_name_designs.v")]
    with self.assertRaises(RuntimeError) as context: db.loadGateLevelVerilog(verilogs, conflicting_design_name_policy="forbid")

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadGateLevelVerilog(verilogs, conflicting_design_name_policy="first")
    self.assertIsNotNone(top)
    self.assertEqual(top.getName(), 'clash')
    self.assertEqual(1, sum(1 for t in top.getTerms()))
    self.assertEqual(1, sum(1 for t in top.getScalarTerms()))
    term = next(iter(top.getScalarTerms()))
    self.assertIsNotNone(term)
    self.assertEqual(term.getName(), 'A')

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadGateLevelVerilog(verilogs, conflicting_design_name_policy="last")
    self.assertIsNotNone(top)
    self.assertEqual(top.getName(), 'clash')
    self.assertEqual(1, sum(1 for t in top.getTerms()))
    self.assertEqual(1, sum(1 for t in top.getScalarTerms()))
    term = next(iter(top.getScalarTerms()))
    self.assertIsNotNone(term)
    self.assertEqual(term.getName(), 'D') 

    #errors
    db.destroy()
    db = naja.NLDB.create(u)
    with self.assertRaises(RuntimeError) as context: db.loadGateLevelVerilog(verilogs, conflicting_design_name_policy='verify')
    with self.assertRaisesRegex(
        TypeError, r"conflicting_design_name_policy must be a str or None, got int"):
      db.loadGateLevelVerilog(verilogs, conflicting_design_name_policy=1)
    with self.assertRaisesRegex(
        ValueError, r"expected one of: forbid, first, last, verify"):
      db.loadGateLevelVerilog(verilogs, conflicting_design_name_policy='foo')


  def testSNLFormat(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadGateLevelVerilog(verilogs)
    self.assertIsNotNone(db.getTopDesign())
    top = db.getTopDesign()
    self.assertTrue(top.isTopDesign())
    self.assertEqual(top.getDB(), db)
    self.assertTrue(db.isTopDB())

    naja_path = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(naja_path)
    naja_dir = os.path.join(naja_path, "test.naja")
    db.dumpNajaIF(naja_dir)
    manifest = naja.snapshotManifest(naja_dir)
    self.assertEqual(manifest["schema_version"], (0, 3, 0))
    self.assertEqual(manifest["producer_version"], naja.getVersion())
    self.assertEqual(manifest["producer_git_hash"], naja.getGitHash())
    #destroy everything
    naja.NLUniverse.get().destroy()

    manifest = naja.snapshotManifest(naja_dir)
    self.assertEqual(manifest["schema_version"], (0, 3, 0))
    self.assertIsNone(naja.NLUniverse.get())

    #load NajaIF
    naja.NLDB.loadNajaIF(naja_dir)
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = u.getTopDB()
    self.assertIsNotNone(db)
    self.assertEqual(db.getID(), 1)

    # The earlier revision must be rejected even when the producer matches.
    with open(os.path.join(naja_dir, "snl.mf"), "w", encoding="utf-8") as manifest_file:
      manifest_file.write("V 0 1 0\n")
      manifest_file.write(f"P {naja.getVersion()} {naja.getGitHash()}\n")
    naja.NLUniverse.get().destroy()
    with self.assertRaisesRegex(RuntimeError, "Incompatible SNL snapshot schema version"):
      naja.NLDB.loadNajaIF(naja_dir)
    self.assertIsNone(naja.NLUniverse.get().getTopDesign())

    with open(os.path.join(naja_dir, "snl.mf"), "w", encoding="utf-8") as manifest_file:
      manifest_file.write("V " + " ".join(map(str, manifest["schema_version"])) + "\n")
      manifest_file.write("P test-producer test-hash\n")
    with self.assertRaises(RuntimeError) as context:
      naja.NLDB.loadNajaIF(naja_dir)
    self.assertIn("Incompatible SNL snapshot producer", str(context.exception))

  def testSnapshotManifestInvalidArguments(self):
    with self.assertRaisesRegex(
        RuntimeError, "malformed naja snapshotManifest"):
      naja.snapshotManifest()

    with self.assertRaisesRegex(
        RuntimeError, "snapshotManifest argument should be a file path"):
      naja.snapshotManifest(42)

  def testLoaderArgumentValidation(self):
    db = naja.NLDB.create(naja.NLUniverse.get())

    invalid_calls = (
      (
        lambda: db.loadLibertyPrimitives([]),
        ValueError,
        "files must contain at least one Liberty path"),
      (
        lambda: db.loadLibertyPrimitives([""]),
        ValueError,
        r"files\[0\] must not be empty"),
      (
        lambda: db.loadGateLevelVerilog([]),
        ValueError,
        "files must contain at least one Verilog path"),
      (
        lambda: db.loadGateLevelVerilog([""]),
        ValueError,
        r"files\[0\] must not be empty"),
      (
        lambda: db.loadSystemVerilog(
          [], elaborated_ast_json_path=""),
        ValueError,
        "elaborated_ast_json_path must not be empty"),
      (
        lambda: db.loadSystemVerilog(
          [], suppress_warnings=[""]),
        ValueError,
        r"suppress_warnings\[0\] must not be empty"),
      (
        lambda: db.loadSystemVerilog(
          [], suppress_warnings=["width trunc"]),
        ValueError,
        r"suppress_warnings\[0\] must not contain whitespace"),
      (
        lambda: db.loadSystemVerilog(
          [], suppress_warnings=["-Wwidth-trunc"]),
        ValueError,
        r"suppress_warnings\[0\].*without a -W/-Wno- prefix"),
      (
        lambda: db.loadSystemVerilog([], defines=[""]),
        ValueError,
        r"defines\[0\] must not be empty"),
      (
        lambda: db.loadSystemVerilog([], defines=["SYNTHESIS DEBUG"]),
        ValueError,
        r"defines\[0\] must not contain whitespace"),
      (
        lambda: db.loadSystemVerilog([], flist=""),
        ValueError,
        "flist must not be empty"),
      (
        lambda: db.loadSystemVerilog([""]),
        ValueError,
        r"files\[0\] must not be empty"),
      (
        lambda: db.loadSystemVerilog([]),
        ValueError,
        "provide at least one files path or flist"),
    )

    for invalid_call, exception_type, message in invalid_calls:
      with self.subTest(message=message):
        with self.assertRaisesRegex(exception_type, message):
          invalid_call()

  def testVerilogNoAssigns(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "test0.v")]
    primitives = [os.path.join(formats_path, "liberty", "benchmarks", "asap7_excerpt", "test0.lib")]
    db.loadLibertyPrimitives(primitives)
    db.loadGateLevelVerilog(verilogs, keep_assigns=False)
    with self.assertRaisesRegex(
        TypeError,
        r"(?:unexpected keyword argument 'keep_assign'.*keep_assigns|"
        r"'keep_assign' is an invalid keyword argument for NLDB\.loadGateLevelVerilog\(\))"):
      db.loadGateLevelVerilog(verilogs, keep_assign=False)

  def testAssignInstancePartition(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.NamedTemporaryFile("w", suffix=".v", delete=False) as source:
      source.write("module child(input i, output o); assign o = i; endmodule\n")
      source.write(
        "module top(input i, output o); "
        "wire n; child u_child(.i(i), .o(n)); assign o = n; endmodule\n")
      source_path = source.name
    try:
      top = db.loadGateLevelVerilog([source_path])
      instances = list(top.getInstances())
      non_assign_instances = list(top.getNonAssignInstances())
      assign_instances = list(top.getAssignInstances())

      self.assertEqual(
        instances,
        non_assign_instances + assign_instances)
      self.assertTrue(all(not inst.getModel().isAssign() for inst in non_assign_instances))
      self.assertTrue(all(inst.getModel().isAssign() for inst in assign_instances))
      self.assertEqual(1, len(non_assign_instances))
      self.assertEqual(1, len(assign_instances))
    finally:
      os.remove(source_path)

  def testVerilogPreprocessEnabled(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    verilogs = [os.path.join(formats_path, "verilog", "benchmarks", "preprocess_top.v")]

    with self.assertRaises(RuntimeError) as context: db.loadGateLevelVerilog(verilogs)

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadGateLevelVerilog(verilogs, preprocess_enabled=True)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertEqual(1, sum(1 for _ in top.getInstances()))
    inst = top.getInstance("u0")
    self.assertIsNotNone(inst)
    self.assertEqual("child", inst.getModel().getName())
    a = top.getBusTerm("a")
    self.assertIsNotNone(a)
    self.assertEqual(1, a.getMSB())
    self.assertEqual(0, a.getLSB())
    self.assertEqual(2, a.getWidth())

  def testSystemVerilogCompactArithmeticDependencies(self):
    db = naja.NLDB.create(naja.NLUniverse.get())
    with tempfile.TemporaryDirectory() as directory:
      source = os.path.join(directory, "arithmetic.sv")
      with open(source, "w") as stream:
        stream.write("""// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module arithmetic(input logic [4:0] a, b, input logic [79:0] count,
                  output logic [10:0] y, sum, difference, output logic [4:0] z);
  assign y = a * b;
  assign sum = a + b;
  assign difference = a - b;
  assign z = a >> count;
endmodule
""")
      top = db.loadSystemVerilog([source],
          diagnostics_report_path=os.path.join(directory, "diagnostics.log"))
      models = {instance.getModel().getName(): instance.getModel()
                for instance in top.getInstances()
                if instance.getModel().getName().startswith(("naja_mul__", "naja_shift__", "naja_add__", "naja_sub__"))}
      self.assertEqual(4, len(models))
      for name, model in models.items():
        expected = 85 if name.startswith("naja_shift__") else 22
        for output in model.getBusTerm("Y").getBits():
          self.assertEqual(expected, sum(1 for _ in naja.SNLDesign.getCombinatorialInputs(output)))

  def testSystemVerilog(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    sv_file = os.path.join(formats_path, "systemverilog", "benchmarks", "simple", "simple.sv")
    output_path = os.environ.get('SNL_WRAPPING_TEST_PATH')
    self.assertIsNotNone(output_path)
    json_path = os.path.join(output_path, "simple_elaborated_ast_python.json")
    diagnostics_path = os.path.join(output_path, "simple_diagnostics_python.txt")

    top = db.loadSystemVerilog(
      [sv_file],
      elaborated_ast_json_path=json_path,
      diagnostics_report_path=diagnostics_path)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertEqual(3, sum(1 for _ in top.getTerms()))
    self.assertTrue(top.hasSourceLoc())
    source_loc = top.getSourceLoc()
    self.assertIsInstance(source_loc, tuple)
    self.assertEqual(5, len(source_loc))
    self.assertTrue(source_loc[0].endswith("systemverilog/benchmarks/simple/simple.sv"))
    self.assertGreaterEqual(source_loc[1], 1)
    self.assertGreaterEqual(source_loc[2], 1)
    self.assertGreaterEqual(source_loc[3], source_loc[1])
    self.assertGreaterEqual(source_loc[4], 1)
    term = top.getTerm("a")
    self.assertIsNotNone(term)
    self.assertTrue(term.hasSourceLoc())
    self.assertTrue(term.getSourceLoc()[0].endswith("systemverilog/benchmarks/simple/simple.sv"))
    net = top.getNet("y")
    self.assertIsNotNone(net)
    self.assertTrue(net.hasSourceLoc())
    self.assertTrue(net.getSourceLoc()[0].endswith("systemverilog/benchmarks/simple/simple.sv"))
    self.assertTrue(os.path.exists(json_path))
    self.assertTrue(os.path.exists(diagnostics_path))
    self.assertIsNone(naja.liveCompilation())
    self.assertIsNone(naja.astSymbolOf(top))
    with self.assertRaises(RuntimeError):
      naja.astSymbolOf(db)
    with self.assertRaises(ValueError):
      naja.snlObjectsOf(object())

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], keep_ast_link=True)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())
    self.assertIsNotNone(naja.liveCompilation())
    top_symbol = naja.astSymbolOf(top)
    self.assertIsNotNone(top_symbol)
    self.assertIn(top, naja.snlObjectsOf(top_symbol))
    net_symbol = naja.astSymbolOf(top.getNet("y"))
    self.assertIsNotNone(net_symbol)
    self.assertIn(top.getNet("y"), naja.snlObjectsOf(net_symbol))
    term_symbol = naja.astSymbolOf(top.getTerm("a"))
    self.assertIsNotNone(term_symbol)
    self.assertIn(top.getTerm("a"), naja.snlObjectsOf(term_symbol))

    # A correctly named capsule whose pointer is unknown to the live registry
    # is valid input and produces an empty result.
    import ctypes
    capsule_new = ctypes.pythonapi.PyCapsule_New
    capsule_new.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p]
    capsule_new.restype = ctypes.py_object
    capsule_name = b"naja.frontend.Symbol"
    unknown_symbol = capsule_new(ctypes.c_void_p(1), capsule_name, None)
    self.assertEqual([], naja.snlObjectsOf(unknown_symbol))

    db.destroy()
    self.assertIsNone(naja.liveCompilation())
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], keep_assigns=False)
    self.assertIsNotNone(top)

    db.destroy()
    db = naja.NLDB.create(u)
    flist_path = os.path.join(output_path, "simple_systemverilog_python.f")
    with open(flist_path, "w", encoding="utf-8") as flist:
      flist.write(f"{sv_file}\n")
    top = db.loadSystemVerilog([], flist=flist_path)
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())

    db.destroy()
    db = naja.NLDB.create(u)
    top = db.loadSystemVerilog([sv_file], suppress_warnings=["width-trunc"])
    self.assertIsNotNone(top)
    self.assertEqual("top", top.getName())

    db.destroy()
    db = naja.NLDB.create(u)
    with tempfile.NamedTemporaryFile("w", suffix=".sv", delete=False) as defineFile:
      defineFile.write("`ifdef SYNTHESIS\n")
      defineFile.write("module synth_top(input logic a, output logic y);\n")
      defineFile.write("  assign y = a;\n")
      defineFile.write("endmodule\n")
      defineFile.write("`else\n")
      defineFile.write("module sim_top(input logic a, output logic y);\n")
      defineFile.write("  assign y = ~a;\n")
      defineFile.write("endmodule\n")
      defineFile.write("`endif\n")
      definePath = defineFile.name
    try:
      top = db.loadSystemVerilog([definePath], defines=["SYNTHESIS"])
      self.assertIsNotNone(top)
      self.assertEqual("synth_top", top.getName())
    finally:
      if os.path.exists(definePath):
        os.remove(definePath)

  def testSystemVerilogTypedExceptions(self):
    self.assertTrue(issubclass(naja.SystemVerilogError, RuntimeError))
    self.assertTrue(issubclass(
      naja.SystemVerilogSyntaxError, naja.SystemVerilogError))
    self.assertTrue(issubclass(
      naja.SystemVerilogUnsupportedError, naja.SystemVerilogError))
    self.assertTrue(issubclass(
      naja.SystemVerilogInternalError, naja.SystemVerilogError))

    u = naja.NLUniverse.get()
    with tempfile.TemporaryDirectory() as tempdir:
      syntax_path = os.path.join(tempdir, "typed_syntax_error.sv")
      with open(syntax_path, "w", encoding="utf-8") as source:
        source.write(
          "module typed_syntax_error(input logic a, output logic y);\n"
          "  assign y = a\n"
          "endmodule\n")

      db = naja.NLDB.create(u)
      with self.assertRaises(naja.SystemVerilogSyntaxError) as context:
        db.loadSystemVerilog([syntax_path])
      syntax_error = context.exception
      self.assertIsInstance(syntax_error, RuntimeError)
      self.assertIn("Error while parsing SystemVerilog: ", str(syntax_error))
      self.assertTrue(syntax_error.diagnostics)
      diagnostic = syntax_error.diagnostics[0]
      self.assertEqual("error", diagnostic["severity"])
      self.assertTrue(diagnostic["file"].endswith("typed_syntax_error.sv"))
      self.assertGreater(diagnostic["line"], 0)
      self.assertGreater(diagnostic["column"], 0)
      self.assertTrue(diagnostic["message"])
      db.destroy()

      unsupported_path = os.path.join(tempdir, "typed_unsupported.sv")
      with open(unsupported_path, "w", encoding="utf-8") as source:
        source.write("""\
module typed_unsupported(input logic a, output logic y);
  task do_check(input logic value);
  endtask
  initial begin
    do_check(a);
  end
  assign y = a;
endmodule
""")

      db = naja.NLDB.create(u)
      with self.assertRaises(naja.SystemVerilogUnsupportedError) as context:
        db.loadSystemVerilog([unsupported_path])
      unsupported_error = context.exception
      self.assertIsInstance(unsupported_error, RuntimeError)
      self.assertIn("Unsupported SystemVerilog elements encountered", str(unsupported_error))
      self.assertTrue(unsupported_error.unsupported_elements)
      unsupported = unsupported_error.unsupported_elements[0]
      self.assertIn("Unsupported initial block", unsupported["message"])
      self.assertEqual(
        "sv.unsupported.procedural_block",
        unsupported["code"])
      db.destroy()

      valid_path = os.path.join(tempdir, "typed_internal.sv")
      with open(valid_path, "w", encoding="utf-8") as source:
        source.write("module typed_internal; endmodule\n")

      db = naja.NLDB.create(u)
      with self.assertRaises(ValueError) as context:
        db.loadSystemVerilog([valid_path], diagnostics_report_path="")
      self.assertIn("diagnostics_report_path must not be empty", str(context.exception))
      self.assertIn("pass None to disable", str(context.exception))

      report_directory = os.path.join(tempdir, "report_directory")
      os.mkdir(report_directory)
      with self.assertRaises(naja.SystemVerilogInternalError) as context:
        db.loadSystemVerilog(
          [valid_path], diagnostics_report_path=report_directory)
      self.assertIn(
        "Failed to create diagnostics report file", str(context.exception))
      db.destroy()

  def testSystemVerilogDiagnosticsReportCanBeDisabled(self):
    u = naja.NLUniverse.get()
    with tempfile.TemporaryDirectory() as tempdir:
      sv_path = os.path.join(tempdir, "console_only.sv")
      with open(sv_path, "w", encoding="utf-8") as source:
        source.write("module console_only; endmodule\n")
      original_cwd = os.getcwd()
      try:
        os.chdir(tempdir)
        db = naja.NLDB.create(u)
        top = db.loadSystemVerilog([sv_path], diagnostics_report_path=None)
        self.assertIsNotNone(top)
        self.assertFalse(os.path.exists("naja_sv_diagnostics.log"))
        db.destroy()
      finally:
        os.chdir(original_cwd)

  def testSystemVerilogIntentAPI(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertFalse(naja.intentAvailable())
    with self.assertRaises(RuntimeError):
      naja.intentTypeOf(db)
    with self.assertRaises(RuntimeError):
      naja.intentParametersOf(db)
    with self.assertRaises(RuntimeError):
      naja.intentPackageMember("mini_pkg")

    with tempfile.TemporaryDirectory() as tempdir:
      sv_file = os.path.join(tempdir, "intent_mini.sv")
      with open(sv_file, "w", encoding="utf-8") as fixture:
        fixture.write(INTENT_MINI_SV)

      top = db.loadSystemVerilog([sv_file], keep_ast_link=True)
      self.assertIsNotNone(top)
      self.assertTrue(naja.intentAvailable())

      state_q = top.getNet("state_q")
      self.assertIsNotNone(state_q)
      type_rec = naja.intentTypeOf(state_q)
      self.assertEqual("mini_pkg::state_e", type_rec["type"])
      self.assertEqual("enum", type_rec["canonical_kind"])
      self.assertTrue(type_rec["src"].endswith("intent_mini.sv:29"))
      enum = type_rec["enum"]
      self.assertEqual(2, enum["width"])
      self.assertTrue(enum["decl"].endswith("intent_mini.sv:3"))
      self.assertEqual(
        {"ST_IDLE": "2'b00", "ST_RUN": "2'b01", "ST_DONE": "2'b11"},
        {m["name"]: m["encoding"] for m in enum["members"]})

      clk_rec = naja.intentTypeOf(top.getNet("clk"))
      self.assertEqual("logic", clk_rec["type"])
      self.assertEqual("scalar", clk_rec["canonical_kind"])
      self.assertTrue(clk_rec["src"].endswith("intent_mini.sv:23"))
      self.assertNotIn("enum", clk_rec)
      self.assertNotIn("struct", clk_rec)
      self.assertEqual(clk_rec, naja.intentTypeOf(top.getTerm("clk")))

      byte_rec = naja.intentTypeOf(top.getNet("byte_q"))
      self.assertEqual("mini_pkg::byte_t", byte_rec["type"])
      self.assertEqual("packed_array", byte_rec["canonical_kind"])
      self.assertTrue(byte_rec["src"].endswith("intent_mini.sv:30"))
      self.assertNotIn("enum", byte_rec)
      self.assertNotIn("struct", byte_rec)

      plain_rec = naja.intentTypeOf(top.getNet("plain_q"))
      self.assertEqual("logic[3:0]", plain_rec["type"])
      self.assertEqual("packed_array", plain_rec["canonical_kind"])
      self.assertTrue(plain_rec["src"].endswith("intent_mini.sv:31"))
      self.assertNotIn("enum", plain_rec)
      self.assertNotIn("struct", plain_rec)

      payload_rec = naja.intentTypeOf(top.getNet("payload_q"))
      self.assertEqual("mini_pkg::payload_t", payload_rec["type"])
      self.assertEqual("packed_struct", payload_rec["canonical_kind"])
      self.assertTrue(payload_rec["src"].endswith("intent_mini.sv:32"))
      self.assertNotIn("enum", payload_rec)
      payload = payload_rec["struct"]
      self.assertEqual(11, payload["width"])
      self.assertTrue(payload["decl"].endswith("intent_mini.sv:8"))
      self.assertEqual([
        {"name": "valid", "type": "logic", "msb": 10, "lsb": 10},
        {"name": "data", "type": "mini_pkg::byte_t", "msb": 9, "lsb": 2},
        {"name": "state", "type": "mini_pkg::state_e", "msb": 1, "lsb": 0},
      ], payload["fields"])

      overlay_rec = naja.intentTypeOf(top.getNet("overlay_q"))
      self.assertEqual("mini_pkg::overlay_t", overlay_rec["type"])
      self.assertEqual("packed_union", overlay_rec["canonical_kind"])
      self.assertEqual(8, overlay_rec["struct"]["width"])
      self.assertTrue(overlay_rec["struct"]["decl"].endswith("intent_mini.sv:13"))
      self.assertEqual([
        {"name": "raw", "type": "logic[7:0]", "msb": 7, "lsb": 0},
        {"name": "data", "type": "mini_pkg::byte_t", "msb": 7, "lsb": 0},
      ], overlay_rec["struct"]["fields"])

      synthetic = naja.SNLScalarNet.create(top, "synthetic")
      self.assertIsNone(naja.intentTypeOf(synthetic))
      self.assertIsNone(naja.astSymbolOf(synthetic))
      self.assertIsNone(naja.intentParametersOf(synthetic))

      ff = next(
        inst for inst in top.getInstances()
        if inst.getModel().getName() == "naja_dffrn__w2")
      self.assertEqual(type_rec, naja.intentTypeOf(ff))

      params = naja.intentParametersOf(top)
      self.assertEqual("intent_mini", params["module"])
      self.assertEqual(2, params["count"])
      by_name = {p["name"]: p for p in params["parameters"]}
      self.assertFalse(by_name["DEPTH"]["localparam"])
      self.assertEqual("4", by_name["DEPTH"]["value"])
      self.assertEqual("4", by_name["DEPTH"]["expr"])
      self.assertTrue(by_name["IDX_W"]["localparam"])
      self.assertEqual("2", by_name["IDX_W"]["value"])
      self.assertEqual("$clog2(DEPTH)", by_name["IDX_W"]["expr"])

      plen = naja.intentPackageMember("mini_pkg", "PLEN")
      self.assertEqual("PLEN", plen["name"])
      self.assertEqual("34", plen["value"])
      self.assertEqual("(32 == 32) ? 34 : 56", plen["expr"])
      self.assertTrue(plen["localparam"])

      package_type = naja.intentPackageMember("mini_pkg", "state_e")
      self.assertEqual("mini_pkg::state_e", package_type["type"])
      self.assertEqual("enum", package_type["canonical_kind"])
      self.assertEqual(2, package_type["enum"]["width"])
      self.assertIsNone(naja.intentPackageMember("mini_pkg", "missing"))

      state_sym = naja.astSymbolOf(state_q)
      linked_objects = naja.snlObjectsOf(state_sym)
      self.assertIn(state_q, linked_objects)
      self.assertIn(ff, linked_objects)

    db.destroy()
    self.assertFalse(naja.intentAvailable())

  def testDesignDumpVerilogOptions(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u)
    self.assertIsNotNone(u)
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    sv_file = os.path.join(
      formats_path, "systemverilog", "benchmarks", "simple", "simple.sv")

    top = db.loadSystemVerilog([sv_file])
    self.assertIsNotNone(top)

    with tempfile.TemporaryDirectory() as dump_dir:
      without_rtl_infos = os.path.join(dump_dir, "simple_default_no_rtl_infos.v")
      top.dumpGateLevelVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(without_rtl_infos))
      with open(without_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertNotIn("sv_src_file", dumped_text)

      with_rtl_infos = os.path.join(dump_dir, "simple_with_rtl_infos.v")
      top.dumpGateLevelVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(with_rtl_infos),
        dumpRTLInfosAsAttributes=True)
      with open(with_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertNotIn("sv_src_file", dumped_text)
      self.assertIn('naja_sv_src="', dumped_text)

      with_verbose_rtl_infos = os.path.join(dump_dir, "simple_with_verbose_rtl_infos.v")
      top.dumpGateLevelVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(with_verbose_rtl_infos),
        dumpRTLInfosAsAttributes=True,
        rtlInfoDumpMode="VerboseAttributes")
      with open(with_verbose_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertIn("sv_src_file", dumped_text)
      self.assertNotIn('naja_sv_src="', dumped_text)

      none_rtl_infos = os.path.join(dump_dir, "simple_none_rtl_infos.v")
      top.dumpGateLevelVerilog(
        path=dump_dir,
        top_file_name=os.path.basename(none_rtl_infos),
        dumpRTLInfosAsAttributes=True,
        rtlInfoDumpMode="None")
      with open(none_rtl_infos, "r", encoding="utf-8") as dumped_file:
        dumped_text = dumped_file.read()
      self.assertNotIn("sv_src_file", dumped_text)
      self.assertNotIn('naja_sv_src="', dumped_text)

      with self.assertRaises(RuntimeError) as context:
        top.dumpGateLevelVerilog(
          path=dump_dir,
          top_file_name="simple_invalid_rtl_infos.v",
          dumpRTLInfosAsAttributes=True,
          rtlInfoDumpMode="Invalid")
      self.assertIn("invalid rtlInfoDumpMode", str(context.exception))

  def testDestroy(self):
    u = naja.NLUniverse.get()
    self.assertIsNotNone(u)
    db = naja.NLDB.create(u) 
    self.assertIsNotNone(db)
    db.destroy()

    with self.assertRaisesRegex(
        RuntimeError,
        r"applying a destroy\(\) to a Python object with no Hurricane object attached"):
      db.destroy()

  def testCreationError(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create("ERROR")
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF(u)
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF("./test_verilogError.v")
    primitives = [1]
    designs = [2]
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    liberty_path = os.path.join(formats_path, 'liberty')
    primitivesNoExtension = [os.path.join(liberty_path, "benchmarks/asap7_excerpt/test0")]
    primitivesCorrect = [os.path.join(liberty_path, "benchmarks/asap7_excerpt/test0.lib")]
    primitivesWrongExtension = [os.path.join(liberty_path, "benchmarks/asap7_excerpt/test0.sd")]
    systemverilog_path = os.path.join(formats_path, "systemverilog")
    svFile = os.path.join(systemverilog_path, "benchmarks/simple/simple.sv")
    missingSvFile = os.path.join(systemverilog_path, "benchmarks/missing/missing.sv")
    with tempfile.NamedTemporaryFile("w", suffix=".sv", delete=False) as multiTopFile:
      multiTopFile.write("module top_a; endmodule\n")
      multiTopFile.write("module top_b; endmodule\n")
      multiTopPath = multiTopFile.name
    with self.assertRaises(TypeError): db.loadGateLevelVerilog("Error", "Error")
    with self.assertRaises(TypeError): db.loadSystemVerilog()
    with self.assertRaises(RuntimeError) as context:
      db.loadSystemVerilog([missingSvFile])
    self.assertIn("Error while parsing SystemVerilog:", str(context.exception))
    try:
      with self.assertRaises(RuntimeError) as context:
        db.loadSystemVerilog([multiTopPath])
      self.assertIn("No top design was found after parsing systemverilog", str(context.exception))
    finally:
      if os.path.exists(multiTopPath):
        os.remove(multiTopPath)
    with self.assertRaisesRegex(
        TypeError, r"files must be a list\[str\], got str"):
      db.loadSystemVerilog("Error")
    with self.assertRaisesRegex(
        TypeError, r"files\[0\] must be a str path, got int"):
      db.loadSystemVerilog(designs)
    with self.assertRaisesRegex(
        TypeError, r"files\[0\] must be a str path, got int"):
      db.loadSystemVerilog([1])
    with self.assertRaisesRegex(TypeError, "elaborated_ast_json_path.*got int"):
      db.loadSystemVerilog([svFile], elaborated_ast_json_path=1)
    with self.assertRaisesRegex(TypeError, "diagnostics_report_path.*got int"):
      db.loadSystemVerilog([svFile], diagnostics_report_path=1)
    with self.assertRaisesRegex(TypeError, r"defines must be a list\[str\].*got int"):
      db.loadSystemVerilog([svFile], defines=1)
    with self.assertRaisesRegex(TypeError, r"defines\[0\].*got int"):
      db.loadSystemVerilog([svFile], defines=[1])
    with self.assertRaisesRegex(TypeError, r"suppress_warnings must be a list\[str\].*got int"):
      db.loadSystemVerilog([svFile], suppress_warnings=1)
    with self.assertRaisesRegex(TypeError, r"suppress_warnings\[0\].*got int"):
      db.loadSystemVerilog([svFile], suppress_warnings=[1])
    with self.assertRaisesRegex(TypeError, "flist.*got int"):
      db.loadSystemVerilog([svFile], flist=1)
    with self.assertRaises(TypeError): db.loadLibertyPrimitives("Error", "Error")
    with self.assertRaisesRegex(
        TypeError, r"NLDB\.loadGateLevelVerilog: files must be a list\[str\], got str"):
      db.loadGateLevelVerilog("Error")
    with self.assertRaisesRegex(
        TypeError, r"NLDB\.loadLibertyPrimitives: files must be a list\[str\], got str"):
      db.loadLibertyPrimitives("Error")
    with self.assertRaisesRegex(
        TypeError, r"files\[0\] must be a str path, got int"):
      db.loadLibertyPrimitives(primitives)
    with self.assertRaisesRegex(
        TypeError, r"NLDB\.loadGateLevelVerilog: files\[0\] must be a str path, got int"):
      db.loadGateLevelVerilog(designs)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesNoExtension)
    with self.assertRaises(RuntimeError) as context: db.loadLibertyPrimitives(primitivesWrongExtension)
    u.destroy()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.create(u)
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF()
    with self.assertRaises(RuntimeError) as context: naja.NLDB.loadNajaIF("./error")

  def testDumpError(self):
    u = naja.NLUniverse.get()
    db = naja.NLDB.create(u) 
    with self.assertRaises(RuntimeError) as context: db.dumpNajaIF()
    with self.assertRaises(RuntimeError) as context: db.dumpNajaIF(db)

if __name__ == '__main__':
  faulthandler.enable()
  unittest.main()
