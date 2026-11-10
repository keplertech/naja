# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import warnings
import naja

class SNLDesignModelingTest(unittest.TestCase):
  def setUp(self):
    universe = naja.NLUniverse.create()
    self.db = naja.NLDB.create(universe)
    self.designs = naja.NLLibrary.create(self.db)
    self.primitives = naja.NLLibrary.createPrimitives(self.db)

  def tearDown(self):
    del self.designs
    del self.primitives
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def testDeprecatedRoleAliases(self):
    model = naja.SNLDesign.createPrimitive(self.primitives, "alias_model")
    term = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "CLK")
    bus = naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Input, 1, 0, "D")
    term.setRole(naja.SNLTermRole.Clock)
    top = naja.SNLDesign.create(self.designs, "alias_top")
    instance = naja.SNLInstance.create(top, model)
    aliases = (
      ("is_clock", "isClock"), ("is_async_reset", "isAsyncReset"),
      ("is_async_set", "isAsyncSet"), ("is_sync_reset", "isSyncReset"),
      ("is_sync_set", "isSyncSet"), ("is_reset", "isReset"),
      ("is_enable", "isEnable"), ("is_data", "isData"),
      ("is_data_input", "isDataInput"), ("is_data_output", "isDataOutput"))
    for obj in (term, bus.getBusTermBit(0), instance.getInstTerm(term)):
      for alias, replacement in aliases:
        with self.subTest(type=type(obj).__name__, alias=alias):
          with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always", DeprecationWarning)
            expected = getattr(obj, replacement)()
            self.assertEqual([], caught)
            self.assertEqual(expected, getattr(obj, alias)())
          self.assertEqual(1, len(caught))
          self.assertIs(DeprecationWarning, caught[0].category)
          self.assertIn(alias + "() is deprecated", str(caught[0].message))
          self.assertIn("use " + replacement + "() instead", str(caught[0].message))
          self.assertEqual(__file__, caught[0].filename)
          with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            with self.assertRaises(DeprecationWarning):
              getattr(obj, alias)()

  def testLibraries(self):
    self.assertTrue(self.designs.isStandard())
    self.assertFalse(self.designs.isPrimitives())
    self.assertTrue(self.primitives.isPrimitives())
    self.assertFalse(self.primitives.isStandard())

  def testGroupedCombinatorialArcs(self):
    model = naja.SNLDesign.createPrimitive(self.primitives, "grouped")
    inputs = naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Input, 15, 0, "I")
    outputs = naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Output, 31, 0, "O")
    input_bits = [inputs.getBusTermBit(i) for i in range(16)]
    output_bits = [outputs.getBusTermBit(i) for i in range(32)]
    naja.SNLDesign.addCombinatorialArcs(input_bits[::-1], output_bits[1::2])
    naja.SNLDesign.addCombinatorialArcs(input_bits, output_bits[0])
    naja.SNLDesign.addCombinatorialArcs(input_bits, output_bits[2::2])
    owner = naja.SNLDesign.create(self.designs, "owner")
    instance = naja.SNLInstance.create(owner, model, "u")
    for bit in input_bits:
      self.assertEqual(output_bits, list(model.getCombinatorialOutputs(bit)))
      self.assertEqual([instance.getInstTerm(t) for t in output_bits],
                       list(instance.getCombinatorialOutputs(instance.getInstTerm(bit))))
    for bit in output_bits:
      self.assertEqual(input_bits, list(model.getCombinatorialInputs(bit)))
      self.assertEqual([instance.getInstTerm(t) for t in input_bits],
                       list(instance.getCombinatorialInputs(instance.getInstTerm(bit))))
    with self.assertRaises(RuntimeError):
      naja.SNLDesign.addCombinatorialArcs(input_bits, output_bits[1::2])
    with self.assertRaises(RuntimeError):
      naja.SNLDesign.addCombinatorialArcs(input_bits[0], output_bits[1])

  def testTermRoleDecoration(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    clock = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Input, "CLK")
    data = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Input, "D")
    reset = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Input, "RESET_B")
    output = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")

    clock.setRole(naja.SNLTermRole.Clock)
    data.setRole(naja.SNLTermRole.DataInput)
    reset.setRole(naja.SNLTermRole.AsyncReset, naja.SNLActiveLevel.Low)
    output.setRole(naja.SNLTermRole.DataOutput)
    for role in (
        naja.SNLTermRole.MemoryWriteAddress,
        naja.SNLTermRole.Other,
        naja.SNLTermRole.ScanInput,
        naja.SNLTermRole.ScanEnable):
      data.setRole(role)
      self.assertEqual(role, data.getRole())
    data.setRole(naja.SNLTermRole.DataInput)

    self.assertEqual(naja.SNLTermRole.Clock, clock.getRole())
    self.assertEqual(naja.SNLTermRole.DataInput, data.getRole())
    self.assertEqual(naja.SNLTermRole.AsyncReset, reset.getRole())
    self.assertEqual(naja.SNLActiveLevel.Low, reset.getResetActiveLevel())
    self.assertEqual(naja.SNLTermRole.DataOutput, output.getRole())
    self.assertEqual([clock], list(reg.getClockTerms()))
    self.assertEqual([reset], list(reg.getAsyncResetTerms()))
    self.assertEqual([data], list(reg.getDataInputTerms()))
    self.assertEqual([output], list(reg.getOutputTerms()))

    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, reg, "reg")
    aliases = {
      "is_clock": "isClock", "is_async_reset": "isAsyncReset",
      "is_async_set": "isAsyncSet", "is_sync_reset": "isSyncReset",
      "is_sync_set": "isSyncSet", "is_reset": "isReset", "is_enable": "isEnable",
      "is_data": "isData", "is_data_input": "isDataInput", "is_data_output": "isDataOutput",
    }
    for pin in (clock, data, reset, output):
      for obj in (pin, instance.getInstTerm(pin)):
        for old, new in aliases.items():
          with self.assertWarns(DeprecationWarning):
            self.assertEqual(getattr(obj, new)(), getattr(obj, old)())
    self.assertTrue(instance.getInstTerm(clock).isClock())
    self.assertTrue(instance.getInstTerm(data).isDataInput())
    self.assertTrue(instance.getInstTerm(reset).isAsyncReset())
    self.assertEqual(
      naja.SNLActiveLevel.Low,
      instance.getInstTerm(reset).getResetActiveLevel())
    self.assertTrue(instance.getInstTerm(output).isDataOutput())

    with self.assertRaises(RuntimeError):
      clock.setRole()
    with self.assertRaises(RuntimeError):
      clock.setRole("Clock")
    with self.assertRaises(RuntimeError):
      clock.setRole(1000)
    with self.assertRaises(RuntimeError):
      clock.setRole(naja.SNLTermRole.Clock, "High")
    with self.assertRaises(RuntimeError):
      clock.setRole(naja.SNLTermRole.Clock, 1000)

  def testRolesFromParameters(self):
    from itertools import product
    reg, pins, parameters, names, models = self.makeParameterizedDFF()
    role, level = naja.SNLTermRole, naja.SNLActiveLevel
    pins["R"].setRole(role.AsyncReset, level.Low)
    pins["L"].setRole(role.Enable)
    pins["I"].setRole(role.DataInput)
    pins["O"].setRole(role.DataOutput)
    self.assertFalse(reg.hasRolesFromParameters())
    entries = []
    for edge, init, load, sync, typ in product((0, 1), repeat=5):
      reset = (role.SyncSet if typ else role.SyncReset) if sync else (
        role.AsyncSet if typ else role.AsyncReset)
      entries.append({"values": [edge, init, load, sync, typ], "roles": {
        pins["CK"]: (role.Clock, level.NA),
        pins["R"]: (reset, level.High) if init else None,
        pins["L"]: (role.Enable, level.NA) if load else None}})
    reg.setRolesFromParameters(parameters=names, roles=entries)
    self.assertTrue(reg.hasRolesFromParameters())
    top = naja.SNLDesign.create(self.designs, "top")
    default = naja.SNLInstance.create(top, reg, "default")
    self.assertEqual(role.Other, default.getInstTerm(pins["R"]).getRole())
    self.assertEqual(role.Other, default.getInstTerm(pins["L"]).getRole())
    for entry in entries:
      with self.subTest(values=entry["values"]):
        instance = naja.SNLInstance.create(top, reg)
        overrides = [naja.SNLInstParameter.create(instance, parameter, f"1'b{value}")
          for parameter, value in zip(parameters, entry["values"])]
        for name in ("CK", "R", "L"):
          term = instance.getInstTerm(pins[name])
          occurrence = naja.SNLOccurrence(term)
          expected = entry["roles"][pins[name]]
          expected_role = expected[0] if expected else role.Other
          self.assertEqual(expected_role, term.getRole())
          self.assertEqual(expected_role, occurrence.getRole())
          expected_level = level.High if name == "R" and expected else level.NA
          self.assertEqual(expected_level, term.getResetActiveLevel())
          self.assertEqual(expected_level, occurrence.getResetActiveLevel())
          self.assertEqual(expected_role == role.Clock, term.isClock())
          self.assertEqual(expected_role == role.AsyncReset, term.isAsyncReset())
          self.assertEqual(expected_role == role.AsyncSet, term.isAsyncSet())
          self.assertEqual(expected_role == role.SyncReset, term.isSyncReset())
          self.assertEqual(expected_role == role.SyncSet, term.isSyncSet())
          self.assertEqual(expected_role in (role.AsyncReset, role.SyncReset), term.isReset())
          self.assertEqual(expected_role == role.Enable, term.isEnable())
        self.assertTrue(instance.getInstTerm(pins["I"]).isDataInput())
        self.assertTrue(instance.getInstTerm(pins["O"]).isDataOutput())
        # A query must observe changes on the same instance immediately.
        overrides[1].setValue("0")
        self.assertEqual(role.Other, instance.getInstTerm(pins["R"]).getRole())
        overrides[2].destroy()
        self.assertFalse(instance.getInstTerm(pins["L"]).isEnable())
    outer = naja.SNLDesign.create(self.designs, "outer")
    wrapper = naja.SNLInstance.create(outer, top, "wrapper")
    occurrence = naja.SNLOccurrence(naja.SNLPath(wrapper), default.getInstTerm(pins["R"]))
    self.assertEqual(role.Other, occurrence.getRole())
    self.assertEqual(role.AsyncReset, naja.SNLOccurrence(pins["R"]).getRole())
    # Model terms and design iterators remain static.
    self.assertEqual(role.AsyncReset, pins["R"].getRole())
    self.assertEqual([pins["R"]], list(reg.getAsyncResetTerms()))
    naja.SNLInstParameter.create(default, parameters[1], "1")
    self.assertTrue(default.getInstTerm(pins["R"]).isAsyncReset())
    self.assertEqual(role.AsyncReset, occurrence.getRole())
    # Replacing the role table takes effect without changing sequential models.
    reg.setRolesFromParameters(names, [{"values": [0, 1, 0, 0, 0], "roles": {
      pins["R"]: (role.SyncSet, level.Low), pins["L"]: (role.DataInput, level.NA),
      pins["O"]: None}}])
    self.assertTrue(default.getInstTerm(pins["R"]).isSyncSet())
    self.assertEqual(level.Low, default.getInstTerm(pins["R"]).getResetActiveLevel())
    self.assertTrue(default.getInstTerm(pins["L"]).isData())
    self.assertFalse(default.getInstTerm(pins["O"]).isDataOutput())
    self.assertTrue(reg.hasSequentialModelFromParameters())

  def testBusBitRolesFromParameters(self):
    model = naja.SNLDesign.createPrimitive(self.primitives, "fifo")
    bus = naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Input, 1, 0, "RSTI")
    low, high = bus.getBusTermBit(0), bus.getBusTermBit(1)
    mode = naja.SNLParameter.createBoolean(model, "use_arst", False)
    role, level = naja.SNLTermRole, naja.SNLActiveLevel
    model.setRolesFromParameters(["use_arst"], [
      {"values": [0], "roles": {low: (role.SyncReset, level.Low), high: None}},
      {"values": [1], "roles": {low: (role.AsyncReset, level.High), high: (role.Enable, level.NA)}}])
    top = naja.SNLDesign.create(self.designs, "top")
    instance = naja.SNLInstance.create(top, model)
    self.assertTrue(instance.getInstTerm(low).isSyncReset())
    self.assertFalse(instance.getInstTerm(high).isEnable())
    self.assertEqual(level.Low, instance.getInstTerm(low).getResetActiveLevel())
    naja.SNLInstParameter.create(instance, mode, "1'b1")
    self.assertTrue(instance.getInstTerm(low).isAsyncReset())
    self.assertTrue(instance.getInstTerm(high).isEnable())
    self.assertEqual(level.High, instance.getInstTerm(low).getResetActiveLevel())

  def testRolesFromParametersArgumentErrors(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "roles")
    cases = [
      ((), {}),
      (([],), {}),
      (([], [], []), {}),
      ((), {"parameters": []}),
      ((), {"roles": []}),
      (([], []), {"parameters": []}),
      ((), {"parameters": [], "roles": [], "unknown": []}),
    ]
    for args, kwargs in cases:
      with self.subTest(args=args, kwargs=kwargs):
        with self.assertRaises(TypeError):
          reg.setRolesFromParameters(*args, **kwargs)
    self.assertFalse(reg.hasRolesFromParameters())

  def testRolesFromParametersNonIntegerRoles(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "roles")
    pin = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "R")
    naja.SNLParameter.createDecimal(reg, "mode", 0)
    for role in (("reset", naja.SNLActiveLevel.High),
                 (naja.SNLTermRole.AsyncReset, "high")):
      with self.subTest(role=role):
        with self.assertRaisesRegex(
            RuntimeError, r"roles\[0\]: expected integer SNLTermRole and SNLActiveLevel values"):
          reg.setRolesFromParameters(
            ["mode"], [{"values": [0], "roles": {pin: role}}])
        self.assertFalse(reg.hasRolesFromParameters())

  def testRolesFromParametersErrors(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "roles")
    pin = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "R")
    parameter = naja.SNLParameter.createDecimal(reg, "mode", 0)
    naja.SNLParameter.createString(reg, "text", "0")
    entry = {"values": [0], "roles": {pin: None}}
    reg.setRolesFromParameters(["mode"], [entry])
    other = naja.SNLDesign.createPrimitive(self.primitives, "other")
    other_pin = naja.SNLScalarTerm.create(other, naja.SNLTerm.Direction.Input, "R")
    cases = [([], [entry]), (["mode"], []), (["missing"], [entry]),
      (["text"], [entry]), (["mode", "mode"], [entry]),
      (["mode"], [entry, entry]), (["mode"], [{"values": [], "roles": {}}]),
      (["mode"], [{"values": [0], "roles": {other_pin: None}}]),
      (["mode"], [{"values": [0], "roles": {pin: None, other_pin: None}}]),
      (["mode"], [{"values": [0], "roles": {pin: (999, 0)}}]),
      (["mode"], [{"values": [0], "roles": {pin: (0, 999)}}]),
      (["mode"], [{"values": [0], "roles": {pin: "reset"}}]),
      (["mode"], [{"values": [0], "roles": {"R": None}}]),
      (["mode"], [{"values": [-1], "roles": {}}]),
      (["mode"], [{"values": [2**64], "roles": {}}]),
      (["mode"], [{"values": ["0"], "roles": {}}]),
      ([1], [entry]), (["mode"], [None]), (["mode"], [{}]),
      ("mode", [entry]), (["mode"], {})]
    for names, entries in cases:
      with self.subTest(names=names, entries=entries):
        with self.assertRaises((RuntimeError, OverflowError)):
          reg.setRolesFromParameters(names, entries)
    top = naja.SNLDesign.create(self.designs, "top")
    with self.assertRaisesRegex(RuntimeError, "primitive"):
      top.setRolesFromParameters(["mode"], [entry])
    instance = naja.SNLInstance.create(top, reg, "ff")
    term = instance.getInstTerm(pin)
    self.assertEqual(naja.SNLTermRole.Other, term.getRole())
    override = naja.SNLInstParameter.create(instance, parameter, "1")
    for query in (term.getRole, term.isReset, term.isData, term.getResetActiveLevel,
                  naja.SNLOccurrence(term).getRole,
                  naja.SNLOccurrence(term).getResetActiveLevel):
      with self.assertRaisesRegex(RuntimeError, "mode=1"):
        query()
    override.setValue("1'bx")
    with self.assertRaisesRegex(RuntimeError, "invalid role parameter.*mode"):
      term.getRole()
    override.destroy()
    required = naja.SNLParameter.createDecimal(reg, "required")
    reg.setRolesFromParameters(["required"], [entry])
    with self.assertRaisesRegex(RuntimeError, "missing required role parameter.*required"):
      term.getRole()
    naja.SNLInstParameter.create(instance, required, "0")
    self.assertEqual(naja.SNLTermRole.Other, term.getRole())
    self.assertEqual(naja.SNLTermRole.Other, naja.SNLOccurrence().getRole())
    self.assertEqual(naja.SNLActiveLevel.NA, naja.SNLOccurrence(instance).getResetActiveLevel())

  def testSequentialModel(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "SCAN_REG")
    q = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")
    qn = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "QN")
    for name in ("CLK", "D", "SE", "SI", "RESET_B", "PRESET"):
      naja.SNLScalarTerm.create(
        reg, naja.SNLTerm.Direction.Input, name)

    self.assertFalse(reg.hasSequentialModel())
    reg.setSequentialModel(
      clocked_on="CLK",
      states=[{
        "name": "IQ",
        "inverted_name": "IQN",
        "next_state": "(SE & SI) | (!SE & D)",
        "clear": "!RESET_B",
        "preset": "PRESET",
      }],
      outputs=[(q, "IQ"), (qn, "IQN")])
    self.assertTrue(reg.hasSequentialModel())

  def makeParameterizedDFF(self):
    from itertools import product
    reg = naja.SNLDesign.createPrimitive(self.primitives, "PARAM_DFF")
    pins = {name: naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output if name == "O" else
      naja.SNLTerm.Direction.Input, name) for name in ("I", "CK", "L", "R", "O")}
    names = ["FALLING_EDGE", "USE_RESET", "USE_ENABLE", "SYNC_RESET", "RESET_VALUE"]
    parameters = [naja.SNLParameter.createBinary(reg, name, 1, 0) for name in names]
    models = []
    for edge, init, load, sync, typ in product((0, 1), repeat=5):
      next_state = "(L & I) | (!L & IQ)" if load else "I"
      state = {"name": "IQ"}
      if init:
        if sync:
          next_state = f"R | (!R & ({next_state}))" if typ else f"!R & ({next_state})"
        else:
          state["preset" if typ else "clear"] = "R"
      state["next_state"] = next_state
      models.append(dict(values=[edge, init, load, sync, typ],
        clocked_on="!CK" if edge else "CK", states=[state], outputs=[(pins["O"], "IQ")]))
    reg.setSequentialModelFromParameters(parameters=names, models=models)
    reg.addClockToOutputsArcs(pins["CK"], [pins["O"]])
    reg.addInputsToClockArcs([pins[name] for name in ("I", "L", "R")], pins["CK"])
    return reg, pins, parameters, names, models

  def testSequentialModelFromParameters(self):
    from itertools import product
    reg, pins, parameters, names, models = self.makeParameterizedDFF()
    top = naja.SNLDesign.create(self.designs, "top")
    instance = naja.SNLInstance.create(top, reg, "ff")
    self.assertTrue(reg.hasSequentialModel())
    with self.assertRaisesRegex(RuntimeError, "requires an instance"):
      reg.getSequentialModel()
    self.assertTrue(reg.hasSequentialModelFromParameters())
    default = instance.getSequentialModel()
    self.assertEqual(("term", pins["I"]), default["states"][0]["next_state"])
    self.assertIsNone(default["states"][0]["clear"])
    overrides = [naja.SNLInstParameter.create(instance, parameter, "0") for parameter in parameters]
    for values in product((0, 1), repeat=5):
      with self.subTest(values=values):
        edge, init, load, sync, typ = values
        for override, value in zip(overrides, values):
          override.setValue(f"1'b{value}")
        model = instance.getSequentialModel()
        self.assertEqual(model, instance.getSequentialModel())
        self.assertEqual("flip_flop", model["kind"])
        self.assertEqual(("not", ("term", pins["CK"])) if edge else
          ("term", pins["CK"]), model["clocked_on"])
        state = model["states"][0]
        self.assertEqual(bool(init and not sync and not typ), state["clear"] is not None)
        self.assertEqual(bool(init and not sync and typ), state["preset"] is not None)
        self.assertEqual([(pins["O"], ("state", 0))], model["outputs"])
        for d, l, r, q in product((False, True), repeat=4):
          def evaluate(tree):
            if isinstance(tree, bool): return tree
            op = tree[0]
            if op == "term": return {pins["I"]: d, pins["L"]: l, pins["R"]: r}[tree[1]]
            if op == "state": return q
            if op == "not": return not evaluate(tree[1])
            if op == "and": return evaluate(tree[1]) and evaluate(tree[2])
            if op == "or": return evaluate(tree[1]) or evaluate(tree[2])
            return evaluate(tree[1]) != evaluate(tree[2])
          expected = q if load and not l else d
          if init and sync and r: expected = bool(typ)
          self.assertEqual(expected, evaluate(state["next_state"]))
          if state["clear"] is not None: self.assertEqual(r, evaluate(state["clear"]))
          if state["preset"] is not None: self.assertEqual(r, evaluate(state["preset"]))
    clone = top.clone("clone")
    self.assertEqual(model, clone.getInstance("ff").getSequentialModel())
    for override in overrides: override.destroy()
    self.assertEqual(default, instance.getSequentialModel())
    reg.setSequentialModel(clocked_on="CK", states=[dict(name="IQ", next_state="I")],
      outputs=[(pins["O"], "IQ")])
    self.assertEqual(reg.getSequentialModel(), instance.getSequentialModel())
    # Redefining a table clears cached static/parameter selections.
    reg.setSequentialModelFromParameters(names, models)
    self.assertEqual(default, instance.getSequentialModel())

  def testSequentialModelParameterErrors(self):
    reg, pins, parameters, names, models = self.makeParameterizedDFF()
    top = naja.SNLDesign.create(self.designs, "top")
    instance = naja.SNLInstance.create(top, reg, "ff")
    required = naja.SNLParameter.createBinary(reg, "required", 1)
    entry = dict(models[0], values=[0])
    reg.setSequentialModelFromParameters(["required"], [entry])
    with self.assertRaisesRegex(RuntimeError, "Missing required.*required.*ff") as caught:
      instance.getSequentialModel()
    self.assertIn("PARAM_DFF", str(caught.exception))
    self.assertIn("no instance override and no parameter default", str(caught.exception))
    override = naja.SNLInstParameter.create(instance, required, "0")
    instance.getSequentialModel()
    override.setValue("1")
    with self.assertRaisesRegex(RuntimeError, "No sequential model matches") as caught:
      instance.getSequentialModel()
    self.assertIn("required=1", str(caught.exception))
    self.assertIn("raw <1>, override", str(caught.exception))
    self.assertIn("available keys in declared parameter order=[[required=0]]", str(caught.exception))
    for invalid in ("1'bx", "1'bz", "garbage", "-1", "18446744073709551616"):
      override.setValue(invalid)
      with self.assertRaisesRegex(RuntimeError, "Sequential model parameter.*required.*ff") as caught:
        instance.getSequentialModel()
      self.assertIn("raw value <" + invalid + "> from instance override", str(caught.exception))
      self.assertIn("PARAM_DFF", str(caught.exception))
    override.destroy()
    with self.assertRaisesRegex(RuntimeError, "Missing required"):
      instance.getSequentialModel()
    for bad_names, bad_models in (([], [entry]), (["missing"], [entry]),
        (["required", "required"], [entry]), (["required"], []),
        (["required"], [entry, entry]), (["required"], [dict(entry, values=[0, 1])]),
        (["required"], [dict(entry, values=["0"])]),
        (["required"], [dict(entry, states="bad")]),
        (["required"], [dict(entry, clocked_on="unknown_pin")]),
        (["required"], [dict(entry, states=[dict(name="IQ", next_state="unknown")])])):
      with self.subTest(names=bad_names, models=bad_models):
        with self.assertRaises(RuntimeError):
          reg.setSequentialModelFromParameters(bad_names, bad_models)
    # Failed declarations preserve the previous table.
    naja.SNLInstParameter.create(instance, required, "0")
    self.assertEqual(("term", pins["I"]), instance.getSequentialModel()["states"][0]["next_state"])

  def testSequentialModelDeclarationDiagnostics(self):
    reg, pins, parameters, names, models = self.makeParameterizedDFF()
    cases = [
      (tuple(names), models, ["PARAM_DFF", "expected lists", "parameters="]),
      (names, tuple(models), ["PARAM_DFF", "expected lists", "models="]),
      ([42], models, ["PARAM_DFF", "parameters[0]", "parameter name string", "42"]),
      (names, [None], ["PARAM_DFF", "models[0]=None", "expected a dictionary"]),
      ([names[0]], [dict(models[0], values=[-1])],
       ["PARAM_DFF", "models[0]", "values[0]", "outside unsigned 64-bit range"]),
      ([names[0]], [dict(models[0], values=[1 << 64])],
       ["PARAM_DFF", "models[0]", "values[0]", "outside unsigned 64-bit range"]),
      ([names[0]], [dict(models[0], values=[0], clocked_on="absent")],
       ["PARAM_DFF", "models[0]", "field=clocked_on", "absent"]),
      ([names[0]], [dict(models[0], values=[0], states=[dict(name="IQ")])],
       ["PARAM_DFF", "models[0]", "field=states[0].next_state", "requires `next_state`"]),
      ([names[0]], [dict(models[0], values=["wrong"])],
       ["PARAM_DFF", "models[0]", "values[0]='wrong'", "unsigned integer"]),
      (["undeclared"], [dict(models[0], values=[0])],
       ["PARAM_DFF", "parameters[0] <undeclared>", "not declared"]),
      ([names[0]], [dict(models[0], values=[0, 1])],
       ["PARAM_DFF", "FALLING_EDGE=0", "key has 2 values; expected 1"]),
    ]
    for selectors, entries, details in cases:
      with self.subTest(details=details):
        with self.assertRaises(RuntimeError) as caught:
          reg.setSequentialModelFromParameters(selectors, entries)
        for detail in details:
          self.assertIn(detail, str(caught.exception))

    for args, kwargs in (((), {}), ((names,), {}), ((names, models), {"unexpected": True})):
      with self.subTest(args=args, kwargs=kwargs):
        with self.assertRaises(RuntimeError) as caught:
          reg.setSequentialModelFromParameters(*args, **kwargs)
        for detail in ("PARAM_DFF", "expected parameters and models arguments", "args=", "kwargs="):
          self.assertIn(detail, str(caught.exception))

  def testSequentialModelConstantRoundTrip(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "CONSTANT_REG")
    q = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "CLK")
    for value in (False, True):
      with self.subTest(value=value):
        reg.setSequentialModel(clocked_on="CLK",
          states=[dict(name="IQ", next_state=str(int(value)))], outputs=[(q, "IQ")])
        self.assertIs(reg.getSequentialModel()["states"][0]["next_state"], value)

  def testSequentialModelInstanceErrors(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "NO_MODEL")
    top = naja.SNLDesign.create(self.designs, "top")
    instance = naja.SNLInstance.create(top, reg, "ff")
    with self.assertRaises(RuntimeError) as caught:
      instance.getSequentialModel()
    for detail in ("ff", "NO_MODEL", "has no sequential model", "attach a static model or parameter table"):
      self.assertIn(detail, str(caught.exception))
    instance.destroy()
    with self.assertRaisesRegex(RuntimeError, "destroyed instance.*use a live SNLInstance"):
      instance.getSequentialModel()

  def testSequentialModelStateOptions(self):
    clear_preset_values = ("zero", "one", "hold", "toggle", "unknown")
    for index, clear_preset_value in enumerate(clear_preset_values):
      with self.subTest(clear_preset_value=clear_preset_value):
        reg = naja.SNLDesign.createPrimitive(
          self.primitives, f"STATE_OPTIONS_{index}")
        q = naja.SNLScalarTerm.create(
          reg, naja.SNLTerm.Direction.Output, "Q")
        for name in ("CLK", "D", "PRESET"):
          naja.SNLScalarTerm.create(
            reg, naja.SNLTerm.Direction.Input, name)

        reg.setSequentialModel(
          clocked_on="CLK",
          states=[{
            "name": "IQ",
            "inverted_name": None,
            "next_state": "D",
            "clear": None,
            "preset": "PRESET" if index == 0 else None,
            "clear_preset_value": clear_preset_value,
          }],
          outputs=[(q, "IQ")],
          kind="latch" if index == 0 else "flip_flop")
        self.assertTrue(reg.hasSequentialModel())

  def testSequentialModelStateErrors(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "STATE_ERRORS")
    q = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")
    for name in ("CLK", "D"):
      naja.SNLScalarTerm.create(
        reg, naja.SNLTerm.Direction.Input, name)

    def set_states(states, kind="flip_flop"):
      reg.setSequentialModel(
        clocked_on="CLK", states=states, outputs=[(q, "IQ")], kind=kind)

    with self.assertRaisesRegex(
        RuntimeError, "malformed SNLDesign.setSequentialModel method"):
      reg.setSequentialModel()
    with self.assertRaisesRegex(
        RuntimeError, "states must be dictionaries"):
      set_states([None])
    with self.assertRaisesRegex(RuntimeError, r"state requires `name`"):
      set_states([{"next_state": "D"}])
    with self.assertRaisesRegex(RuntimeError, r"`name` must be a string"):
      set_states([{"name": 0, "next_state": "D"}])
    with self.assertRaisesRegex(
        RuntimeError, r"`inverted_name` must be a string"):
      set_states([{"name": "IQ", "inverted_name": 0, "next_state": "D"}])
    with self.assertRaisesRegex(
        RuntimeError, r"duplicate state name `IQ`"):
      set_states([
        {"name": "IQ", "next_state": "D"},
        {"name": "IQ", "next_state": "D"},
      ])
    with self.assertRaisesRegex(
        RuntimeError, "kind must be flip_flop or latch"):
      set_states([{"name": "IQ", "next_state": "D"}], kind="register")
    with self.assertRaisesRegex(
        RuntimeError,
        "`clear_preset_value` must be zero, one, hold, toggle, or unknown"):
      set_states([{
        "name": "IQ",
        "next_state": "D",
        "clear_preset_value": "invalid",
      }])

  def testSequentialModelOutputErrors(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    q = naja.SNLScalarTerm.create(
      reg, naja.SNLTerm.Direction.Output, "Q")
    for name in ("CLK", "D"):
      naja.SNLScalarTerm.create(
        reg, naja.SNLTerm.Direction.Input, name)
    states = [{"name": "IQ", "next_state": "D"}]

    def set_outputs(outputs):
      reg.setSequentialModel(
        clocked_on="CLK", states=states, outputs=outputs)

    with self.assertRaisesRegex(
        RuntimeError, "expects lists for states and outputs"):
      set_outputs(((q, "IQ"),))
    for outputs in ([q], [(q,)]):
      with self.assertRaisesRegex(
          RuntimeError,
          r"outputs must be \(SNLBitTerm, expression\) tuples"):
        set_outputs(outputs)
    for outputs in ([("Q", "IQ")], [(q, 0)]):
      with self.assertRaisesRegex(
          RuntimeError,
          r"outputs must be \(SNLBitTerm, expression\) tuples"):
        set_outputs(outputs)

    foreign = naja.SNLDesign.createPrimitive(self.primitives, "FOREIGN")
    foreign_q = naja.SNLScalarTerm.create(
      foreign, naja.SNLTerm.Direction.Output, "Q")
    with self.assertRaisesRegex(
        RuntimeError, "output term belongs to another design"):
      set_outputs([(foreign_q, "IQ")])

  def testLoweredSequentialTermRoles(self):
    formats_path = os.environ.get('FORMATS_PATH')
    self.assertIsNotNone(formats_path)
    async_path = os.path.join(
      formats_path, "systemverilog", "benchmarks",
      "seq_timing_event_list_negedge_reset_supported",
      "seq_timing_event_list_negedge_reset_supported.sv")
    top = self.db.loadSystemVerilog([async_path])
    dffrn_inst = next(
      inst for inst in top.getPrimitiveInstances()
      if inst.getModel().getName().startswith("naja_dffrn"))
    dffrn = dffrn_inst.getModel()

    clocks = list(dffrn.getClockTerms())
    resets = list(dffrn.getAsyncResetTerms())
    data_inputs = list(dffrn.getDataInputTerms())
    outputs = list(dffrn.getOutputTerms())
    self.assertEqual(1, len(clocks))
    self.assertEqual(1, len(resets))
    self.assertEqual(8, len(data_inputs))
    self.assertEqual(8, len(outputs))
    self.assertEqual(naja.SNLTermRole.Clock, clocks[0].getRole())
    self.assertTrue(clocks[0].isClock())
    self.assertEqual(naja.SNLTermRole.AsyncReset, resets[0].getRole())
    self.assertEqual(naja.SNLActiveLevel.Low, resets[0].getResetActiveLevel())
    self.assertTrue(resets[0].isAsyncReset())
    self.assertTrue(resets[0].isReset())
    self.assertFalse(resets[0].isData())
    self.assertEqual(9, len(list(naja.SNLDesign.getClockRelatedInputs(clocks[0]))))
    self.assertTrue(all(term.isDataInput() for term in data_inputs))
    self.assertTrue(all(term.isDataOutput() for term in outputs))
    self.assertTrue(data_inputs[0].isData())
    self.assertTrue(outputs[0].isData())

    reset_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.isAsyncReset())
    self.assertEqual(naja.SNLTermRole.AsyncReset, reset_inst_term.getRole())
    self.assertEqual(
      naja.SNLActiveLevel.Low, reset_inst_term.getResetActiveLevel())
    self.assertTrue(reset_inst_term.isReset())
    self.assertFalse(reset_inst_term.isAsyncSet())
    self.assertFalse(reset_inst_term.isData())

    clock_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.isClock())
    data_input_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.isDataInput())
    data_output_inst_term = next(
      term for term in dffrn_inst.getInstTerms() if term.isDataOutput())
    self.assertTrue(data_input_inst_term.isData())
    self.assertTrue(data_output_inst_term.isData())
    self.assertFalse(clock_inst_term.isEnable())
    self.assertFalse(clock_inst_term.isData())
    self.assertFalse(data_input_inst_term.isClock())
    self.assertFalse(data_input_inst_term.isDataOutput())
    self.assertFalse(data_output_inst_term.isDataInput())

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as async_set_file:
      async_set_file.write("""
module py_async_set_role_coverage(
  input logic clk,
  input logic set,
  input logic d,
  output logic q
);
  always_ff @(posedge clk or posedge set) begin
    if (set) q <= 1'b1;
    else q <= d;
  end
endmodule
""")
      async_set_path = async_set_file.name
    try:
      async_set_db = naja.NLDB.create(naja.NLUniverse.get())
      async_set_top = async_set_db.loadSystemVerilog([async_set_path])
      dffs_inst = next(
        inst for inst in async_set_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffs"))
      dffs = dffs_inst.getModel()
      sets = list(dffs.getAsyncSetTerms())
      self.assertEqual(1, len(sets))
      self.assertEqual(naja.SNLTermRole.AsyncSet, sets[0].getRole())
      set_inst_term = next(
        term for term in dffs_inst.getInstTerms() if term.isAsyncSet())
      set_bit_term = set_inst_term.getBitTerm()
      self.assertEqual(naja.SNLTermRole.AsyncSet, set_inst_term.getRole())
      self.assertEqual(sets[0], set_bit_term)
      self.assertFalse(set_inst_term.isReset())
      self.assertTrue(set_bit_term.isAsyncSet())
      self.assertFalse(set_bit_term.isReset())
    finally:
      os.remove(async_set_path)

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as enable_file:
      enable_file.write("""
module py_enable_role_coverage(
  input logic clk,
  input logic en,
  input logic d,
  output logic q
);
  always_ff @(posedge clk) begin
    if (en) q <= d;
    else q <= q;
  end
endmodule
""")
      enable_path = enable_file.name
    try:
      enable_db = naja.NLDB.create(naja.NLUniverse.get())
      enable_top = enable_db.loadSystemVerilog([enable_path])
      dffe_inst = next(
        inst for inst in enable_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffe"))
      enable_bit_term = next(
        term for term in dffe_inst.getModel().getBitTerms()
        if term.isEnable())
      self.assertEqual(naja.SNLTermRole.Enable, enable_bit_term.getRole())
      self.assertTrue(enable_bit_term.isEnable())
      self.assertFalse(enable_bit_term.isData())
    finally:
      os.remove(enable_path)

    sync_path = os.path.join(
      formats_path, "systemverilog", "benchmarks",
      "seq_reset_action_supported", "seq_reset_action_supported.sv")
    sync_db = naja.NLDB.create(naja.NLUniverse.get())
    sync_top = sync_db.loadSystemVerilog([sync_path])
    dff = next(
      inst.getModel() for inst in sync_top.getPrimitiveInstances()
      if inst.getModel().getName().startswith("naja_dff__"))
    self.assertEqual([], list(dff.getAsyncResetTerms()))
    self.assertTrue(all(
      term.getRole() != naja.SNLTermRole.AsyncReset
      for term in dff.getBitTerms()))

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as sync_reset_file:
      sync_reset_file.write("""
module py_sync_reset_role_coverage(
  input logic clk,
  input logic rst,
  input logic [1:0] d,
  output logic [1:0] q
);
  always_ff @(posedge clk) begin
    if (rst) q <= 2'b00;
    else q <= d;
  end
endmodule
""")
      sync_reset_path = sync_reset_file.name
    try:
      sync_reset_db = naja.NLDB.create(naja.NLUniverse.get())
      sync_reset_top = sync_reset_db.loadSystemVerilog([sync_reset_path])
      dffsr_inst = next(
        inst for inst in sync_reset_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffsr"))
      dffsr = dffsr_inst.getModel()
      sync_resets = list(dffsr.getSyncResetTerms())
      self.assertEqual(1, len(sync_resets))
      self.assertEqual(naja.SNLTermRole.SyncReset, sync_resets[0].getRole())
      self.assertEqual(
        naja.SNLActiveLevel.High, sync_resets[0].getResetActiveLevel())
      self.assertTrue(sync_resets[0].isSyncReset())
      self.assertTrue(sync_resets[0].isReset())
      self.assertEqual([], list(dffsr.getAsyncResetTerms()))
      sync_reset_inst_term = next(
        term for term in dffsr_inst.getInstTerms() if term.isSyncReset())
      self.assertEqual(naja.SNLTermRole.SyncReset, sync_reset_inst_term.getRole())
      self.assertTrue(sync_reset_inst_term.isReset())
      self.assertFalse(sync_reset_inst_term.isAsyncReset())
    finally:
      os.remove(sync_reset_path)

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".sv", delete=False) as sync_set_file:
      sync_set_file.write("""
module py_sync_set_role_coverage(
  input logic clk,
  input logic set,
  input logic en,
  input logic [2:0] d,
  output logic [2:0] q
);
  always_ff @(posedge clk) begin
    if (set) q <= 3'b111;
    else if (en) q <= d;
  end
endmodule
""")
      sync_set_path = sync_set_file.name
    try:
      sync_set_db = naja.NLDB.create(naja.NLUniverse.get())
      sync_set_top = sync_set_db.loadSystemVerilog([sync_set_path])
      dffsse_inst = next(
        inst for inst in sync_set_top.getPrimitiveInstances()
        if inst.getModel().getName().startswith("naja_dffsse"))
      dffsse = dffsse_inst.getModel()
      sync_sets = list(dffsse.getSyncSetTerms())
      self.assertEqual(1, len(sync_sets))
      self.assertEqual(naja.SNLTermRole.SyncSet, sync_sets[0].getRole())
      self.assertEqual(
        naja.SNLActiveLevel.High, sync_sets[0].getResetActiveLevel())
      self.assertTrue(sync_sets[0].isSyncSet())
      self.assertFalse(sync_sets[0].isReset())
      self.assertFalse(sync_sets[0].isSyncReset())
      sync_set_inst_term = next(
        term for term in dffsse_inst.getInstTerms() if term.isSyncSet())
      self.assertEqual(naja.SNLTermRole.SyncSet, sync_set_inst_term.getRole())
      self.assertTrue(sync_set_inst_term.isSyncSet())
      self.assertFalse(sync_set_inst_term.isReset())
      self.assertFalse(sync_set_inst_term.isSyncReset())
    finally:
      os.remove(sync_set_path)

  def testCombi(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    naja.SNLDesign.addCombinatorialArcs([i0, i1], o)
    self.assertTrue(design.isPrimitive())
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(i0)))
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(i1)))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o)))
    self.assertEqual(0, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(o)))
    inputs = [t for t in naja.SNLDesign.getCombinatorialInputs(o)]
    self.assertEqual(2, len(inputs))
    self.assertEqual(i0, inputs[0])
    self.assertEqual(i1, inputs[1])
    outputs = [t for t in naja.SNLDesign.getCombinatorialOutputs(i0)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])
    outputs = [t for t in naja.SNLDesign.getCombinatorialOutputs(i1)]
    self.assertEqual(1, len(outputs))
    self.assertEqual(o, outputs[0])

    #create instance
    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, design, "instance")
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i0))))
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(i1))))
    self.assertEqual(2, sum(1 for t in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))))
    self.assertEqual(0, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(o))))
    inputs = [it for it in naja.SNLInstance.getCombinatorialInputs(instance.getInstTerm(o))]
    self.assertEqual(2, len(inputs))
    self.assertEqual(instance.getInstTerm(i0), inputs[0])
    self.assertEqual(instance.getInstTerm(i1), inputs[1])
    
    self.assertEqual(1, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))))
    outputs = [t for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i0))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])
    self.assertEqual(1, sum(1 for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))))
    outputs = [t for t in naja.SNLInstance.getCombinatorialOutputs(instance.getInstTerm(i1))]
    self.assertEqual(1, len(outputs))
    self.assertEqual(instance.getInstTerm(o), outputs[0])

  def testSeq(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d0 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "D0")
    q0 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q0")
    d1 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "D1")
    q1 = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Output, "Q1")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs(d0, c)
    naja.SNLDesign.addClockToOutputsArcs(c, q0)
    naja.SNLDesign.addInputsToClockArcs([d1], c)
    naja.SNLDesign.addClockToOutputsArcs(c, [q1])
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))
    self.assertEqual([c], list(naja.SNLDesign.getInputRelatedClocks(d0)))
    self.assertEqual([c], list(naja.SNLDesign.getInputRelatedClocks(d1)))
    self.assertEqual([c], list(naja.SNLDesign.getOutputRelatedClocks(q0)))
    self.assertEqual([c], list(naja.SNLDesign.getOutputRelatedClocks(q1)))

    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, reg, "reg")
    ic = instance.getInstTerm(c)
    id0 = instance.getInstTerm(d0)
    iq0 = instance.getInstTerm(q0)
    self.assertEqual(
      [id0, instance.getInstTerm(d1)],
      list(instance.getClockRelatedInputs(ic)))
    self.assertEqual(
      [iq0, instance.getInstTerm(q1)],
      list(instance.getClockRelatedOutputs(ic)))
    self.assertEqual([ic], list(instance.getInputRelatedClocks(id0)))
    self.assertEqual([ic], list(instance.getOutputRelatedClocks(iq0)))

  def testParameterizedCombinatorialArcs(self):
    gate = naja.SNLDesign.createPrimitive(self.primitives, "PARAM_GATE")
    mode = naja.SNLParameter.createString(gate, "MODE", "NORMAL")
    i0 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Input, "I1")
    o0 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Output, "O0")
    o1 = naja.SNLScalarTerm.create(
      gate, naja.SNLTerm.Direction.Output, "O1")

    gate.setTimingModelParameter("MODE", "NORMAL")
    naja.SNLDesign.addCombinatorialArcs(i0, o0)
    naja.SNLDesign.addCombinatorialArcs(i1, o1)
    naja.SNLDesign.addCombinatorialArcs("CROSS", i0, o1)
    naja.SNLDesign.addCombinatorialArcs("CROSS", i1, o0)

    self.assertEqual([o0], list(gate.getCombinatorialOutputs(i0)))
    self.assertEqual([o1], list(gate.getCombinatorialOutputs(i1)))

    top = naja.SNLDesign.create(self.designs, "TOP")
    normal = naja.SNLInstance.create(top, gate, "normal")
    cross = naja.SNLInstance.create(top, gate, "cross")
    naja.SNLInstParameter.create(cross, mode, "CROSS")
    self.assertEqual(
      normal.getInstTerm(o0),
      next(iter(normal.getCombinatorialOutputs(normal.getInstTerm(i0)))))
    self.assertEqual(
      cross.getInstTerm(o1),
      next(iter(cross.getCombinatorialOutputs(cross.getInstTerm(i0)))))

    with self.assertRaises(RuntimeError):
      gate.setTimingModelParameter("UNKNOWN", "NORMAL")
    with self.assertRaises(RuntimeError):
      gate.setTimingModelParameter("MODE")
    with self.assertRaises(RuntimeError):
      naja.SNLDesign.addCombinatorialArcs(1, i0, o0)

  def testCombiWithBusses0(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "DESIGN")
    o = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Output, 3, 0, "O")
    d = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, 3, 0, "D")
    naja.SNLDesign.addCombinatorialArcs(d, o)
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o.getBusTermBit(0))))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(d.getBusTermBit(0))))

  def testCombiWithBusses1(self):
    carry4 = naja.SNLDesign.createPrimitive(self.primitives, "CARRY4")
    o = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "O")
    co = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Output, 3, 0, "CO")
    di = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "DI")
    s = naja.SNLBusTerm.create(carry4, naja.SNLTerm.Direction.Input, 3, 0, "S")
    cyinit  = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CYINIT")
    ci = naja.SNLScalarTerm.create(carry4, naja.SNLTerm.Direction.Input, "CI")
    o_bits = [b for b in o.getBits()]
    co_bits = [b for b in co.getBits()]
    di_bits = [b for b in di.getBits()] 
    s_bits = [b for b in s.getBits()] 
    #cyinit and ci are in combinatorial dependency with o and co outputs 
    naja.SNLDesign.addCombinatorialArcs([cyinit, ci], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[0], [o, co])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(s_bits[3], co_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], [o_bits[1], o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[0], co)
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [o_bits[2], o_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[1], [co_bits[1], co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], o_bits[3])
    naja.SNLDesign.addCombinatorialArcs(di_bits[2], [co_bits[2], co_bits[3]])
    naja.SNLDesign.addCombinatorialArcs(di_bits[3], co_bits[3])
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(cyinit)))
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(ci)))
    self.assertEqual(8, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[0])))
    self.assertEqual(6, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[1])))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[2])))
    self.assertEqual(2, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(s_bits[3])))
    self.assertEqual(3, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[0])))
    self.assertEqual(5, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[1])))
    self.assertEqual(7, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[2])))
    self.assertEqual(9, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bits[3])))

  def testCombiWithBusses2(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    o = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Output, 3, 0, "O")
    i = naja.SNLBusTerm.create(design, naja.SNLTerm.Direction.Input, 3, 0, "I")
    naja.SNLDesign.addCombinatorialArcs([i], [o])
    for o_bit in o.getBits():
      self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialInputs(o_bit)))
    for i_bit in i.getBits():
      self.assertEqual(4, sum(1 for t in naja.SNLDesign.getCombinatorialOutputs(i_bit)))

  def testSeqWithBusses0(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Input, 3, 0, "D")
    q = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Output, 3, 0, "Q")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs(d, c)
    naja.SNLDesign.addClockToOutputsArcs(c, q)
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testSeqWithBusses1(self):
    reg = naja.SNLDesign.createPrimitive(self.primitives, "REG")
    d = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Input, 3, 0, "D")
    q = naja.SNLBusTerm.create(reg, naja.SNLTerm.Direction.Output, 3, 0, "Q")
    c = naja.SNLScalarTerm.create(reg, naja.SNLTerm.Direction.Input, "C")
    naja.SNLDesign.addInputsToClockArcs([d], c)
    naja.SNLDesign.addClockToOutputsArcs(c, [q])
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedInputs(c)))
    self.assertEqual(4, sum(1 for t in naja.SNLDesign.getClockRelatedOutputs(c)))

  def testCreationErrors(self):
    prim = naja.SNLDesign.createPrimitive(self.primitives, "design")
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.createPrimitive(self.designs, "design")

  def testCombiErrors(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    i0 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "O")
    #wrong type
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(i0, i1, o)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(design, o)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs(i0, design)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs([design, i0], [o, design])
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addCombinatorialArcs([i0], [o, design])

  def testSeqErrors(self):
    design = naja.SNLDesign.createPrimitive(self.primitives, "design")
    d = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "D")
    q = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Output, "Q")
    c = naja.SNLScalarTerm.create(design, naja.SNLTerm.Direction.Input, "C")
    #wrong type
    naja.SNLDesign.addClockToOutputsArcs(c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(d, [c, q])
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs(design, c)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addInputsToClockArcs([design], c)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(d, c, q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs([d, c], q)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(c, design)
    with self.assertRaises(RuntimeError) as context: naja.SNLDesign.addClockToOutputsArcs(c, [design])
    with self.assertRaises(RuntimeError) as context: design.getCombinatorialInputs(design)
    with self.assertRaises(RuntimeError) as context: design.getCombinatorialOutputs(design)

    #create instance
    top = naja.SNLDesign.create(self.designs, "TOP")
    instance = naja.SNLInstance.create(top, design, "instance")
    with self.assertRaises(RuntimeError) as context: instance.getCombinatorialInputs(d)
    with self.assertRaises(RuntimeError) as context: instance.getCombinatorialOutputs(q)
   
if __name__ == '__main__':
  unittest.main()
