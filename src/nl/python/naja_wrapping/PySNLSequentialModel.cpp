// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLSequentialModel.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"

#include "SNLBooleanTree.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLDesignModeling.h"

namespace PYNAJA {

using namespace naja::NL;

namespace {

std::string describePythonObject(PyObject* object) {
  if (!object) return "<missing>";
  auto* representation = PyObject_Repr(object);
  if (!representation) { PyErr_Clear(); return std::string("<type ") + Py_TYPE(object)->tp_name + ">"; }
  const char* text = PyUnicode_AsUTF8(representation);
  const std::string result = text ? text : "<unprintable>";
  Py_DECREF(representation);
  if (!text) PyErr_Clear();
  return result;
}

void setContextualError(const std::string& context, const std::string& detail = "") {
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  std::string reason = detail;
  if (value) {
    auto* message = PyObject_Str(value);
    if (message) {
      if (!reason.empty()) reason += "; ";
      const char* text = PyUnicode_AsUTF8(message);
      if (text) reason += text;
      Py_DECREF(message);
    }
  }
  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);
  PyErr_Clear();
  setError(context + ": " + reason);
}

const char* getStringField(
    PyObject* dictionary, const char* field, bool required) {
  auto* object = PyDict_GetItemString(dictionary, field);
  if (object == nullptr) {
    if (required) {
      setError(std::string("SNLDesign.setSequentialModel() state requires `") +
               field + "`");
    }
    return nullptr;
  }
  if (object == Py_None && not required) {
    return nullptr;
  }
  if (not PyUnicode_Check(object)) {
    setError(std::string("SNLDesign.setSequentialModel() `") + field +
             "` must be a string");
    return nullptr;
  }
  return PyUnicode_AsUTF8(object);
}

SNLDesignModeling::BooleanExpression parseExpression(
    const SNLDesign* design,
    const char* expression,
    const SNLBooleanTree::StateIdentifiers& stateIdentifiers) {
  SNLBooleanTree tree;
  tree.parse(design, expression, stateIdentifiers);
  return tree.getBooleanExpression();
}

bool addStateIdentifier(
    SNLBooleanTree::StateIdentifiers& stateIdentifiers,
    const char* name,
    size_t index,
    bool inverted) {
  if (stateIdentifiers.emplace(name, std::make_pair(index, inverted)).second) {
    return true;
  }
  setError(std::string("SNLDesign.setSequentialModel() duplicate state name `") +
           name + "`");
  return false;
}

bool parseClearPresetValue(
    PyObject* stateObject,
    SNLDesignModeling::SequentialState& state) {
  using Value = SNLDesignModeling::SequentialState::ClearPresetValue;
  const char* value = getStringField(
      stateObject, "clear_preset_value", false);
  if (value == nullptr) {
    return not PyErr_Occurred();
  }
  const std::string valueString(value);
  if (valueString == "zero") state.clearPresetValue = Value::Zero;
  else if (valueString == "one") state.clearPresetValue = Value::One;
  else if (valueString == "hold") state.clearPresetValue = Value::Hold;
  else if (valueString == "toggle") state.clearPresetValue = Value::Toggle;
  else if (valueString == "unknown") state.clearPresetValue = Value::Unknown;
  else {
    setError(
        "SNLDesign.setSequentialModel() `clear_preset_value` must be "
        "zero, one, hold, toggle, or unknown");
    return false;
  }
  return true;
}

bool parseSequentialModel(const SNLDesign* selfObject, const char* clockedOn,
    PyObject* statesObject, PyObject* outputsObject, const char* kind,
    SNLDesignModeling::SequentialModel& model, std::string* fieldPath = nullptr) {
  SNLBooleanTree::StateIdentifiers stateIdentifiers;
  for (Py_ssize_t i = 0; i < PyList_Size(statesObject); ++i) {
    if (fieldPath) *fieldPath = "states[" + std::to_string(i) + "]";
    auto* stateObject = PyList_GetItem(statesObject, i);
    if (not PyDict_Check(stateObject)) {
      setError("SNLDesign.setSequentialModel() states must be dictionaries");
      return false;
    }
    const char* name = getStringField(stateObject, "name", true);
    if (name == nullptr ||
        not addStateIdentifier(stateIdentifiers, name, i, false)) {
      return false;
    }
    const char* invertedName = getStringField(
        stateObject, "inverted_name", false);
    if (PyErr_Occurred() ||
        (invertedName != nullptr &&
         not addStateIdentifier(stateIdentifiers, invertedName, i, true))) {
      return false;
    }
  }

  if (fieldPath) *fieldPath = "kind";
  const std::string kindString(kind);
  if (kindString == "flip_flop") {
    model.kind = SNLDesignModeling::SequentialModel::Kind::FlipFlop;
  } else if (kindString == "latch") {
    model.kind = SNLDesignModeling::SequentialModel::Kind::Latch;
  } else {
    setError(
        "SNLDesign.setSequentialModel() kind must be flip_flop or latch");
    return false;
  }
  if (fieldPath) *fieldPath = "clocked_on";
  model.clockedOn = parseExpression(
      selfObject, clockedOn, stateIdentifiers);

  for (Py_ssize_t i = 0; i < PyList_Size(statesObject); ++i) {
    if (fieldPath) *fieldPath = "states[" + std::to_string(i) + "]";
    auto* stateObject = PyList_GetItem(statesObject, i);
    if (fieldPath) *fieldPath = "states[" + std::to_string(i) + "].next_state";
    const char* nextState = getStringField(stateObject, "next_state", true);
    if (nextState == nullptr) return false;

    SNLDesignModeling::SequentialState state;
    auto* init = PyDict_GetItemString(stateObject, "init");
    auto* initFrom = PyDict_GetItemString(stateObject, "init_from");
    if (init && initFrom) {
      setError("Sequential state cannot specify both init and init_from");
      return false;
    }
    if (init) {
      if (!PyLong_Check(init)) {
        setError("Sequential state init must be 0 or 1");
        return false;
      }
      long value = PyLong_AsLong(init);
      if (PyErr_Occurred() || (value != 0 && value != 1)) {
        PyErr_Clear();
        setError("Sequential state init must be 0 or 1");
        return false;
      }
      state.init = value != 0;
    }
    if (initFrom) {
      if (!PyDict_Check(initFrom)) {
        setError("Sequential state init_from must be a dictionary");
        return false;
      }
      const char* parameter = getStringField(initFrom, "parameter", true);
      if (!parameter) return false;
      auto* bitObject = PyDict_GetItemString(initFrom, "bit");
      if (!bitObject || !PyLong_Check(bitObject)) {
        setError("Sequential state init_from.bit must be a nonnegative integer");
        return false;
      }
      size_t bit = PyLong_AsSize_t(bitObject);
      if (PyErr_Occurred()) return false;
      state.initFrom = SNLDesignModeling::SequentialState::InitialValueParameter{parameter, bit};
    }
    state.nextState = parseExpression(
        selfObject, nextState, stateIdentifiers);
    if (fieldPath) *fieldPath = "states[" + std::to_string(i) + "].clear";
    const char* clear = getStringField(stateObject, "clear", false);
    if (PyErr_Occurred()) return false;
    if (clear != nullptr) {
      state.clear = parseExpression(selfObject, clear, stateIdentifiers);
    }
    if (fieldPath) *fieldPath = "states[" + std::to_string(i) + "].preset";
    const char* preset = getStringField(stateObject, "preset", false);
    if (PyErr_Occurred()) return false;
    if (preset != nullptr) {
      state.preset = parseExpression(selfObject, preset, stateIdentifiers);
    }
    if (fieldPath) *fieldPath = "states[" + std::to_string(i) + "].clear_preset_value";
    if (not parseClearPresetValue(stateObject, state)) return false;
    model.states.push_back(std::move(state));
  }

  for (Py_ssize_t i = 0; i < PyList_Size(outputsObject); ++i) {
    if (fieldPath) *fieldPath = "outputs[" + std::to_string(i) + "]";
    auto* outputObject = PyList_GetItem(outputsObject, i);
    if (not PyTuple_Check(outputObject) || PyTuple_Size(outputObject) != 2) {
      setError(
          "SNLDesign.setSequentialModel() outputs must be "
          "(SNLBitTerm, expression) tuples");
      return false;
    }
    auto* termObject = PyTuple_GetItem(outputObject, 0);
    auto* expressionObject = PyTuple_GetItem(outputObject, 1);
    if (not IsPySNLBitTerm(termObject) ||
        not PyUnicode_Check(expressionObject)) {
      setError(
          "SNLDesign.setSequentialModel() outputs must be "
          "(SNLBitTerm, expression) tuples");
      return false;
    }
    auto* term = PYSNLBitTerm_O(termObject);
    if (!term || term->getDesign() != selfObject) {
      setError(
          "SNLDesign.setSequentialModel() output term belongs to "
          "another design");
      return false;
    }
    model.outputs.push_back({
        term,
        parseExpression(
            selfObject, PyUnicode_AsUTF8(expressionObject), stateIdentifiers)});
  }
  return true;
}

}  // namespace

