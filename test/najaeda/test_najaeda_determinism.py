# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Cross-process determinism contract shared by CMake, Bazel and stress CI."""

import difflib
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SYSTEMVERILOG = """
module leaf(input logic [4:0] a, b, input logic [1:0] op,
            output logic [4:0] y, z, output logic flag);
  function automatic logic [4:0] adjust(input logic [4:0] v);
    logic [4:0] first, second;
    first = v; second = ~v;
    if (v[0]) begin first = v ^ 5'd9; second = v + 5'd3; end
    else begin first = v + 5'd7; second = v ^ 5'd6; end
    adjust = first ^ second;
  endfunction
  typedef logic [4:0] word_t;
  function automatic word_t permute(input word_t v);
    word_t result;
    for (int k = 0; k < 5; k++) result[k] = v[(k+4)%5];
    return word_t'(result);
  endfunction
  function automatic logic [4:0] rotate(input logic [4:0] v);
    return permute(v);
  endfunction
  logic [4:0] first, second, third;
  always_comb begin
    first = a; second = b; third = a ^ b; flag = 0;
    case (op)
      0: begin
        first = adjust(a); second = rotate(b); third = adjust(b);
        if (a[1]) begin first = b; second = a; third = 5'd11; flag = 1; end
      end
      1: begin
        first = rotate(a); second = adjust(b); third = rotate(b);
        if (b[2]) begin first = 5'd13; second = a; third = b; flag = 1; end
        else begin first = b; second = 5'd17; third = a; end
      end
      default: begin
        first = adjust(b); second = adjust(a); third = rotate(a);
      end
    endcase
    y = (first * second) ^ (third >> op); z = third;
    if (b !=? {2'b?1, 3'bz0x}) y = second;
    for (int k = 0; k < 5; k++) z[k] = third[k] ^ first[k];
    z *= second;
    z -= first;
    y >>= op;
    if (first < second) z[{3'b0, op}] = ^(first + second);
    if (a ==? 5'b1?0xz) z = first;
  end
endmodule
module stateful(input logic clk, rst, enable, input logic [4:0] a, b,
                output logic [4:0] q, r, s);
  always_ff @(posedge clk) begin
    if (rst) q <= 0;
    else begin
      q <= a;
      r <= b;
      if (enable) s <= a ^ b;
    end
  end
endmodule
module storage(input logic clk_a, clk_b, we_a, we_b,
               input logic [1:0] addr_a, addr_b, input logic [2:0] a, b,
               output logic [2:0] q, r);
  logic [2:0] first [4], second [4];
  always @(posedge clk_a) begin
    if (we_a) begin first[addr_a] <= a; second[addr_a] <= b; end
    q <= first[addr_a];
  end
  always @(posedge clk_b) begin
    if (we_b) begin first[addr_b] <= b; second[addr_b] <= a; end
    r <= second[addr_b];
  end
endmodule
module top(input logic [9:0] a, b, input logic [1:0] op,
           input logic clk_a, clk_b, rst, enable,
           output wire [9:0] y, z, output wire [1:0] flag,
           output wire [4:0] q, r, s, output wire [2:0] mem_q, mem_r);
  stateful regs(clk_a, rst, enable, a[4:0], b[4:0], q, r, s);
  storage mems(clk_a, clk_b, enable, rst, op, op,
               a[2:0], b[2:0], mem_q, mem_r);
  for (genvar g = 0; g < 2; g++) begin: lanes
    leaf u(a[g*5+:5], b[g*5+:5], op, y[g*5+:5], z[g*5+:5], flag[g]);
  end
endmodule
"""

VERILOG = """
(* model_tag = "leaf" *)
module leaf(input [2:0] a, output [2:0] y);
  assign y = {a[1:0], a[2]};
endmodule
(* design_tag = "top", revision = 7, marker *)
module top((* port_tag = "input" *) input [5:0] a, output [5:0] y);
  (* net_tag = "lanes" *) wire [2:0] first, second;
  (* instance_tag = "first" *) leaf u0(.a(a[2:0]), .y(first));
  leaf u1(.a(a[5:3]), .y(second));
  assign y = {second, first};
endmodule
"""

