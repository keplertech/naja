// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLReset.h"

#include <vector>
#include <string_view>

#include "PyInterface.h"
#include "NLReset.h"
#include "PySNLBitTerm.h"
#include "PySNLDesign.h"
#include "PySNLInstTerm.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLReset, function)

namespace {

bool toTiming(const char* value, NLReset::Timing& timing) {
  const std::string_view name(value);
  if (name == "async") {
    timing = NLReset::Timing::Async;
    return true;
  }
  if (name == "sync") {
    timing = NLReset::Timing::Sync;
    return true;
  }
  setError("NLReset: assertion and deassertion must be 'async' or 'sync'");
  return false;
}

const char* timingName(NLReset::Timing timing) {
  return timing == NLReset::Timing::Async ? "async" : "sync";
}

// Sources must be bit terminals of the owning design.
bool toSource(PyObject* value, NLObject*& source) {
  if (IsPySNLBitTerm(value)) {
    source = PYSNLBitTerm_O(value);
    return true;
  }
  if (IsPySNLInstTerm(value)) {
    source = PYSNLInstTerm_O(value);
    return true;
  }
  return false;
}

bool toSources(PyObject* list, std::vector<NLObject*>& sources) {
  if (not PyList_Check(list)) {
    setError("NLReset sources must be a list of terminals");
    return false;
  }
  Py_ssize_t size = PyList_Size(list);
  for (Py_ssize_t i = 0; i < size; ++i) {
    NLObject* source = nullptr;
    if (not toSource(PyList_GetItem(list, i), source)) {
      setError("NLReset sources must be SNL terminals");
      return false;
    }
    sources.push_back(source);
  }
  return true;
}

PyObject* linkSource(NLObject* source) {
  if (auto* term = dynamic_cast<SNLBitTerm*>(source)) {
    return PySNLBitTerm_Link(term);
  }
  // LCOV_EXCL_START NLReset::checkSource currently permits only SNLBitTerm sources.
  if (auto* instTerm = dynamic_cast<SNLInstTerm*>(source)) {
    return PySNLInstTerm_Link(instTerm);
  }
  setError("NLReset source is not a supported terminal");
  return nullptr;
  // LCOV_EXCL_STOP
}

} // namespace

static PyObject* PyNLReset_create(PyObject*, PyObject* args, PyObject* kwargs) {
  PyObject *design = nullptr, *list = nullptr;
  const char *name = nullptr, *assertion = "async", *deassertion = "sync";
  int level;
  static const char* names[] = {"design", "name", "sources", "activeLevel", "assertion", "deassertion", nullptr};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OsOi|ss:NLReset.create", const_cast<char**>(names),
                                   &design, &name, &list, &level, &assertion, &deassertion)) {
    return nullptr;
  }
  if (!IsPySNLDesign(design)) {
    setError("NLReset.create expects an SNLDesign");
    return nullptr;
  }
  std::vector<NLObject*> sources;
  if (!toSources(list, sources)) {
    return nullptr;
  }
  NLReset::Timing assertionTiming, deassertionTiming;
  if (!toTiming(assertion, assertionTiming) || !toTiming(deassertion, deassertionTiming)) {
    return nullptr;
  }
  TRY
  return PyNLReset_Link(NLReset::create(PYSNLDesign_O(design), NLName(name), sources,
    static_cast<NLReset::ActiveLevel>(level), assertionTiming, deassertionTiming));
  NLCATCH
}
static PyObject* PyNLReset_getActiveLevel(PyNLReset* self) {
  METHOD_HEAD("NLReset.getActiveLevel()")
  return PyLong_FromLong(static_cast<long>(selfObject->getActiveLevel()));
}
static PyObject* PyNLReset_getInactiveValue(PyNLReset* self) {
  METHOD_HEAD("NLReset.getInactiveValue()")
  return PyLong_FromLong(selfObject->getInactiveValue());
}
static PyObject* PyNLReset_getAssertion(PyNLReset* self) {
  METHOD_HEAD("NLReset.getAssertion()")
  return PyUnicode_FromString(timingName(selfObject->getAssertion()));
}
static PyObject* PyNLReset_getDeassertion(PyNLReset* self) {
  METHOD_HEAD("NLReset.getDeassertion()")
  return PyUnicode_FromString(timingName(selfObject->getDeassertion()));
}

static PyObject* PyNLReset_getSources(PyNLReset* self) {
  METHOD_HEAD("NLReset.getSources()")
  PyObject* list = PyList_New(0);
  if (!list) {
    return nullptr;
  }
  for (NLObject* source : selfObject->getSources()) {
    PyObject* linked = linkSource(source);
    if (not linked) {
      // LCOV_EXCL_START Validated sources can fail to link only on CPython allocation failure.
      Py_DECREF(list);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    const int status = PyList_Append(list, linked);
    Py_DECREF(linked);
    if (status < 0) {
      Py_DECREF(list);
      return nullptr;
    }
  }
  return list;
}

GetNameMethod(NLReset)
GetObjectMethod(NLReset, SNLDesign, getDesign)

DBoDestroyAttribute(PyNLReset_destroy, PyNLReset)

PyMethodDef PyNLReset_Methods[] = {
  {"create", (PyCFunction)PyNLReset_create, METH_VARARGS | METH_KEYWORDS | METH_STATIC, "Declare a reset on owning-design bit terminals."},
  {"getName", (PyCFunction)PyNLReset_getName, METH_NOARGS, "getName"},
  {"getDesign", (PyCFunction)PyNLReset_getDesign, METH_NOARGS, "getDesign"},
  {"getSources", (PyCFunction)PyNLReset_getSources, METH_NOARGS, "getSources"},
  {"getActiveLevel", (PyCFunction)PyNLReset_getActiveLevel, METH_NOARGS, "getActiveLevel"},
  {"getInactiveValue", (PyCFunction)PyNLReset_getInactiveValue, METH_NOARGS, "getInactiveValue"},
  {"getAssertion", (PyCFunction)PyNLReset_getAssertion, METH_NOARGS, "getAssertion"},
  {"getDeassertion", (PyCFunction)PyNLReset_getDeassertion, METH_NOARGS, "getDeassertion"},
  {"destroy", (PyCFunction)PyNLReset_destroy, METH_NOARGS, "destroy"},
  {nullptr, nullptr, 0, nullptr}
};

DBoDeallocMethod(NLReset)

DBoLinkCreateMethod(NLReset)
PyTypeNLObjectWithoutNLIDLinkPyType(NLReset)
PyTypeObjectDefinitions(NLReset)

} // namespace PYNAJA
