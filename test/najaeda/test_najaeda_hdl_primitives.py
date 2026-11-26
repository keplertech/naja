# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""External primitive binding preserves models and language-specific interfaces."""

from pathlib import Path
import os
import subprocess
import tempfile
import unittest

from najaeda import naja, netlist


LIBERTY = '''library(CELL_LIB) {
  type(word) { base_type : array; data_type : bit; bit_width : 3;
               bit_from : 4; bit_to : 2; downto : true; }
  cell(CELL_A) {
    pin(A) { direction : input; }
    pin(Y) { direction : output; function : "!A";
      timing() { related_pin : "A"; timing_type : combinational; }
    }
  }
  cell(CELL_B) {
    bus(A) { bus_type : word; direction : input; }
    bus(Y) { bus_type : word; direction : output; }
  }
  cell(CELL_F) {
    ff(IQ, IQN) { clocked_on : "C"; next_state : "D"; }
    pin(C) { direction : input; clock : true; }
    pin(D) { direction : input; }
    pin(Q) { direction : output; function : "IQ"; }
  }
}'''
SV = 'module top(input a, output y); CELL_A u(.A(a), .Y(y)); endmodule'
VHDL_HEADER = '''library ieee; use ieee.std_logic_1164.all;
entity top is port(a: in std_logic; y: out std_logic); end;
'''
VHDL_COMPONENT = VHDL_HEADER + '''architecture rtl of top is
  component cell_a port(a: in std_logic; y: out std_logic); end component;
begin u: CeLl_A port map(a, y); end;'''
VHDL_DIRECT = VHDL_HEADER.replace('entity top', 'library cell_lib;\nentity top') + '''
architecture rtl of top is begin
  u: entity CELL_LIB.cell_a port map(a => a, y => y);
end;'''


class HDLPrimitiveTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.reset()

    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()
        self.tmp.cleanup()

    def reset(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()
        path = self.root / 'cells.lib'
        path.write_text(LIBERTY)
        netlist.load_liberty(str(path))
        self.db = naja.NLUniverse.get().getTopDB()
        self.cells = self.db.getLibrary('CELL_LIB')

    def load(self, source, language='sv', **kwargs):
        path = self.root / ('top.' + language)
        path.write_text(source)
        if language == 'sv':
            config = netlist.SystemVerilogConfig(diagnostics_report_path=None)
            netlist.load_system_verilog(str(path), config, **kwargs)
        else:
            netlist.load_vhdl(str(path), diagnostics_report_path=None, **kwargs)
        return naja.NLUniverse.get().getTopDesign()

    def assert_cell(self, top):
        model = self.cells.getSNLDesign('CELL_A')
        instance = top.getInstance('u')
        self.assertEqual(instance.getModel(), model)
        self.assertTrue(model.isPrimitive())
        self.assertFalse(model.isBlackBox())
        self.assertEqual(model.getTruthTable(), [1, 1])
        self.assertEqual(list(naja.SNLDesign.getCombinatorialInputs(model.getTerm('Y'))),
                         [model.getTerm('A')])
        for pin, signal in [('A', 'a'), ('Y', 'y')]:
            self.assertEqual(instance.getInstTerm(model.getTerm(pin)).getNet(), top.getNet(signal))
        self.assertEqual(list(top.getLibrary().getSNLDesigns()), [top])

    def test_systemverilog_and_vhdl_identity(self):
        for language, source in [('sv', SV), ('vhd', VHDL_COMPONENT), ('vhd', VHDL_DIRECT)]:
            with self.subTest(language=language, source=source):
                self.reset()
                self.assert_cell(self.load(source, language))

    def test_explicit_sv_stub_and_positional_order(self):
        self.assert_cell(self.load(
            'module CELL_A(output Y, input A); endmodule\n' +
            SV.replace('.A(a), .Y(y)', 'y, a')))

    def test_vhdl_explicit_entity_interface(self):
        self.assert_cell(self.load(VHDL_COMPONENT.replace('architecture rtl',
            'library ieee; use ieee.std_logic_1164.all;\n'
            'entity cell_a is port(a: in std_logic; y: out std_logic); end;\narchitecture rtl'),
            'vhd'))

    def test_bus_declaration_order(self):
        sv = '''module top(input [0:2] a, output [0:2] y);
          CELL_B u(.A(a), .Y(y)); endmodule'''
        vhdl = '''entity top is port(a: in bit_vector(0 to 2); y: out bit_vector(0 to 2)); end;
          architecture rtl of top is
          component cell_b port(a: in bit_vector(8 to 10); y: out bit_vector(8 to 10)); end component;
          begin u: cell_b port map(y => y, a => a); end;'''
        for language, source in [('sv', sv), ('vhd', vhdl)]:
            self.reset()
            top = self.load(source, language)
            instance = top.getInstance('u')
            model = self.cells.getSNLDesign('CELL_B')
            self.assertEqual(instance.getModel(), model)
            for pin, signal in [('A', 'a'), ('Y', 'y')]:
                for term, net in zip(model.getTerm(pin).getBits(), top.getNet(signal).getBits()):
                    self.assertEqual(instance.getInstTerm(term).getNet(), net)

    def test_sequential_model_and_snapshot(self):
        for language, source in [
                ('sv', 'module top(input c,d, output q); CELL_F u(.C(c),.D(d),.Q(q)); endmodule'),
                ('vhd', '''entity top is port(c,d: in bit; q: out bit); end;
                  architecture rtl of top is component cell_f port(c,d: in bit; q: out bit); end component;
                  begin u: cell_f port map(c,d,q); end;''')]:
            self.reset()
            top = self.load(source, language)
            model = top.getInstance('u').getModel()
            self.assertEqual(model, self.cells.getSNLDesign('CELL_F'))
            self.assertIsNotNone(model.getSequentialModel())
            snapshot = self.root / ('snapshot-' + language)
            self.db.dumpNajaIF(str(snapshot))
            naja.NLUniverse.get().destroy()
            netlist.load_liberty(str(self.root / 'cells.lib'))
            self.cells = naja.NLUniverse.get().getTopDB().getLibrary('CELL_LIB')
            db = naja.NLDB.loadNajaIF(str(snapshot))
            self.assertEqual(db.getTopDesign().getInstance('u').getModel(),
                             db.getLibrary('CELL_LIB').getSNLDesign('CELL_F'))
            self.assertIsNotNone(db.getTopDesign().getInstance('u').getModel().getSequentialModel())

    def test_generated_and_array_instances(self):
        top = self.load("""module top(input [1:0] a, output [1:0] y, z);
          for (genvar k=0; k<2; k++) begin: g
            CELL_A u(.A(a[k]), .Y(y[k]));
          end
          CELL_A v[1:0](.A(a), .Y(z));
        endmodule""")
        instances = list(top.getInstances())
        self.assertEqual(len(instances), 4)
        model = self.cells.getSNLDesign('CELL_A')
        self.assertTrue(all(i.getModel() == model for i in instances))
        for instance in instances:
            self.assertTrue(all(t.getNet() is not None for t in instance.getInstTerms()))

    def test_escaped_external_identifiers(self):
        for language in ('sv', 'vhd'):
            self.reset()
            library = naja.NLLibrary.createPrimitives(self.db, 'EXTRA')
            model = naja.SNLDesign.createPrimitive(library, 'gate.x')
            a = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, 'I+')
            y = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Output, 'O!')
            source = (r'module top(input a, output y); \gate.x u(.\I+ (a), .\O! (y)); endmodule'
                      if language == 'sv' else
                      VHDL_HEADER.replace('entity top', 'library extra; entity top') +
                      r'architecture rtl of top is begin u: entity extra.\gate.x\ port map(\I+\ => a, \O!\ => y); end;')
            top = self.load(source, language)
            instance = top.getInstance('u')
            self.assertEqual(instance.getModel(), model)
            self.assertEqual(instance.getInstTerm(a).getNet(), top.getNet('a'))
            self.assertEqual(instance.getInstTerm(y).getNet(), top.getNet('y'))

    def test_vhdl_case_collisions(self):
        naja.SNLDesign.createPrimitive(self.cells, 'cell_a')
        with self.assertRaisesRegex(RuntimeError, 'ambiguous primitive'):
            self.load(VHDL_COMPONENT, 'vhd')
        self.reset()
        model = self.cells.getSNLDesign('CELL_A')
        naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, 'a')
        with self.assertRaisesRegex(RuntimeError, 'ambiguous case-insensitive getTerm'):
            self.load(VHDL_COMPONENT, 'vhd')

    def test_snapshot_rejects_mismatched_source_interfaces(self):
        self.load(SV)
        snapshot = self.root / 'mismatched-snapshot'
        self.db.dumpNajaIF(str(snapshot))
        self.reset()
        self.cells.getSNLDesign('CELL_A').getTerm('A').setName('DIFFERENT')
        with self.assertRaisesRegex(RuntimeError, 'snapshot library interface differs'):
            netlist.load_naja_if(str(snapshot))

    def test_case_insensitive_lookups_and_lifecycle(self):
        self.assertIsNone(self.db.getLibrary('cell_lib'))
        self.assertEqual(self.db.getLibraryCaseInsensitive('cell_lib'), self.cells)
        model = self.cells.getSNLDesignCaseInsensitive('cell_a')
        self.assertIsNone(self.cells.getSNLDesign('cell_a'))
        self.assertEqual(model.getTermCaseInsensitive('a'), model.getTerm('A'))
        self.assertIsNone(model.getTermCaseInsensitive('absent'))
        duplicate = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, 'a')
        with self.assertRaisesRegex(RuntimeError, 'ambiguous'):
            model.getTermCaseInsensitive('A')
        duplicate.destroy()
        self.assertEqual(model.getTermCaseInsensitive('a'), model.getTerm('A'))
        other = naja.SNLDesign.createPrimitive(self.cells, 'cell_a')
        with self.assertRaisesRegex(RuntimeError, 'ambiguous'):
            self.cells.getSNLDesignCaseInsensitive('CELL_A')
        other.setName('cell_other')
        self.assertEqual(self.cells.getSNLDesignCaseInsensitive('cell_a'), model)
        otherlib = naja.NLLibrary.create(self.db, 'cell_lib')
        with self.assertRaisesRegex(RuntimeError, 'ambiguous'):
            self.db.getLibraryCaseInsensitive('CELL_LIB')
        otherlib.setName('RENAMED')
        for owner, method in [(self.db, 'getLibraryCaseInsensitive'),
                              (self.cells, 'getSNLDesignCaseInsensitive'),
                              (model, 'getTermCaseInsensitive')]:
            with self.assertRaises(TypeError):
                getattr(owner, method)(17)

    def test_errors(self):
        cases = [
            ('sv', SV.replace('CELL_A u', 'cell_a u'), 'unknown module'),
            ('sv', SV.replace('.A(a), .Y(y)', 'a, y'), 'positional primitive'),
            ('sv', 'module CELL_A(input A, output Y); assign Y=A; endmodule\n' + SV, 'conflicts'),
            ('sv', 'module CELL_A(input [1:0] A, output Y); endmodule\n' + SV, 'mismatch'),
            ('vhd', VHDL_COMPONENT.replace('y: out std_logic); end component',
                                           'y: in std_logic); end component'), 'mismatch'),
            ('vhd', VHDL_DIRECT.replace('a => a, y => y', 'a, y'), 'positional primitive'),
            ('vhd', VHDL_DIRECT.replace('CELL_LIB.cell_a', 'work.cell_a'), 'missing entity'),
        ]
        for language, source, message in cases:
            with self.subTest(language=language, message=message):
                self.reset()
                with self.assertRaisesRegex(RuntimeError, message):
                    self.load(source, language)

    def test_ambiguity_and_database_isolation(self):
        for language, source in [('sv', SV), ('vhd', VHDL_COMPONENT)]:
            self.reset()
            duplicate = naja.NLLibrary.createPrimitives(self.db, 'OTHER')
            naja.SNLDesign.createPrimitive(duplicate, 'CELL_A')
            with self.assertRaisesRegex(RuntimeError, 'ambiguous primitive'):
                self.load(source, language)
        # Explicit VHDL library qualification does not search the other library.
        self.assert_cell(self.load(VHDL_DIRECT, 'vhd'))
        db = naja.NLDB.create(naja.NLUniverse.get())
        path = self.root / 'isolated.sv'
        path.write_text(SV)
        with self.assertRaisesRegex(RuntimeError, 'unknown module'):
            db.loadSystemVerilog([str(path)], diagnostics_report_path=None)

    @unittest.skipUnless(os.environ.get('NAJA_EDIT_BINARY'), 'naja_edit binary not supplied')
    def test_cli_both_frontends(self):
        for language, source in [('systemverilog', SV), ('vhdl', VHDL_COMPONENT)]:
            with self.subTest(language=language):
                path = self.root / ('cli.sv' if language == 'systemverilog' else 'cli.vhd')
                path.write_text(source)
                output = self.root / (language + '.v')
                top_option = '--sv_top' if language == 'systemverilog' else '--vhdl_top'
                result = subprocess.run([
                    os.environ['NAJA_EDIT_BINARY'], '-f', language,
                    '-p', str(self.root / 'cells.lib'), '-i', str(path), top_option, 'top',
                    '-t', 'gate-level-verilog', '-o', str(output)],
                    cwd=self.root, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.reset()
                netlist.load_gate_level_verilog(str(output))
                self.assert_cell(naja.NLUniverse.get().getTopDesign())
                snapshot = self.root / (language + '-snapshot')
                self.db.dumpNajaIF(str(snapshot))
                result = subprocess.run([
                    os.environ['NAJA_EDIT_BINARY'], '-f', 'snl', '-p', str(self.root / 'cells.lib'),
                    '-i', str(snapshot), '-t', 'gate-level-verilog', '-o', str(output)],
                    cwd=self.root, capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_extended_vhdl_pin_and_cell_names(self):
        self.assert_cell(self.load(VHDL_COMPONENT.replace('cell_a', '\\CELL_A\\')
                                  .replace('CeLl_A', '\\CELL_A\\')
                                  .replace('port map(a, y)', 'port map(\\A\\ => a, \\Y\\ => y)')
                                  .replace('port(a: in std_logic; y: out std_logic); end component',
                                           'port(\\A\\: in std_logic; \\Y\\: out std_logic); end component'), 'vhd'))
        self.reset()
        with self.assertRaisesRegex(RuntimeError, 'missing entity'):
            self.load(VHDL_COMPONENT.replace('CeLl_A', '\\cell_a\\')
                      .replace('component cell_a', 'component \\cell_a\\'), 'vhd')


if __name__ == '__main__':
    unittest.main()