PyObject* PySNLDesign_setSequentialModel(
    PySNLDesign* self, PyObject* args, PyObject* kwargs) {
  const char* clockedOn = nullptr;
  PyObject* statesObject = nullptr;
  PyObject* outputsObject = nullptr;
  const char* kind = "flip_flop";
  static const char* const keywords[] = {
      "clocked_on", "states", "outputs", "kind", nullptr};
  if (not PyArg_ParseTupleAndKeywords(
          args, kwargs, "sOO|s:SNLDesign.setSequentialModel",
          const_cast<char**>(keywords),
          &clockedOn, &statesObject, &outputsObject, &kind)) {
    setError("malformed SNLDesign.setSequentialModel method");
    return nullptr;
  }
  if (not PyList_Check(statesObject) || not PyList_Check(outputsObject)) {
    setError(
        "SNLDesign.setSequentialModel() expects lists for states and outputs");
    return nullptr;
  }
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.setSequentialModel()")

  TRY
  SNLDesignModeling::SequentialModel model;
  if (!parseSequentialModel(selfObject, clockedOn, statesObject, outputsObject, kind, model)) return nullptr;
  SNLDesignModeling::setSequentialModel(selfObject, model);
  NLCATCH
  Py_RETURN_NONE;
}

PyObject* PySNLDesign_hasSequentialModel(PySNLDesign* self) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.hasSequentialModel()")
  if (SNLDesignModeling::hasSequentialModel(selfObject)) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