VHDL = """
entity leaf is port(a : in bit_vector(2 downto 0);
                    y : out bit_vector(2 downto 0)); end;
architecture rtl of leaf is begin y <= not a; end;
library ieee; use ieee.numeric_std.all;
entity scale is port(a : in bit; y : out bit); end;
architecture rtl of scale is begin
  process(all) variable lane : integer range 0 to 1;
  begin
    lane := 0;
    if a='1' then lane := 1; end if;
    if lane*10=10 then y <= '1'; else y <= '0'; end if;
  end process;
end;
entity top is port(a : in bit_vector(5 downto 0);
                   y : out bit_vector(5 downto 0); flag : out bit); end;
architecture rtl of top is begin
  scalar: entity work.scale port map(a(0),flag);
  lanes: for g in 0 to 1 generate
    u: entity work.leaf port map(a(g*3+2 downto g*3), y(g*3+2 downto g*3));
  end generate;
end;
"""


def dump_state(output):
    from najaeda import naja

    # Capture the live database before invoking any dumper. A dumper-only sort
    # cannot repair unstable creation order, IDs, names or connectivity.
    state = []

    def attributes(obj):
        return {
            "attributes": [[a.getName(), a.getValue(), a.hasValue()] for a in obj.getAttributes()],
            "source_location": obj.getSourceLoc(),
            "source_declaration": obj.getSourceDeclaration(),
            "source_origins": obj.getSourceOrigins(),
        }

    def visit(library):
        for design in library.getSNLDesigns():
            state.append([
                design.getNLID().toTuple(), design.getName(), attributes(design),
                [[p.getName(), p.getValue()] for p in design.getParameters()],
                [[i.getNLID().toTuple(), i.getName(), attributes(i),
                  i.getModel().getNLID().toTuple(),
                  [[p.getName(), p.getValue()] for p in i.getInstParameters()],
                  [[t.getNLID().toTuple(), attributes(t),
                    t.getNet().getNLID().toTuple() if t.getNet() else None]
                   for t in i.getInstTerms()]] for i in design.getInstances()],
                [[n.getNLID().toTuple(), n.getName(), attributes(n)] for n in design.getNets()],
                [[t.getNLID().toTuple(), t.getName(), attributes(t)] for t in design.getTerms()],
                [[t.getNLID().toTuple(), attributes(t)] for t in design.getBitTerms()],
                [[n.getNLID().toTuple(), attributes(n)] for n in design.getBitNets()],
            ])
        for child in library.getLibraries():
            visit(child)

    for db in naja.NLUniverse.get().getUserDBs():
        for library in db.getLibraries():
            visit(library)
    output.mkdir()
    (output / "state.json").write_text(json.dumps(state, indent=2), encoding="utf-8")
    top = naja.NLUniverse.get().getTopDesign()
    top.dumpGateLevelVerilog(str(output), "design.v")
    top.dumpFullDotFile(str(output / "design.dot"))
    top.getDB().dumpNajaIF(str(output / "snapshot"))


def worker(kind, source, output, run):
    # Keep differently sized allocations alive across native elaboration, in
    # addition to fresh-process ASLR and different Python hash seeds.
    allocations = [bytearray(512 + (i * 137) % 8192) for i in range(256 + int(run) * 17)]
    from najaeda import naja

    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    universe.setTopDB(db)
    if kind == "sv":
        db.loadSystemVerilog([source], diagnostics_report_path=None)
    elif kind == "v":
        db.loadGateLevelVerilog([source])
        from najaeda import naja
        top = naja.NLUniverse.get().getTopDesign()
        instance = top.getInstance("u0")
        # Exercise sparse bit/terminal records without source metadata, as well
        # as ordered duplicates and typed values constructed directly in Python.
        for obj in (top.getTerm("a").getBusTermBit(1), top.getNet("first").getBit(1),
                    instance.getInstTerm(instance.getModel().getTerm("a").getBusTermBit(1))):
            obj.addAttribute(naja.SNLAttribute("tag", "23"))
            obj.addAttribute(naja.SNLAttribute("tag", 23))
            obj.addAttribute(naja.SNLAttribute("marker"))
    elif kind == "vhd":
        db.loadVHDL(source, top="top", diagnostics_report_path=None)
    else:
        raise ValueError(kind)
    output = Path(output)
    dump_state(output / "original")
    universe.destroy()
    naja.NLDB.loadNajaIF(str(output / "original" / "snapshot"))
    dump_state(output / "reloaded")
    assert allocations  # Retain heap perturbation until both dumps are complete.


