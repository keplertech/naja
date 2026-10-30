# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest

import naja


class SNLMemoryInterfaceTest(unittest.TestCase):
  def setUp(self):
    self.db = naja.NLDB.create(naja.NLUniverse.create())

  def tearDown(self):
    naja.NLUniverse.get().destroy()

  def loadSV(self, source):
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, 'memory.sv')
      with open(path, 'w') as stream:
        stream.write(source)
      top = self.db.loadSystemVerilog([path], diagnostics_report_path=None)
    memories = [i for i in top.getInstances() if i.getModel().hasMemoryInterface()]
    self.assertEqual(1, len(memories))
    self.assertFalse(top.hasMemoryInterface())
    return memories[0]

  def writable(self):
    return self.loadSV('''module sample(input clk, we,
      input [1:0] a, b, input [2:0] d, output [2:0] q, r);
      reg [2:0] storage [0:3];
      always @(posedge clk) if (we) storage[a] <= d;
      assign q = storage[a];
      assign r = storage[b];
    endmodule''')

  def assertBits(self, terms, model, name, indices):
    self.assertIsInstance(terms, list)
    self.assertEqual([model.getBusTerm(name).getBusTermBit(i) for i in indices], terms)
    for term in terms:
      self.assertIsInstance(term, naja.SNLBitTerm)
      self.assertEqual(model, term.getDesign())

  def testWritableAndMultipleReadPorts(self):
    instance = self.writable()
    model = instance.getModel()
    declared = model.getMemoryInterface()
    effective = instance.getMemoryInterface()
    self.assertEqual(declared, effective)
    self.assertEqual({'width', 'depth', 'abits', 'resetMode', 'clock', 'reset',
                      'readPorts', 'writePorts'}, set(declared))
    self.assertEqual((3, 4, 2, 'none'), tuple(declared[k] for k in ('width', 'depth', 'abits', 'resetMode')))
    self.assertEqual(model.getScalarTerm('CLK'), declared['clock'])
    self.assertEqual(2, len(declared['readPorts']))
    self.assertEqual(1, len(declared['writePorts']))
    for p, port in enumerate(declared['readPorts']):
      self.assertBits(port['address'], model, 'RADDR', range(p*2, p*2+2))
      self.assertBits(port['data'], model, 'RDATA', range(p*3, p*3+3))
      self.assertEqual([], port['enables'])
    port = declared['writePorts'][0]
    self.assertBits(port['address'], model, 'WADDR', range(2))
    self.assertBits(port['data'], model, 'WDATA', range(3))
    self.assertBits(port['enables'], model, 'WE', [0])
    self.assertEqual([], port['mask'])
    self.assertEqual([], port['extraWriteInputs'])
    self.assertIsNone(port['clock'])
    # Returning mutable containers must not mutate C++ metadata.
    declared['readPorts'][0]['address'].clear()
    self.assertEqual(effective, model.getMemoryInterface())

  def testReadOnly(self):
    instance = self.loadSV('''module sample(input [1:0] a, output reg [2:0] q);
      localparam logic [2:0] storage [0:3] = '{3'd3, 3'd6, 3'd1, 3'd4};
      assign q = storage[a];
    endmodule''')
    memory = instance.getMemoryInterface()
    self.assertEqual((3, 4, 2), tuple(memory[k] for k in ('width', 'depth', 'abits')))
    self.assertEqual([], memory['writePorts'])
    self.assertIsNone(memory['clock'])
    self.assertIsNone(memory['reset'])
    self.assertEqual('none', memory['resetMode'])
    self.assertBits(memory['readPorts'][0]['data'], instance.getModel(), 'RDATA', range(3))

  def testIndependentWriteClocks(self):
    instance = self.loadSV('''module sample(input c0, c1, e0, e1,
      input [1:0] a0, a1, input [2:0] d0, d1, output [2:0] q0, q1);
      reg [2:0] storage [0:3];
      always @(posedge c0) if (e0) storage[a0] <= d0;
      always @(posedge c1) if (e1) storage[a1] <= d1;
      assign q0 = storage[a0];
      assign q1 = storage[a1];
    endmodule''')
    model = instance.getModel()
    memory = instance.getMemoryInterface()
    self.assertIsNone(memory['clock'])
    self.assertIsNone(memory['reset'])
    self.assertEqual(2, len(memory['writePorts']))
    for p, port in enumerate(memory['writePorts']):
      self.assertEqual(model.getBusTerm('WCLK').getBusTermBit(p), port['clock'])
      self.assertEqual(model, port['clock'].getDesign())
      self.assertBits(port['address'], model, 'WADDR', range(p*2, p*2+2))
      self.assertBits(port['data'], model, 'WDATA', range(p*3, p*3+3))
      self.assertBits(port['mask'], model, 'WMASK', range(p*3, p*3+3))
      self.assertBits(port['enables'], model, 'WE', [p])

  def testInstanceOverridesAndDisconnectedPorts(self):
    original = self.writable()
    model = original.getModel()
    instance = naja.SNLInstance.create(original.getDesign(), model, 'other')
    for name, value in [('WIDTH', '2'), ('DEPTH', '3'), ('ABITS', '1')]:
      naja.SNLInstParameter.create(instance, model.getParameter(name), value)
    memory = instance.getMemoryInterface()
    self.assertEqual((2, 3, 1), tuple(memory[k] for k in ('width', 'depth', 'abits')))
    self.assertEqual((3, 4, 2), tuple(model.getMemoryInterface()[k] for k in ('width', 'depth', 'abits')))
    self.assertEqual([], memory['readPorts'])
    self.assertEqual([], memory['writePorts'])
    self.assertIsNone(memory['clock'])
    self.assertIsNone(memory['reset'])
    # C++ preserves model terminals (and their declared widths) under overrides.
    for term in original.getInstTerms():
      instance.getInstTerm(term.getBitTerm()).setNet(term.getNet())
    memory = instance.getMemoryInterface()
    self.assertEqual(model.getMemoryInterface()['readPorts'], memory['readPorts'])
    self.assertEqual(model.getMemoryInterface()['writePorts'], memory['writePorts'])
    instance.getInstParameter('WIDTH').setValue('invalid')
    with self.assertRaises(RuntimeError):
      instance.getMemoryInterface()

  def testNoMemoryAndDestroyedObjects(self):
    library = naja.NLLibrary.create(self.db)
    design = naja.SNLDesign.create(library, 'plain')
    model = naja.SNLDesign.create(library, 'memory_named_but_unmodeled')
    instance = naja.SNLInstance.create(design, model, 'plain_instance')
    self.assertFalse(model.hasMemoryInterface())
    self.assertFalse(design.hasMemoryInterface())
    for obj in (design, instance):
      with self.assertRaisesRegex(RuntimeError, 'has no memory'):
        obj.getMemoryInterface()
    instance.destroy()
    with self.assertRaises(RuntimeError):
      instance.getMemoryInterface()
    design.destroy()
    with self.assertRaises(RuntimeError):
      design.hasMemoryInterface()
    with self.assertRaises(RuntimeError):
      design.getMemoryInterface()

  def testLibertyControlsAndExtraInputs(self):
    source = """library(synthetic) {
      type(bits) { base_type : array; data_type : bit;
        bit_width : 3; bit_from : 2; bit_to : 0; downto : true; }
      type(addr) { base_type : array; data_type : bit;
        bit_width : 2; bit_from : 1; bit_to : 0; downto : true; }
      cell(storage) {
        memory() { type : ram; address_width : 2; word_width : 3; }
        pin(clk) { direction : input; clock : true; }
        pin(e0) { direction : input;
          timing() { related_pin : "clk"; timing_type : setup_rising; } }
        pin(e1) { direction : input;
          timing() { related_pin : "clk"; timing_type : setup_rising; } }
        bus(a) { bus_type : addr; direction : input; }
        bus(q) { bus_type : bits; direction : output;
          memory_read() { address : a; }
          timing() { related_pin : "clk"; timing_type : rising_edge; } }
        WRITE_BUSES
      }
    }"""
    buses = '\n'.join('''bus(%s) { bus_type : bits; direction : input;
      memory_write() { address : a; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; } }''' % name
      for name in ('d', 'mask', 'aux0', 'aux1'))
    with tempfile.TemporaryDirectory() as directory:
      path = os.path.join(directory, 'synthetic.lib')
      with open(path, 'w') as stream:
        stream.write(source.replace('WRITE_BUSES', buses))
      self.db.loadLibertyPrimitives([path])
    model = self.db.getLibrary('synthetic').getSNLDesign('storage')
    memory = model.getMemoryInterface()
    self.assertTrue(model.hasMemoryInterface())
    self.assertEqual((3, 4, 2), tuple(memory[k] for k in ('width', 'depth', 'abits')))
    self.assertEqual(model.getScalarTerm('clk'), memory['clock'])
    self.assertIsNone(memory['reset'])
    self.assertEqual(1, len(memory['readPorts']))
    self.assertEqual(1, len(memory['writePorts']))
    port = memory['writePorts'][0]
    self.assertBits(port['address'], model, 'a', [1, 0])
    self.assertBits(port['data'], model, 'd', [2, 1, 0])
    self.assertBits(port['mask'], model, 'mask', [2, 1, 0])
    self.assertEqual(2, len(port['extraWriteInputs']))
    for terms, name in zip(port['extraWriteInputs'], ('aux0', 'aux1')):
      self.assertBits(terms, model, name, [2, 1, 0])
    self.assertEqual([model.getScalarTerm('e0'), model.getScalarTerm('e1')], port['enables'])
    self.assertBits(memory['readPorts'][0]['data'], model, 'q', [2, 1, 0])
    top = naja.SNLDesign.create(naja.NLLibrary.create(self.db), 'top')
    instance = naja.SNLInstance.create(top, model, 'storage_instance')
    for bit in instance.getInstTerms():
      bit.setNet(naja.SNLScalarNet.create(top))
    self.assertEqual(memory, instance.getMemoryInterface())

  def testReset(self):
    instance = self.loadSV('''module sample(input clk, rst,
      input [1:0] a, input [2:0] d, output [2:0] q);
      logic [2:0] current [0:3], following [0:3];
      always_comb begin following = current; following[a] = d; end
      always_ff @(posedge clk or negedge rst) begin
        if (~rst) current <= '{3'd0, 3'd0, 3'd0, 3'd0};
        else current <= following;
      end
      assign q = current[a];
    endmodule''')
    for memory in (instance.getMemoryInterface(), instance.getModel().getMemoryInterface()):
      self.assertEqual('async_low', memory['resetMode'])
      self.assertEqual(instance.getModel().getScalarTerm('RST'), memory['reset'])
    for mode, asynchronous, active_low in [('async_high', '1', '0'),
        ('sync_low', '0', '1'), ('sync_high', '0', '0')]:
      with self.subTest(mode=mode):
        for name, value in [('RST_ASYNC', asynchronous), ('RST_ACTIVE_LOW', active_low)]:
          parameter = instance.getInstParameter(name)
          if parameter is None:
            parameter = naja.SNLInstParameter.create(instance, instance.getModel().getParameter(name), value)
          else:
            parameter.setValue(value)
        self.assertEqual(mode, instance.getMemoryInterface()['resetMode'])
        self.assertEqual('async_low', instance.getModel().getMemoryInterface()['resetMode'])


if __name__ == '__main__':
  unittest.main()