PyObject* PySNLDesign_hasSequentialModelFromParameters(PySNLDesign* self) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.hasSequentialModelFromParameters()")
  return PyBool_FromLong(SNLDesignModeling::hasSequentialModelFromParameters(selfObject));
}

PyObject* PySNLDesign_setSequentialModelFromParameters(
    PySNLDesign* self, PyObject* args, PyObject* kwargs) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.setSequentialModelFromParameters()")
  const std::string methodContext = "SNLDesign.setSequentialModelFromParameters: " + selfObject->getDescription();
  std::string context = methodContext;
  PyObject* parametersObject = nullptr;
  PyObject* modelsObject = nullptr;
  static const char* const keywords[] = {"parameters", "models", nullptr};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO",
      const_cast<char**>(keywords), &parametersObject, &modelsObject)) {
    setContextualError(context, "expected parameters and models arguments; args=" + describePythonObject(args) +
        ", kwargs=" + describePythonObject(kwargs));
    return nullptr;
  }
  if (!PyList_Check(parametersObject) || !PyList_Check(modelsObject)) {
    setContextualError(context, "expected lists; parameters=" + describePythonObject(parametersObject) +
        ", models=" + describePythonObject(modelsObject));
    return nullptr;
  }
  try {
  std::vector<std::string> parameters;
  for (Py_ssize_t i = 0; i < PyList_Size(parametersObject); ++i) {
    context = methodContext + "; parameters[" + std::to_string(i) + "]";
    auto* name = PyList_GetItem(parametersObject, i);
    if (!PyUnicode_Check(name)) {
      setContextualError(context, "expected a parameter name string; received " + describePythonObject(name));
      return nullptr;
    }
    const char* text = PyUnicode_AsUTF8(name);
    if (!text) { setContextualError(context, "cannot encode parameter name as UTF-8"); return nullptr; }
    parameters.emplace_back(text);
  }
  SNLDesignModeling::SequentialModelTable models;
  for (Py_ssize_t i = 0; i < PyList_Size(modelsObject); ++i) {
    auto* entry = PyList_GetItem(modelsObject, i);
    context = methodContext + "; models[" + std::to_string(i) + "]=" + describePythonObject(entry);
    if (!PyDict_Check(entry)) {
      setContextualError(context, "expected a dictionary with values, clocked_on, states, outputs, and optional kind");
      return nullptr;
    }
    auto* valuesObject = PyDict_GetItemString(entry, "values");
    auto* states = PyDict_GetItemString(entry, "states");
    auto* outputs = PyDict_GetItemString(entry, "outputs");
    const char* clock = getStringField(entry, "clocked_on", true);
    const char* kind = getStringField(entry, "kind", false);
    if (PyErr_Occurred()) { setContextualError(context); return nullptr; }
    if (!valuesObject || !PyList_Check(valuesObject) || !states ||
        !PyList_Check(states) || !outputs || !PyList_Check(outputs)) {
      setContextualError(context, "expected lists for values, states, and outputs; values=" +
          describePythonObject(valuesObject) + ", states=" + describePythonObject(states) +
          ", outputs=" + describePythonObject(outputs));
      return nullptr;
    }
    std::vector<uint64_t> values;
    for (Py_ssize_t j = 0; j < PyList_Size(valuesObject); ++j) {
      auto* value = PyList_GetItem(valuesObject, j);
      if (!PyLong_Check(value)) {
        setContextualError(context, "values[" + std::to_string(j) + "]=" + describePythonObject(value) +
            "; expected an unsigned integer in [0, 18446744073709551615]");
        return nullptr;
      }
      values.push_back(PyLong_AsUnsignedLongLong(value));
      if (PyErr_Occurred()) {
        setContextualError(context, "values[" + std::to_string(j) + "] is outside unsigned 64-bit range");
        return nullptr;
      }
    }
    SNLDesignModeling::SequentialModel model;
    std::string field;
    try {
      if (!parseSequentialModel(selfObject, clock, states, outputs,
          kind ? kind : "flip_flop", model, &field)) {
        setContextualError(context + "; field=" + field);
        return nullptr;
      }
    } catch (const std::exception& error) {
      setContextualError(context + "; field=" + field, error.what());
      return nullptr;
    }
    if (!models.emplace(std::move(values), std::move(model)).second) {
      setContextualError(context, "Duplicate sequential model parameter values; an earlier entry already has this values key; each key must be unique");
      return nullptr;
    }
  }
  context = methodContext;
  SNLDesignModeling::setSequentialModelFromParameters(selfObject, parameters, models);
  } catch (const std::exception& error) {
    setContextualError(context, error.what());
    return nullptr;
  }
  Py_RETURN_NONE;
}