def artifacts(directory):
    result = {}
    for path in sorted(directory.rglob("*")):
        if not path.is_file():
            continue
        data = path.read_bytes()
        if path.suffix == ".v":
            lines = data.splitlines(keepends=True)
            # Only the date line in the standard banner is volatile. Keep all
            # other comments, metadata, whitespace and line endings in the check.
            if len(lines) > 1 and lines[0].rstrip(b"\r\n") == b"/" * 80:
                if lines[1].startswith(b"// "):
                    lines[1] = b"// <timestamp>\n"
            data = b"".join(lines)
        result[path.relative_to(directory).as_posix()] = data
    return result


class NajaEDADeterminismTest(unittest.TestCase):
    def assert_artifacts_equal(self, expected, actual):
        self.assertEqual(expected.keys(), actual.keys(), "artifact set changed")
        for name in expected:
            if expected[name] == actual[name]:
                continue
            message = (
                f"{name} differs: SHA-256 "
                f"{hashlib.sha256(expected[name]).hexdigest()} != "
                f"{hashlib.sha256(actual[name]).hexdigest()}")
            if name.endswith((".json", ".txt", ".v", ".dot", ".mf")):
                message += "\n" + "".join(difflib.unified_diff(
                    expected[name].decode().splitlines(keepends=True),
                    actual[name].decode().splitlines(keepends=True),
                    fromfile="expected/" + name, tofile="actual/" + name))[:4000]
            self.fail(message)

    def check_determinism(self, kind, source, files=None):
        runs = int(os.environ.get("NAJA_DETERMINISM_RUNS", "5"))
        self.assertGreaterEqual(runs, 5, "determinism requires at least five fresh processes")
        with tempfile.TemporaryDirectory(dir=os.environ.get("NAJAEDA_TEST_PATH")) as directory:
            directory = Path(directory)
            source_path = directory / ("source." + kind)
            for name, contents in (files or {}).items():
                (directory / name).write_text(contents, encoding="utf-8")
            source_path.write_text(source.replace("__DATA_DIRECTORY__", str(directory)), encoding="utf-8")
            baseline = None
            for run in range(runs):
                output = directory / str(run)
                output.mkdir()
                environment = os.environ.copy()
                environment["PYTHONHASHSEED"] = str(run)
                result = subprocess.run(
                    [sys.executable, str(Path(__file__).resolve()), "--worker",
                     kind, str(source_path), str(output), str(run)],
                    env=environment, capture_output=True, text=True, timeout=120)
                self.assertEqual(0, result.returncode, result.stdout + result.stderr)
                original = artifacts(output / "original")
                for name in ("state.json", "design.v", "design.dot", "snapshot/snl.mf",
                             "snapshot/db_implementation.snl", "snapshot/db_interface.snl",
                             "snapshot/db0_interface.snl"):
                    self.assertIn(name, original)
                with self.subTest(language=kind, run=run, phase="snapshot round trip"):
                    self.assert_artifacts_equal(original, artifacts(output / "reloaded"))
                # Reload in a process which has never elaborated this design.
                fresh = subprocess.run(
                    [sys.executable, str(Path(__file__).resolve()), "--load-worker",
                     str(output / "original" / "snapshot"), str(output / "fresh")],
                    env=environment, capture_output=True, text=True, timeout=120)
                self.assertEqual(0, fresh.returncode, fresh.stdout + fresh.stderr)
                with self.subTest(language=kind, run=run, phase="fresh-process reload"):
                    self.assert_artifacts_equal(original, artifacts(output / "fresh"))
                if baseline is None:
                    baseline = original
                else:
                    with self.subTest(language=kind, run=run, phase="fresh process"):
                        self.assert_artifacts_equal(baseline, original)

    def test_canonical_db0_families(self):
        binary = os.environ.get("NAJA_DB0_AUDIT_BINARY")
        self.assertTrue(binary, "Set NAJA_DB0_AUDIT_BINARY to the snlCapnPTests executable")
        runs = int(os.environ.get("NAJA_DETERMINISM_RUNS", "5"))
        self.assertGreaterEqual(runs, 5)
        with tempfile.TemporaryDirectory(dir=os.environ.get("NAJAEDA_TEST_PATH")) as scratch:
            baseline = None
            for run in range(runs):
                output = Path(scratch) / str(run)
                environment = os.environ.copy()
                environment.pop("NAJA_DB0_AUDIT_INPUT", None)
                environment.pop("XML_OUTPUT_FILE", None)
                environment.pop("GTEST_OUTPUT", None)
                environment.update(NAJA_DB0_AUDIT_OUTPUT=str(output), NAJA_DB0_AUDIT_RUN=str(run))
                command = [binary, "--gtest_filter=SNLCapnPPrimitiveIdentityTest.AllFamiliesArtifacts"]
                result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=120)
                self.assertEqual(0, result.returncode, result.stdout + result.stderr)
                original = artifacts(output / "original")
                self.assertIn("state.txt", original)
                self.assertIn("design.dot", original)
                self.assertIn("design.v", original)
                self.assertIn("snapshot/db0_interface.snl", original)
                self.assert_artifacts_equal(original, artifacts(output / "reloaded"))
                environment.update(NAJA_DB0_AUDIT_INPUT=str(output / "original" / "snapshot"),
                                   NAJA_DB0_AUDIT_OUTPUT=str(output / "fresh"))
                result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=120)
                self.assertEqual(0, result.returncode, result.stdout + result.stderr)
                self.assert_artifacts_equal(original, artifacts(output / "fresh"))
                if baseline is None:
                    baseline = original
                else:
                    self.assert_artifacts_equal(baseline, original)

    def test_systemverilog(self):
        self.check_determinism("sv", SYSTEMVERILOG)

    def test_systemverilog_inlined_loop_counters(self):
        self.check_determinism("sv", """
module leaf(input logic [11:0] a, input logic [1:0] mode,
            output logic [11:0] y);
  function automatic logic [11:0] f(input logic [11:0] data, input logic [1:0] sel);
    logic [11:0] result;
    case(sel)
      0: for(int index=0;index<3;index++) result[index*4+:4]=data[3:0];
      1: for(int index=0;index<2;index++) result[index*6+:6]=data[5:0];
      default: result=data;
    endcase
    return result;
  endfunction
  assign y=f(a,mode);
endmodule
module top(input logic [11:0] a, input logic [1:0] mode,
           output logic [11:0] y,z);
  leaf u(a,mode,y); leaf v(a,mode,z);
endmodule
""")

    def test_systemverilog_repeated_mux_selections(self):
        self.check_determinism("sv", """
module leaf(input logic [2:0] a,b, input logic s,t,u,
            output logic [2:0] y,z,shared,dual);
  always_comb begin
    y=a; if(s)y=b; if(t)y=b; if(u)y=b;
  end
  assign shared=s ? a : b;
  assign z=t ? a : shared;
  assign dual=u ? (t ? a : b) : b;
endmodule
module top(input logic [2:0] a,b, input logic s,t,u,
           output logic [2:0] y0,y1,z0,z1,q0,q1,r0,r1);
  leaf x(a,b,s,t,u,y0,z0,q0,r0);
  leaf y(a,b,s,t,u,y1,z1,q1,r1);
endmodule
""")

    def test_systemverilog_overwritten_mux_results(self):
        self.check_determinism("sv", """
module leaf(input logic [3:0] a,b, input logic s,
            output logic [3:0] y,z);
  always_comb begin
    y=s?a:b;
    y=a;
    z=s?b:a;
  end
endmodule
module top(input logic [3:0] a,b, input logic s,
           output logic [3:0] y0,y1,z0,z1);
  leaf x(a,b,s,y0,z0);
  leaf y(a,b,s,y1,z1);
endmodule
""")

    def test_systemverilog_compact_case_and_concat_replay(self):
        self.check_determinism("sv", """
module top(input logic [7:0] a, output logic [1:0] hi,
           output logic [3:0] lo, output logic [5:0] choice,
           output wire [7:0] first, second);
  always_comb begin
    {hi,lo} = a[5:0];
    if(a[7]) {hi,lo} = a[5:0] >> 1;
    else if(a[6]) {hi,lo} = a[5:0] << 1;
    case(a[7:6])
      0: choice=a[5:0]; 1: choice=a[5:0]; 2: choice=a[5:0];
      default: choice=0;
    endcase
  end
  assign first=a[7] ? 8'd2 : 8'd1;
  assign second=a[7] ? 8'd2 : 8'd1;
endmodule
""")

    def test_systemverilog_shared_power_and_table_expressions(self):
        self.check_determinism("sv", """
module leaf(input logic [31:0] exponent, input logic [1:0] index,
            output logic [31:0] first, duplicate, adjusted,
            output logic [7:0] left, right);
  function automatic logic [7:0] lookup(input logic [1:0] i);
    localparam logic [0:3][7:0] words = {8'd3, 8'd5, 8'd9, 8'd13};
    return words[i];
  endfunction
  assign first = 32'd2 ** exponent;
  assign duplicate = 32'd2 ** exponent;
  assign adjusted = 32'd2 ** (exponent + 32'd1);
  assign left = lookup(index);
  assign right = lookup(index);
endmodule
module top(input logic [31:0] exponent, input logic [1:0] index,
           output logic [31:0] first, duplicate, adjusted,
           output logic [7:0] left, right,
           output logic [31:0] second, third, fourth,
           output logic [7:0] a, b);
  leaf x(exponent, index, first, duplicate, adjusted, left, right);
  leaf y(exponent, index, second, third, fourth, a, b);
endmodule
""")

    def test_systemverilog_function_input_copy_returns(self):
        from test_najaeda_function_replay import SYSTEMVERILOG as function_source
        self.check_determinism("sv", function_source)

    def test_systemverilog_function_case_returns(self):
        from test_najaeda_function_replay import wide_case_fixture
        source, _ = wide_case_fixture()
        self.check_determinism("sv", source)

    def test_systemverilog_scoped_tables_and_copyouts(self):
        self.check_determinism("sv", """
module top(input logic [4:0] a, output logic [3:0] y,
           output logic [2:0] partial, output logic signed [4:0] quotient);
  function automatic logic [3:0] lookup(input logic [4:0] value);
    case (value[4])
      0: begin
        logic [2:0] words [0:2];
        words = '{3'd2, 3'd5, 3'd7};
        return words[value[2:0] % 3];
      end
      default: begin
        logic [3:0] words [4:0];
        words = '{4'd1, 4'd6, 4'd9, 4'd11, 4'd14};
        return words[value[2:0] % 5];
      end
    endcase
  endfunction
  function automatic void copy(input logic [2:0] v,
                               output logic [3:0] first, second);
    first = v;
    second = first + 1;
  endfunction
  function automatic logic [2:0] update(input logic [4:0] v);
    update[v[1:0]] = v[2];
  endfunction
  always_comb begin
    automatic logic [3:0] left, right;
    copy(a, left, right);
    y = lookup(a) ^ left ^ right;
  end
  assign partial = update(a);
  assign quotient = $signed(a) / 2;
endmodule
""")

    def test_systemverilog_fixed_array_function_returns(self):
        self.check_determinism("sv", """
module top(input logic [3:0] a, output wire [23:0] y, z,
           output wire [7:0] constant_y, output wire unknown_x, unknown_z,
           output wire partial_x, partial_zero, output wire [3:0] partial_written);
  typedef logic [3:0] table_t [2:3][5:3];
  typedef logic [3:0] cube_t [1:0][2:3][4:3];
  function automatic int row_limit();
    int n=2;
    while (n<4) n++;
    return n;
  endfunction
  function automatic table_t make_table(input logic [3:0] v);
    table_t result;
    for (int r=2; r<row_limit(); r++)
      for (int c=5; c>=3; c--) result[r][c] = v + 1 + 3*(r-2) + 5-c;
    return result;
  endfunction
  function automatic table_t named_table(input logic [3:0] v);
    for (int r=2; r<row_limit(); r++)
      for (int c=5; c>=3; c--) named_table[r][c] = v + 1 + 3*(r-2) + 5-c;
  endfunction
  function automatic cube_t make_cube();
    cube_t result;
    int i=0;
    while (i<2) begin
      for (int j=2; j<=3; j++)
        for (int k=4; k>=3; k--) result[i][j][k] = 3*i + 2*j + k;
      i++;
    end
    return result;
  endfunction
  function automatic table_t unknown_table(input logic unused);
    table_t result;
    for (int r=2; r<=3; r++)
      for (int c=5; c>=3; c--) result[r][c] = 4'b10xz;
    return result;
  endfunction
  function automatic table_t partial_table(input logic [3:0] v);
    table_t result;
    result[2][5] = v;
    return result;
  endfunction
  typedef bit [3:0] bit_table_t [2:3][5:3];
  function automatic bit_table_t partial_bits(input bit [3:0] v);
    partial_bits[2][5] = v;
  endfunction
  table_t first, second, constants;
  table_t partial;
  bit_table_t bit_values;
  cube_t cube;
  assign first=make_table(a);
  always_comb second=named_table(a);
  assign cube=make_cube();
  assign constants=unknown_table(1'b0);
  assign partial=partial_table(a);
  assign bit_values=partial_bits(a);
  assign y={first[2][5],first[2][4],first[2][3],first[3][5],first[3][4],first[3][3]};
  assign z={second[2][5],second[2][4],second[2][3],second[3][5],second[3][4],second[3][3]};
  assign constant_y={cube[1][2][4],cube[0][3][3]};
  assign unknown_x=constants[2][3][1];
  assign unknown_z=constants[3][5][0];
  assign partial_written=partial[2][5];
  assign partial_x=partial[3][4][0];
  assign partial_zero=bit_values[3][4][0];
endmodule
""")

    def test_systemverilog_indexed_vector_updates(self):
        self.check_determinism("sv", """
module top(input logic [3:0] index, input logic [3:0] data,
           input logic [15:0] base, output logic [15:0] y);
  always_comb begin
    y = base;
    y[index] = data[0];
    y[index +: 4] = data;
    y[index +: 2] = y[index +: 2] ^ data[1:0];
  end
endmodule
""")

    def test_systemverilog_nested_packed_writes(self):
        self.check_determinism("sv", """
module top(input logic clk, rst_n, enable,
           input logic signed [5:0] row, col, lane,
           input logic [1:0][2:0] indices,
           input logic [3:0] data,
           output logic [1:0][7:0][3:0] q,
           output logic [2:3][-2:0][4:3][3:0] r);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin q <= '0; r <= '0; end
    else if (enable) begin
      q[row][indices[row]] <= data;
      q[row][col] <= ~data;
      r[row][col][lane] <= data;
    end
  end
endmodule
""")

    def test_systemverilog_shared_inversions(self):
        self.check_determinism("sv", """
module top(input logic [3:0] a, output logic [3:0] y, z);
  for (genvar k = 0; k < 4; k++) begin
    always_comb begin
      casez (a)
        4'b00??: y[k] = a[k];
        4'b?010: y[k] = ~a[k];
        default: y[k] = 1'b0;
      endcase
    end
  end
  assign z = (~a & 4'ha) | (~a & 4'h5);
endmodule
""")

    def test_systemverilog_wide_index_reads(self):
        self.check_determinism("sv", """
module top(input logic [31:0] index,
           input logic [63:0] data,
           input logic signed [5:0] signed_index,
           output logic [7:0] selected,
           output logic signed_bit);
  for (genvar k = 0; k < 8; k++) begin
    assign selected[k] = data[index * 8 + k];
  end
  assign signed_bit = data[signed_index];
endmodule
""")

    def test_systemverilog_indexed_vector_selects(self):
        self.check_determinism("sv", """
module top(input logic signed [79:0] index,
           input logic [15:0] data, input logic [0:15] ascending_data,
           output logic [3:0] up, down, ascending_slice);
  assign up = data[index +: 4];
  assign down = data[index -: 4];
  assign ascending_slice = ascending_data[index +: 4];
endmodule
""")

    def test_systemverilog_selected_sequential_arithmetic(self):
        self.check_determinism("sv", """
module top(input logic clk, rst, enable,
           output logic [4:0] a, b, c);
  always_ff @(posedge clk) begin
    if (rst) begin a <= '0; b <= '0; c <= '0; end
    else if (enable) begin
      a[4:0] <= a + 1'b1;
      b[3:1] <= 1'b1 + b;
      c[2:2] <= c - 1'b1;
    end
  end
endmodule
""")

    def test_systemverilog_dynamic_packed_fields(self):
        self.check_determinism("sv", """
package types;
  typedef struct packed {
    logic [1:0] tag;
    logic [4:0] data;
    logic valid;
  } state_t;
endpackage
module top import types::*;
  (input logic clk, rst, input logic [2:0] index,
   input logic [1:0] value, output state_t q);
  always_ff @(posedge clk) q.tag <= value;
  always_ff @(posedge clk) q.valid <= value[0];
  always_ff @(posedge clk or posedge rst) begin
    if (rst) q.data <= '0;
    else q.data[index] <= value[0];
  end
endmodule
""")

    def test_verilog(self):
        self.check_determinism("v", VERILOG)

    def test_systemverilog_interfaces(self):
        self.check_determinism("sv", """
interface lane_if #(parameter W = 3);
  logic [W-1:0] value, result;
  modport endpoint(input value, output result);
endinterface
module endpoint(lane_if.endpoint bus);
  assign bus.result = ~bus.value;
endmodule
module relay(lane_if.endpoint lanes[2:1]);
  for (genvar g = 1; g <= 2; g++) begin: generated
    endpoint worker(lanes[g]);
  end
endmodule
module top(input logic [10:0] a, output wire [10:0] y);
  lane_if lanes[4:3]();
  lane_if #(.W(5)) wide();
  assign lanes[4].value = a[2:0];
  assign lanes[3].value = a[5:3];
  assign wide.value = a[10:6];
  assign y = {wide.result, lanes[3].result, lanes[4].result};
  relay grouped(lanes);
  endpoint wider(wide);
endmodule
""")

    def test_vhdl_repeated_mux_selections(self):
        self.check_determinism("vhd", """
entity leaf is port(a,b : in bit_vector(2 downto 0); s,t,u : in bit;
                    y,dual : out bit_vector(2 downto 0)); end;
architecture rtl of leaf is begin
  process(a,b,s,t,u) is variable temp : bit_vector(2 downto 0);
  begin
    temp:=a;
    if s='1' then temp:=b; end if;
    if t='1' then temp:=b; end if;
    if u='1' then temp:=b; end if;
    y<=temp;
    if u='1' then
      if t='1' then dual<=a; else dual<=b; end if;
    else dual<=b; end if;
  end process;
end;
entity top is port(a,b : in bit_vector(2 downto 0); s,t,u : in bit;
                   y0,y1,q0,q1 : out bit_vector(2 downto 0)); end;
architecture rtl of top is begin
  x: entity work.leaf port map(a,b,s,t,u,y0,q0);
  y: entity work.leaf port map(a,b,s,t,u,y1,q1);
end;
""")

    def test_vhdl_overwritten_mux_results(self):
        self.check_determinism("vhd", """
entity leaf is port(a,b : in bit_vector(3 downto 0); s : in bit;
                    y,z : out bit_vector(3 downto 0)); end;
architecture rtl of leaf is begin
  process(a,b,s) is variable temp : bit_vector(3 downto 0);
  begin
    if s='1' then temp:=a; else temp:=b; end if;
    temp:=a; y<=temp;
    if s='1' then z<=b; else z<=a; end if;
  end process;
end;
entity top is port(a,b : in bit_vector(3 downto 0); s : in bit;
                   y0,y1,z0,z1 : out bit_vector(3 downto 0)); end;
architecture rtl of top is begin
  x: entity work.leaf port map(a,b,s,y0,z0);
  y: entity work.leaf port map(a,b,s,y1,z1);
end;
""")

    def test_vhdl_constant_ports(self):
        self.check_determinism("vhd", """
library ieee; use ieee.std_logic_1164.all;
entity source is port(a : in std_logic; y : out std_logic_vector(5 downto 0)); end;
architecture rtl of source is begin y <= "01XZ" & a & '1'; end;
library ieee; use ieee.std_logic_1164.all;
entity top is port(a : in std_logic; y : out std_logic_vector(5 downto 0)); end;
architecture rtl of top is begin u : entity work.source port map(a, y); end;
""")

    def test_vhdl_memory(self):
        self.check_determinism("vhd", """
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
entity top is port(clk, we : in std_logic;
  addr : in std_logic_vector(1 downto 0);
  d : in std_logic_vector(4 downto 0);
  q : out std_logic_vector(4 downto 0)); end;
architecture rtl of top is
  type words is array (0 to 3) of std_logic_vector(4 downto 0);
  signal storage : words;
begin
  process(clk) begin
    if rising_edge(clk) then
      if we = '1' then storage(to_integer(unsigned(addr))) <= d; end if;
    end if;
  end process;
  q <= storage(to_integer(unsigned(addr)));
end;
""")

    def test_vhdl_indexed_vector_update(self):
        self.check_determinism("vhd", """
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity top is port(index : in unsigned(2 downto 0); data : in bit;
                   base : in bit_vector(7 downto 0); y : out bit_vector(7 downto 0)); end;
architecture rtl of top is begin
  process(all) begin y <= base; y(to_integer(index)) <= data; end process;
end;
""")

    def test_vhdl_file_initialized_memory(self):
        self.check_determinism("vhd", """
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all; use std.textio.all;
entity top is port(clk, en : in std_logic; address : in natural range 4 to 12;
  data : in std_logic_vector(4 downto 0); q : out std_logic_vector(4 downto 0)); end;
architecture rtl of top is
  type words is array(4 to 12) of std_logic_vector(4 downto 0);
  impure function initialize return words is
    file source : text is in "__DATA_DIRECTORY__/words.hex";
    variable line_buffer : line;
    variable word : std_logic_vector(4 downto 0);
    variable result : words := (others => (others => '0'));
  begin
    for i in result'range loop
      exit when endfile(source);
      readline(source, line_buffer); hread(line_buffer, word);
      case i is
        when 4 => result(i) := word;
        when others => result(i) := word;
      end case;
    end loop;
    return result;
  end;
  signal storage : words := initialize;
begin
  process(clk) begin if rising_edge(clk) then
    if en='1' then storage(address) <= data; end if;
    q <= storage(address);
  end if; end process;
end;
""", files={"words.hex": "0d\n12\n01\n"})

    def test_vhdl_soc_language_features(self):
        self.check_determinism("vhd", """
entity leaf is generic(filename : string); port(clk, en : in bit;
  a : in natural range 0 to 255; y : out bit_vector(7 downto 0)); end;
architecture rtl of leaf is
  signal stored : bit_vector(3 downto 0) := "1010";
  alias low : bit_vector(0 to 3) is stored;
begin
  process(clk)
    type phase_t is (idle, active);
    subtype flag_t is bit;
    function invert(v : bit) return bit is begin return not v; end;
    variable phase : phase_t;
    variable v : flag_t;
  begin
    if falling_edge(clk) then
      phase := idle; v := invert(en);
      if a = 255 then phase := active; end if;
      low(0) <= v when phase = active;
    end if;
  end process;
  y <= (3 downto 0 => stored, others => '0');
end;
entity top is port(clk, en : in bit; a : in natural range 0 to 255;
  y : out bit_vector(15 downto 0); flag : out bit); end;
architecture rtl of top is signal held : bit := '1';
begin
  disabled: if false generate held <= en; end generate;
  flag <= held;
  u: entity work.leaf generic map("a/b") port map(clk,en,a,y(7 downto 0));
  v: entity work.leaf generic map("a_2fb") port map(clk,en,a,y(15 downto 8));
end;
""")

    def test_vhdl(self):
        self.check_determinism("vhd", VHDL)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--worker":
        worker(*sys.argv[2:])
    elif len(sys.argv) > 1 and sys.argv[1] == "--load-worker":
        from najaeda import naja
        naja.NLDB.loadNajaIF(sys.argv[2])
        dump_state(Path(sys.argv[3]))
    else:
        unittest.main()
