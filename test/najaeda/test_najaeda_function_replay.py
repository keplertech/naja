# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Behavioral function copy-in / packed member replay through the raw API."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

from najaeda import naja


SYSTEMVERILOG = r"""
package values;
  typedef enum logic [1:0] {Idle, Busy, Done} code_t;
  typedef struct packed {
    logic signed [3:0] payload;
    logic [0:2] mark;
    code_t code;
  } item_t;
  typedef struct packed { logic [1:0] state; logic [3:0] selector; } reply_t;
endpackage
module top(input logic [8:0] a, input logic signed [3:0] mode,
           output wire [8:0] y, z, c0, c1, c2,
           output wire signed [12:0] extended, output wire [5:0] reply);
  import values::*;
  function automatic item_t change(input item_t v, input int op);
    if (op == 2) begin
      v.mark = '1;
      return v;
    end
    unique case (op)
      0: v.mark = {1'b0, v.mark[1:2]};
      1: v.mark = {1'b1, v.mark[1:2]};
    endcase
    return v;
  endfunction
  function automatic item_t nested(input item_t v, input int op);
    v = change(v, op);
    v.payload = v.payload + 4'sd3;
    return change(v, 1);
  endfunction
  function automatic logic signed [8:0] converted(input item_t v, input int op);
    v = change(v, op);
    return signed'(v);
  endfunction
  function automatic reply_t clear_selector(input reply_t v, input int op);
    if (op == 2) begin
      v.selector = '1;
      return v;
    end
    v.selector = {1'b0, v.selector[2:0]};
    return v;
  endfunction
  reply_t reply_value;
  assign reply_value = a[5:0];
  assign reply = clear_selector(reply_value, mode);
  item_t value;
  assign value = a;
  assign y = change(value, mode);
  assign z = nested(value, mode);
  assign c0 = change(value, 0);
  assign c1 = change(value, 1);
  assign c2 = change(value, 2);
  assign extended = converted(value, mode);
endmodule
"""

TESTBENCH = r"""
module tb;
  reg [8:0] a;
  reg signed [3:0] mode;
  wire [8:0] y,z,c0,c1,c2, ry,rz,rc0,rc1,rc2;
  wire signed [12:0] extended, rextended;
  wire [5:0] reply, rreply;
  top dut(a,mode,y,z,c0,c1,c2,extended,reply);
  reference_rtl ref_model(a,mode,ry,rz,rc0,rc1,rc2,rextended,rreply);
  integer i,j;
  initial begin
    for (i=0;i<512;i=i+1) for(j=-8;j<8;j=j+1) begin
      a=i; mode=j; #1;
      if ({y,z,c0,c1,c2,extended,reply} !== {ry,rz,rc0,rc1,rc2,rextended,rreply})
        $fatal(1,"function mismatch a=%h mode=%d y=%h ref=%h z=%h refz=%h c=%h/%h/%h refc=%h/%h/%h ext=%h refext=%h",
               a,mode,y,ry,z,rz,c0,c1,c2,rc0,rc1,rc2,extended,rextended);
    end
    $display("PASS 8192 function input combinations");
    $finish;
  end
endmodule
"""


def wide_case_fixture():
    source = SYSTEMVERILOG.replace("logic [0:2] mark;", "marker_t mark;")
    source = source.replace("  typedef struct packed {",
        "  typedef struct packed { logic [0:2] bits; } marker_t;\n  typedef struct packed {", 1)
    source = source.replace("v.mark", "v.mark.bits")
    source = source.replace("logic signed [3:0] payload", "logic signed [69:0] payload")
    source = source.replace("[8:0]", "[74:0]").replace("[12:0]", "[78:0]")
    dispatch = """
  function automatic item_t dispatch(input item_t v, input int op);
case (op)
  2: begin v.mark.bits = '1; return v; end
  -1: begin
    if (v.payload[0]) begin v.payload = ~v.payload; return v; end
    v.mark.bits = 3'b101;
  end
  3: return change(v, 0);
  default: v.payload = v.payload ^ 70'd37;
endcase
v.payload = v.payload + 70'sd9;
return change(v, op);
  endfunction
"""
    source = source.replace("  item_t value;", dispatch + "  item_t value;")
    source = source.replace("assign y = change(value, mode)", "assign y = dispatch(value, mode)")
    bench = TESTBENCH.replace("[8:0]", "[74:0]").replace("[12:0]", "[78:0]")
    bench = bench.replace("i<512", "i<152")
    bench = bench.replace("a=i;", "a=75'b1 << (i/2); if (i%2) a=~a; if(i==150) a='0; if(i==151) a='1;")
    bench = bench.replace("PASS 8192", "PASS 2432")
    return source, bench



class FunctionReplayTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("iverilog") and shutil.which("vvp"),
                         "Icarus Verilog is required for RTL comparison")
    def test_packed_input_copy_and_nested_returns(self):
        self.check_source(SYSTEMVERILOG, TESTBENCH)

    @unittest.skipUnless(shutil.which("iverilog") and shutil.which("vvp"),
                         "Icarus Verilog is required for RTL comparison")
    def test_wide_nested_members_and_early_case_returns(self):
        self.check_source(*wide_case_fixture())

    def check_source(self, source_text, testbench):
        with tempfile.TemporaryDirectory(prefix="naja-function-replay-") as tmp:
            directory = Path(tmp)
            source = directory / "source.sv"
            source.write_text(source_text)
            universe = naja.NLUniverse.create()
            db = naja.NLDB.create(universe)
            universe.setTopDB(db)
            try:
                top = db.loadSystemVerilog([str(source)], diagnostics_report_path=None)
                top.dumpGateLevelVerilog(str(directory), "design.v")
                self.compare_rtl(directory, source_text, testbench)
                snapshot = directory / "snapshot"
                db.dumpNajaIF(str(snapshot))
            finally:
                universe.destroy()
            # A different process reconstructs all canonical primitives without
            # any warm primitive caches from elaboration.
            subprocess.run([sys.executable, "-c", """
from najaeda import naja
import sys
u = naja.NLUniverse.create()
db = naja.NLDB.loadNajaIF(sys.argv[1])
db.getTopDesign().dumpGateLevelVerilog(sys.argv[2], 'design.v')
u.destroy()
""", str(snapshot), str(directory)], check=True, env=os.environ.copy())
            self.compare_rtl(directory, source_text, testbench)

    def compare_rtl(self, directory, source_text, testbench):
        # Use the same unmodified function bodies in the simulator reference.
        reference = source_text.replace("package values;", "package ref_values;")
        reference = reference.replace("import values::*;", "import ref_values::*;")
        reference = reference.replace("module top(", "module reference_rtl(")
        tb = directory / "tb.sv"
        tb.write_text(reference + testbench)
        executable = directory / "simulation"
        sources = [str(tb), str(directory / "design.v")]
        primitives = directory / "naja_primitives.v"
        if primitives.exists():
            sources.append(str(primitives))
        subprocess.run(["iverilog", "-g2012", "-s", "tb", "-o", str(executable),
                        *sources], check=True)
        result = subprocess.run(["vvp", str(executable)], text=True, capture_output=True)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("PASS ", result.stdout)


if __name__ == "__main__":
    unittest.main()