namespace {

PyObject* expressionToPython(const SNLDesignModeling::BooleanExpression& expression,
    size_t id) {
  using Op = SNLDesignModeling::BooleanExpression::Operator;
  const auto& node = expression.nodes.at(id);
  switch (node.operation) {
    case Op::Constant: return PyBool_FromLong(node.constant);
    case Op::Term: return Py_BuildValue("(sN)", "term", PySNLBitTerm_Link(node.term));
    case Op::State: return Py_BuildValue("(sn)", "state", static_cast<Py_ssize_t>(node.state));
    default: break;
  }
  const char* name = node.operation == Op::Not ? "not" :
      node.operation == Op::And ? "and" : node.operation == Op::Or ? "or" : "xor";
  auto* tuple = PyTuple_New(node.operands.size() + 1);
  if (!tuple) return nullptr;
  PyTuple_SET_ITEM(tuple, 0, PyUnicode_FromString(name));
  for (size_t i = 0; i < node.operands.size(); ++i) {
    auto* operand = expressionToPython(expression, node.operands[i]);
    if (!operand) { Py_DECREF(tuple); return nullptr; }
    PyTuple_SET_ITEM(tuple, i + 1, operand);
  }
  return tuple;
}

PyObject* optionalExpressionToPython(
    const std::optional<SNLDesignModeling::BooleanExpression>& expression) {
  if (!expression) Py_RETURN_NONE;
  return expressionToPython(*expression, expression->root);
}

PyObject* modelToPython(const SNLDesignModeling::SequentialModel& model) {
  auto* states = PyList_New(0);
  auto* outputs = PyList_New(0);
  if (!states || !outputs) { Py_XDECREF(states); Py_XDECREF(outputs); return nullptr; }
  const char* values[] = {"zero", "one", "hold", "toggle", "unknown"};
  for (const auto& state : model.states) {
    auto* entry = Py_BuildValue("{s:N,s:N,s:N,s:s}",
        "next_state", expressionToPython(state.nextState, state.nextState.root),
        "clear", optionalExpressionToPython(state.clear),
        "preset", optionalExpressionToPython(state.preset),
        "clear_preset_value", values[static_cast<size_t>(state.clearPresetValue)]);
    if (entry && (state.init || state.initFrom)) {
      auto* value = state.init ? PyLong_FromLong(*state.init) :
          Py_BuildValue("{s:s,s:K}", "parameter", state.initFrom->parameter.c_str(),
              "bit", static_cast<unsigned long long>(state.initFrom->bit));
      if (!value || PyDict_SetItemString(entry, state.init ? "init" : "init_from", value) < 0) {
        Py_XDECREF(value); Py_DECREF(entry); Py_DECREF(states); Py_DECREF(outputs); return nullptr;
      }
      Py_DECREF(value);
    }
    if (!entry || PyList_Append(states, entry) < 0) {
      // LCOV_EXCL_START: Python allocation failure cleanup.
      Py_XDECREF(entry); Py_DECREF(states); Py_DECREF(outputs); return nullptr;
      // LCOV_EXCL_STOP
    }
    Py_DECREF(entry);
  }
  for (const auto& output : model.outputs) {
    auto* entry = Py_BuildValue("(NN)", PySNLBitTerm_Link(output.term),
        expressionToPython(output.function, output.function.root));
    if (!entry || PyList_Append(outputs, entry) < 0) {
      // LCOV_EXCL_START: Python allocation failure cleanup.
      Py_XDECREF(entry); Py_DECREF(states); Py_DECREF(outputs); return nullptr;
      // LCOV_EXCL_STOP
    }
    Py_DECREF(entry);
  }
  return Py_BuildValue("{s:s,s:N,s:N,s:N}", "kind",
      model.kind == SNLDesignModeling::SequentialModel::Kind::FlipFlop ? "flip_flop" : "latch",
      "clocked_on", expressionToPython(model.clockedOn, model.clockedOn.root),
      "states", states, "outputs", outputs);
}

}  // namespace

