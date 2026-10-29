#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Initialized RAM contents, synthesis behavior, and file diagnostics."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import sys
import unittest
import naja

FIXTURE = Path(__file__).with_name('memory_file_init.vhdl')


class MemoryInitializationTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='naja-memory-init-')
        self.work = Path(self.temp.name)
        self.previous = Path.cwd()
        os.chdir(self.work)
        naja.NLUniverse.create()
        self.db = naja.NLDB.create(naja.NLUniverse.get())

    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()
        os.chdir(self.previous)
        self.temp.cleanup()

    def load(self, data='123\nA_bC\n00f\n', source=None):
        if data is not None:
            (self.work / 'image.hex').write_text(data)
        path = self.work / 'input.vhdl'
        path.write_text(source or FIXTURE.read_text())
        return self.db.loadVHDL([str(path)], top='probe', diagnostics_report_path=str(self.work / 'diagnostics.log'))

    def memory(self, top):
        memories = [i for i in top.getInstances() if i.getModel().getParameter('INIT_ENABLE') is not None]
        self.assertEqual(len(memories), 1)
        return memories[0]

    def initial_bits(self, memory):
        literal = memory.getInstParameter('INIT').getValue()
        width, digits = literal.split("'")
        if digits[0] == 'b':
            return digits[1:]
        return ''.join(c * 4 if c in 'xz' else format(int(c, 16), '04b') for c in digits[1:])[-int(width):]

    def simulate(self, top, bench):
        if not shutil.which('iverilog') or not shutil.which('vvp'):
            self.skipTest('Icarus Verilog required for cycle checks')
        top.dumpGateLevelVerilog(str(self.work), 'probe.v')
        (self.work / 'tb.v').write_text(bench)
        subprocess.run(['iverilog', '-g2012', '-s', 'tb', '-o', 'sim', *[str(f) for f in self.work.glob('*.v')]],
                       cwd=self.work, check=True, capture_output=True, text=True)
        subprocess.run(['vvp', 'sim'], cwd=self.work, check=True, capture_output=True, text=True)

    def test_file_contents_zero_fill_and_write_after_initialization(self):
        top = self.load()
        memory = self.memory(top)
        self.assertEqual(memory.getInstParameter('INIT_ENABLE').getValue(), '1')
        self.assertEqual(int(memory.getInstParameter('DEPTH').getValue()), 7)
        literal = memory.getInstParameter('INIT').getValue()
        self.assertEqual(int(self.initial_bits(memory), 2), 0x00fabc123)
        self.assertLess(len(list(top.getNets())), 100)
        self.simulate(top, '''module tb;
reg clk=0,we=0; reg [2:0] addr=0; reg [11:0] d=0; wire [11:0] q;
probe dut(.clk(clk),.we(we),.addr(addr),.d(d),.q(q));
task tick; begin #1;clk=1;#1;clk=0;end endtask
initial begin
tick; if(q !== 12'h123) $fatal(1,"initial word zero");
addr=1;tick; if(q !== 12'habc) $fatal(1,"initial word one");
addr=6;tick; if(q !== 0) $fatal(1,"zero fill");
addr=0;we=1;d=12'h456;tick; if(q !== 12'h123) $fatal(1,"read before write");
we=0;tick; if(q !== 12'h456) $fatal(1,"write persisted");
$finish;end endmodule''')

    def test_missing_file_is_an_error(self):
        with self.assertRaisesRegex(RuntimeError, 'cannot open initialization file.*image.hex'):
            self.load(data=None)

    def test_malformed_data_is_an_error(self):
        with self.assertRaisesRegex(RuntimeError, 'invalid hexadecimal initialization data'):
            self.load(data='q23\n')

    def test_short_line_is_an_error(self):
        with self.assertRaisesRegex(RuntimeError, 'line is too short'):
            self.load(data='12\n')

    def test_empty_file_keeps_zero_fill(self):
        top = self.load(data='')
        self.assertEqual(int(self.initial_bits(self.memory(top)), 2), 0)

    def test_descending_nonzero_addresses(self):
        source = FIXTURE.read_text().replace('array(0 to 6)', 'array(6 downto 3)')
        top = self.load(data='123\n456\n789\nabc\n', source=source)
        literal = self.memory(top).getInstParameter('INIT').getValue()
        self.assertEqual(int(self.initial_bits(self.memory(top)), 2), 0x123456789abc)
        self.simulate(top, '''module tb;
reg clk=0,we=0;reg [2:0] addr=3;reg [11:0] d=0;wire [11:0] q;
probe dut(.clk(clk),.we(we),.addr(addr),.d(d),.q(q));
initial begin #1;clk=1;#1;if(q !== 12'habc) $fatal;
clk=0;addr=6;#1;clk=1;#1;if(q !== 12'h123) $fatal;$finish;end endmodule''')

    def test_nonbinary_memory_initialization_is_preserved(self):
        top = self.load(data='XZ0\n')
        literal = self.memory(top).getInstParameter('INIT').getValue()
        self.assertTrue(self.initial_bits(self.memory(top)).endswith('xxxxzzzz0000'))

    def test_write_mode_is_rejected(self):
        source = FIXTURE.read_text().replace('open read_mode', 'open write_mode')
        with self.assertRaisesRegex(RuntimeError, 'only read-only text files'):
            self.load(source=source)
        self.assertEqual((self.work / 'image.hex').read_text(), '123\nA_bC\n00f\n')

    def test_large_initialized_array_stays_a_memory(self):
        source = FIXTURE.read_text().replace('array(0 to 6)', 'array(0 to 65535)')
        top = self.load(source=source)
        self.assertEqual(self.memory(top).getInstParameter('DEPTH').getValue(), '65536')
        self.assertLess(len(list(top.getNets())), 100)
        self.assertLess(len(list(top.getInstances())), 100)

    def test_binary_read(self):
        source = FIXTURE.read_text().replace('hread(row, word)', 'read(row, word)')
        top = self.load(data='000100100011\n', source=source)
        self.assertTrue(self.initial_bits(self.memory(top)).endswith('000100100011'))

    def test_changed_file_invalidates_cached_design(self):
        top = self.load()
        (self.work / 'image.hex').write_text('456\n')
        with self.assertRaisesRegex(RuntimeError, 'initialization file changed for existing design'):
            self.db.loadVHDL([str(self.work / 'input.vhdl')], top='probe')
        self.assertEqual(self.initial_bits(self.memory(top))[-12:], '000100100011')

    def test_while_loop_and_eof(self):
        source = FIXTURE.read_text().replace('variable result : words_t', 'variable i : natural := 0;\n    variable result : words_t')
        source = source.replace("for i in result'range loop", "while not endfile(input_file) loop")
        source = source.replace('result(i) := word;', 'result(i) := word; i := i + 1;')
        top = self.load(source=source)
        self.assertEqual(int(self.initial_bits(self.memory(top)), 2), 0x00fabc123)

    def test_constant_initializer_also_infers_memory(self):
        source = FIXTURE.read_text().replace(':= load_image(filename);', ":= (others => x\"a5c\");")
        top = self.load(data=None, source=source)
        self.assertEqual(int(self.initial_bits(self.memory(top)), 2), int('a5c' * 7, 16))

    def test_hex_padding_and_overflow(self):
        source = FIXTURE.read_text().replace('11 downto 0', '4 downto 0')
        top = self.load(data='1f\n', source=source)
        self.assertEqual(int(self.initial_bits(self.memory(top)), 2), 31)
        naja.NLUniverse.get().destroy()
        naja.NLUniverse.create()
        self.db = naja.NLDB.create(naja.NLUniverse.get())
        with self.assertRaisesRegex(RuntimeError, 'exceeds the target width'):
            self.load(data='ff\n', source=source)

    def test_masked_write_preserves_other_initialized_lanes(self):
        source = FIXTURE.read_text().replace('d : in std_logic_vector', 'sel : in std_logic_vector(2 downto 0); d : in std_logic_vector', 1)
        source = source.replace('storage(to_integer(addr)) <= d;', '''
for lane in 0 to 2 loop
if sel(lane)='1' then
storage(to_integer(addr))((lane+1)*4-1 downto lane*4) <= d((lane+1)*4-1 downto lane*4);
end if; end loop;''')
        top = self.load(source=source)
        self.memory(top)
        self.simulate(top, '''module tb;
reg clk=0,we=0;reg [2:0] addr=0,sel=1;reg [11:0] d=12'habc;wire [11:0] q;
probe dut(.clk(clk),.we(we),.addr(addr),.d(d),.q(q),.sel(sel));
task tick; begin #1;clk=1;#1;clk=0;end endtask
initial begin tick;if(q !== 12'h123) $fatal;
we=1;tick;if(q !== 12'h123) $fatal;
we=0;tick;if(q !== 12'h12c) $fatal;
we=1;sel=4;tick;we=0;tick;if(q !== 12'ha2c) $fatal;
$finish;end endmodule''')

    def test_snapshot_reload_in_fresh_process_needs_no_init_file(self):
        top = self.load()
        expected = self.memory(top).getInstParameter('INIT').getValue()
        naja.NLUniverse.get().setTopDesign(top)
        self.db.dumpNajaIF(str(self.work / 'snapshot'))
        (self.work / 'image.hex').unlink()
        program = '''import naja, sys
naja.NLDB.loadNajaIF(sys.argv[1])
top = naja.NLUniverse.get().getTopDesign()
mem = [i for i in top.getInstances() if i.getModel().getParameter('INIT_ENABLE') is not None]
assert len(mem) == 1
assert mem[0].getInstParameter('INIT').getValue() == sys.argv[2]
top.dumpGateLevelVerilog(sys.argv[3], 'probe.v')
'''
        exported = self.work / 'reloaded'
        exported.mkdir()
        subprocess.run([sys.executable, '-c', program, str(self.work / 'snapshot'), expected, str(exported)],
                       check=True, capture_output=True, text=True)
        self.assertTrue((exported / 'probe.v').exists())

    def test_nested_function_and_cached_parent_track_file_changes(self):
        source = FIXTURE.read_text().replace('entity probe is', 'entity storage_unit is', 1)
        source = source.replace('architecture rtl of probe is', 'architecture rtl of storage_unit is', 1)
        source += '''
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity probe is port(clk,we : in std_logic;addr : in unsigned(2 downto 0);
d : in std_logic_vector(11 downto 0);q : out std_logic_vector(11 downto 0));end;
architecture rtl of probe is begin
u:entity work.storage_unit port map(clk,we,addr,d,q);end;'''
        self.load(source=source)
        (self.work / 'image.hex').write_text('456\n')
        with self.assertRaisesRegex(RuntimeError, 'initialization file changed for existing design'):
            self.db.loadVHDL([str(self.work / 'input.vhdl')], top='probe')

    def test_reference_initial_contents_with_nvc(self):
        if not shutil.which('nvc'):
            self.skipTest('NVC required for VHDL reference check')
        self.load()
        bench = '''library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb is end; architecture sim of tb is
signal clk,we : std_logic := '0';signal addr : unsigned(2 downto 0) := "000";
signal d,q : std_logic_vector(11 downto 0);
begin u:entity work.probe port map(clk,we,addr,d,q);
process begin
wait for 1 ns;clk<='1';wait for 1 ns;assert q=x"123" severity failure;
clk<='0';addr<="001";wait for 1 ns;clk<='1';wait for 1 ns;assert q=x"abc" severity failure;
clk<='0';addr<="110";wait for 1 ns;clk<='1';wait for 1 ns;assert q=x"000" severity failure;
wait;end process;end;'''
        (self.work / 'reference.vhdl').write_text(bench)
        subprocess.run(['nvc', '--std=2008', '-a', 'input.vhdl', 'reference.vhdl', '-e', 'tb', '-r', '--stop-time=8ns'],
                       cwd=self.work, check=True, capture_output=True, text=True)


    def test_while_predicate_observes_updated_integer_variable(self):
        source = FIXTURE.read_text().replace('variable result : words_t', 'variable i : natural := 0;\n    variable result : words_t')
        source = source.replace("for i in result'range loop", "while i < 3 loop")
        source = source.replace('result(i) := word;', 'result(i) := word; i := i + 1;')
        top = self.load(source=source)
        self.assertEqual(int(self.initial_bits(self.memory(top)), 2), 0x00fabc123)


if __name__ == '__main__':
    unittest.main()
