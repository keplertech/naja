#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Focused VHDL language regressions; run with the built naja on PYTHONPATH."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import naja

FIXTURES = Path(__file__).with_name('soc_probes')


class SocFrontendTest(unittest.TestCase):
    def setUp(self):
        naja.NLUniverse.create()
        self.db = naja.NLDB.create(naja.NLUniverse.get())
        self.work = tempfile.TemporaryDirectory(prefix='naja-soc-test-')
        self.directory = Path(self.work.name)

    def tearDown(self):
        naja.NLUniverse.get().destroy()
        self.work.cleanup()

    def load(self, name=None, source=None):
        path = FIXTURES / name if source is None else self.directory / 'input.vhdl'
        if source is not None:
            path.write_text(source)
        return self.db.loadVHDL([str(path)], top='probe',
                               diagnostics_report_path=str(self.directory / 'diagnostics.log'))

    def simulate(self, design, bench):
        if not shutil.which('iverilog') or not shutil.which('vvp'):
            self.skipTest('iverilog and vvp required for cycle checks')
        design.dumpGateLevelVerilog(str(self.directory), 'probe.v')
        (self.directory / 'tb.v').write_text(bench)
        files = [str(f) for f in self.directory.glob('*.v')]
        subprocess.run(['iverilog', '-g2012', '-s', 'tb', '-o', 'sim', *files],
                       cwd=self.directory, check=True, capture_output=True, text=True)
        subprocess.run(['vvp', 'sim'], cwd=self.directory, check=True,
                       capture_output=True, text=True)

    def test_initializer_diagnostic_names_signal_and_declaration(self):
        with self.assertRaisesRegex(RuntimeError, r'startup_flag.*14_initializer.vhdl.*line 5, column 3'):
            self.load('14_initializer.vhdl')

    def test_conditional_assignment_holds_when_false(self):
        design = self.load('01_conditional.vhdl')
        self.simulate(design, '''module tb;
reg clk=0, en=1, d=1; wire q;
probe dut(.clk(clk),.en(en),.d(d),.q(q));
initial begin
#1; clk=1; #1; if(q !== 1) $fatal;
clk=0; en=0; d=0; #1; clk=1; #1; if(q !== 1) $fatal;
clk=0; en=1; #1; clk=1; #1; if(q !== 0) $fatal;
$finish; end endmodule''')

    def test_conditional_assignment_keeps_earlier_scheduled_value(self):
        design = self.load(source='''entity probe is port(a, b : in bit; q : out bit_vector(1 downto 0)); end;
architecture rtl of probe is begin process(all)
variable v : bit_vector(1 downto 0);
begin v := "00"; v(0) := '1' when a='1' else '0' when b='1';
q <= "11"; q(1) <= v(0) when a='1' else '0' when b='1';
end process; end;''')
        self.simulate(design, '''module tb;
reg a=0,b=0; wire [1:0] q; probe dut(.a(a),.b(b),.q(q));
initial begin #1; if(q !== 3) $fatal;
b=1; #1; if(q !== 1) $fatal;
a=1; #1; if(q !== 3) $fatal;
$finish; end endmodule''')

    def check_unary(self, design, inverted=False):
        expected = '!a' if inverted else 'a'
        self.simulate(design, "module tb; reg a=0; wire q; probe dut(.a(a),.q(q)); "
                      "initial begin #1; if(q !== " + expected + ") $fatal; "
                      "a=1; #1; if(q !== " + expected + ") $fatal; $finish; end endmodule")

    def test_process_enumeration(self):
        self.check_unary(self.load('03_process_type.vhdl'))

    def test_process_subtype(self):
        self.check_unary(self.load('03_process_subtype.vhdl'))

    def test_process_function(self):
        self.check_unary(self.load('03_process_function.vhdl'), inverted=True)

    def test_process_declarations_restore_outer_scope(self):
        design = self.load(source='''entity probe is port(a : in bit; q : out bit); end;
architecture rtl of probe is
subtype flag is bit; type phases is (idle, active);
function inv(v : bit) return bit is begin return not v; end;
signal x : bit;
begin process(all)
subtype flag is bit_vector(1 downto 0);
type phases is (idle, active);
function inv(v : bit) return bit is begin return v; end;
variable v : flag;
begin v := (others => inv(a)); x <= v(0); end process;
process(all) variable v : flag; begin v := inv(x); q <= v; end process;
end;''')
        self.check_unary(design, inverted=True)

    def test_natural_range_port_width_and_comparison(self):
        design = self.load('05_natural_port.vhdl')
        self.assertEqual(design.getBusTerm('a').getWidth(), 8)
        self.simulate(design, '''module tb;
reg [7:0] a=0; wire q; integer i; probe dut(.a(a),.q(q));
initial begin for(i=0;i<256;i=i+1) begin a=i; #1;
if(q !== (i==255)) $fatal; end $finish; end endmodule''')

    def test_falling_edge_updates_only_on_negative_edge(self):
        design = self.load('06_falling_edge.vhdl')
        self.simulate(design, '''module tb;
reg clk=1,d=0; wire q; probe dut(.clk(clk),.d(d),.q(q));
initial begin #1; clk=0; #1; if(q !== 0) $fatal;
d=1; clk=1; #1; if(q !== 0) $fatal;
clk=0; #1; if(q !== 1) $fatal;
$finish; end endmodule''')

    def test_alias_slice_read(self):
        design = self.load('04_alias.vhdl')
        self.simulate(design, '''module tb;
reg [3:0] a=0; wire [1:0] q; integer i; probe dut(.a(a),.q(q));
initial begin for(i=0;i<16;i=i+1) begin a=i; #1;
if(q !== (i&3)) $fatal; end $finish; end endmodule''')

    def test_alias_writes_share_scheduled_parent_value(self):
        design = self.load(source='''entity probe is port(a : in bit_vector(3 downto 0); q : out bit_vector(3 downto 0)); end;
architecture rtl of probe is
signal data : bit_vector(3 downto 0);
alias low : bit_vector(7 to 8) is data(1 downto 0);
alias first is low(7);
begin process(all) begin data <= a; low(8) <= not a(0); first <= a(1); end process;
q <= data; end;''')
        self.simulate(design, '''module tb;
reg [3:0] a=0; wire [3:0] q; integer i; probe dut(.a(a),.q(q));
initial begin for(i=0;i<16;i=i+1) begin a=i; #1;
if(q !== (i^1)) $fatal; end $finish; end endmodule''')

    def test_slice_choice_aggregate(self):
        design = self.load('13_slice_aggregate.vhdl')
        self.simulate(design, '''module tb;
reg [3:0] a=0; wire [7:0] q; integer i; probe dut(.a(a),.q(q));
initial begin for(i=0;i<16;i=i+1) begin a=i; #1;
if(q !== i) $fatal; end $finish; end endmodule''')

    def test_attribute_range_choice_aggregate(self):
        design = self.load(source='''entity probe is port(a : in bit_vector(2 to 5); q : out bit_vector(0 to 7)); end;
architecture rtl of probe is begin q <= (a'range => a, others => '0'); end;''')
        self.simulate(design, '''module tb;
reg [2:5] a=0; wire [0:7] q; integer i; probe dut(.a(a),.q(q));
initial begin for(i=0;i<16;i=i+1) begin a=i; #1;
if(q !== (i<<2)) $fatal; end $finish; end endmodule''')

    def test_disabled_generate_preserves_undriven_initializer(self):
        design = self.load('10_disabled_writer.vhdl')
        self.simulate(design, '''module tb;
reg a=0; wire [2:0] q; probe dut(.a(a),.q(q));
initial begin #1; if(q !== 5) $fatal;
a=1; #1; if(q !== 5) $fatal; $finish; end endmodule''')

    def test_string_generic_default(self):
        design = self.load('07_string_generic.vhdl')
        self.simulate(design, "module tb; wire q; probe dut(.q(q)); initial begin #1; if(q !== 0) $fatal; $finish; end endmodule")

    def test_string_generic_forwarding_and_specialization(self):
        design = self.load(source='''entity leaf is generic(filename : string); port(q : out bit); end;
architecture rtl of leaf is begin
q <= '1' when filename = "ok.hex" and filename'length = 6 else '0'; end;
entity middle is generic(stem : string); port(q : out bit); end;
architecture rtl of middle is begin
u : entity work.leaf generic map(filename => stem & ".hex") port map(q => q); end;
entity probe is port(q : out bit_vector(1 downto 0)); end;
architecture rtl of probe is begin
u : entity work.middle generic map("ok") port map(q(0));
v : entity work.middle generic map("other") port map(q(1)); end;''')
        self.simulate(design, "module tb; wire [1:0] q; probe dut(.q(q)); initial begin #1; if(q !== 1) $fatal; $finish; end endmodule")

    def test_simulation_package_visibility_has_no_hardware_effect(self):
        self.check_unary(self.load('08_textio_visibility.vhdl'))

    def test_parent_initializer_does_not_initialize_child_output(self):
        design = self.load('14_child_driven_initializer.vhdl')
        self.simulate(design, '''module tb;
reg clk=0,d=1; wire q; probe dut(.clk(clk),.d(d),.q(q));
initial begin #1; if(q !== 1'bx) $fatal;
clk=1; #1; if(q !== 1) $fatal;
$finish; end endmodule''')

    def test_file_initializer_is_not_silently_dropped(self):
        with self.assertRaisesRegex(RuntimeError, 'impure functions require a static initializer'):
            self.load('02_file_initializer.vhdl')

    def test_output_default_is_not_silently_dropped(self):
        with self.assertRaisesRegex(RuntimeError, 'port defaults are supported only for input ports'):
            self.load('09_output_default.vhdl')

    def test_impure_function_is_not_silently_dropped(self):
        with self.assertRaisesRegex(RuntimeError, 'impure functions require a static initializer'):
            self.load('12_impure.vhdl')

    def test_unused_undriven_output_remains_an_explicit_gap(self):
        with self.assertRaisesRegex(RuntimeError, 'signal bit has no driver: unused'):
            self.load('15_unused_output.vhdl')

    def test_string_generic_redeclaration_is_rejected(self):
        with self.assertRaises(RuntimeError):
            self.load(source='''entity probe is generic(filename : string := "a"); port(q : out bit); end;
architecture rtl of probe is signal filename : bit;
begin filename <= '0'; q <= filename; end;''')


if __name__ == '__main__':
    unittest.main()