PyObject* PySNLDesign_getSequentialModel(PySNLDesign* self) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.getSequentialModel()")
  TRY
  return modelToPython(SNLDesignModeling::getSequentialModel(selfObject));
  NLCATCH
}

PyObject* PySNLInstance_getSequentialModel(PySNLInstance* self) {
  auto* selfObject = PYSNLInstance_O(self);
  if (!selfObject) {
    setError("SNLInstance.getSequentialModel(): destroyed instance; cannot resolve sequential behavior through an unbound wrapper; use a live SNLInstance");
    return nullptr;
  }
  TRY
  return modelToPython(SNLDesignModeling::getSequentialModel(selfObject));
  NLCATCH
}

PyObject* PySNLInstance_getSequentialInitialValues(PySNLInstance* self) {
  auto* selfObject = PYSNLInstance_O(self);
  if (!selfObject) {
    setError("SNLInstance.getSequentialInitialValues(): destroyed instance");
    return nullptr;
  }
  TRY
  const auto values = SNLDesignModeling::getSequentialInitialValues(selfObject);
  auto* result = PyList_New(values.size());
  if (!result) return nullptr;
  for (size_t i = 0; i < values.size(); ++i) {
    PyList_SET_ITEM(result, i, values[i] ? PyLong_FromLong(*values[i]) : Py_NewRef(Py_None));
  }
  return result;
  NLCATCH
}

}  // namespace PYNAJA
