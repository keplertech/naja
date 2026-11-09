# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Check native RTL and both frontend dumps, including overwritten X/Z decisions."""
import argparse
import os
from pathlib import Path
import tempfile

from compare_pipeline import run


def main():
    parser = argparse.ArgumentParser()
    for option in ("adapter", "nvc", "iverilog", "vvp"):
        parser.add_argument("--" + option, required=True)
    args = parser.parse_args()
    fixtures = Path(__file__).resolve().parent
    # Include X/Z in the killed decisions and in the surviving data. The live
    # enable is binary: this test does not introduce an enable-X policy.
    patterns = [(en, sel, data) for en in (1, 0, 1) for sel in ("00", "01", "10", "11", "xx", "zz")
                for data in ("0000", "1010", "10xz", "z01x")]
    with tempfile.TemporaryDirectory(prefix="naja-scheduled-overwrite-") as directory:
        work = Path(directory)
        run([str(Path(args.adapter).resolve()),
             "--gtest_filter=VHDLConstructorTest.ScheduledOverwritePreservesStorageAndOrigins"],
            work, dict(os.environ, SCHEDULED_OVERWRITE_DUMP=str(work)))
        steps = "\n".join(f"step(1'b{en}, 2'b{sel}, 4'b{data});" for en, sel, data in patterns)
        tb = """
module tb;
  reg clk=0, en;
  reg [1:0] s;
  reg [3:0] a=4'b0101, b;
  wire [3:0] q,p;
  wire held;
  reg [3:0] expected_q=4'bxxxx, expected_p;
  reg expected_held=1'bx;
  scheduled_overwrite dut(.*);
  task step(input new_en, input [1:0] new_s, input [3:0] new_b);
    begin
      clk=0; en=new_en; s=new_s; b=new_b; #5;
      expected_p=expected_q; expected_q=b;
      if(en) expected_held=b[0];
      clk=1; #5;
      if(q !== expected_q || p !== expected_p || held !== expected_held)
        $fatal(1,"scheduled write or retained enable mismatch");
    end
  endtask
  initial begin
STEPS
    $display("PASS: scheduled overwrite and old-value reads"); $finish;
  end
endmodule
""".replace("STEPS", steps)
        (work / "tb.sv").write_text(tb)
        # Native SV and each generated network use exactly the same oracle.
        for kind in ("native", "sv", "vhdl"):
            sources = [str(fixtures / "scheduled_overwrite.sv")] if kind == "native" else [
                str(work / kind / "scheduled_overwrite.v"), str(work / kind / "naja_primitives.v")]
            run([args.iverilog, "-g2012", "-s", "tb", "-o", "sim", *sources, "tb.sv"], work)
            run([args.vvp, "sim"], work)
        steps = "\n".join(f"step('{en}', \"{sel.upper()}\", \"{data.upper()}\");"
                          for en, sel, data in patterns)
        (work / "tb.vhd").write_text("""
library ieee; use ieee.std_logic_1164.all;
entity tb is end;
architecture test of tb is
  signal clk : std_logic := '0';
  signal en : std_logic;
  signal s : std_logic_vector(1 downto 0);
  signal a : std_logic_vector(3 downto 0) := "0101";
  signal b,q,p : std_logic_vector(3 downto 0);
  signal held : std_logic;
begin
  dut: entity work.scheduled_overwrite port map(clk,en,s,a,b,q,p,held);
  process
    variable expected_q,expected_p : std_logic_vector(3 downto 0) := (others=>'U');
    variable expected_held : std_logic := 'U';
    procedure step(new_en : std_logic; new_s : std_logic_vector; new_b : std_logic_vector) is
    begin
      clk<='0'; en<=new_en; s<=new_s; b<=new_b; wait for 5 ns;
      expected_p:=expected_q; expected_q:=new_b;
      if new_en='1' then expected_held:=new_b(new_b'right); end if;
      clk<='1'; wait for 5 ns;
      assert q=expected_q and p=expected_p and held=expected_held
        report "scheduled write or retained enable mismatch" severity failure;
    end;
  begin
STEPS
    report "PASS: native VHDL scheduled overwrite and old-value reads";
    wait;
  end process;
end;
""".replace("STEPS", steps))
        run([args.nvc, "--std=2008", "-a", str(fixtures / "scheduled_overwrite.vhd"), "tb.vhd"], work)
        run([args.nvc, "--std=2008", "-e", "tb"], work)
        run([args.nvc, "--std=2008", "-r", "tb", f"--stop-time={10 * len(patterns)}ns"], work)


if __name__ == "__main__":
    main()
