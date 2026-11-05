// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLClock.h"

#include <vector>

#include "PyInterface.h"
#include "NLClock.h"
#include "PySNLBitTerm.h"
#include "PySNLDesign.h"
#include "PySNLInstTerm.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLClock, function)

namespace {

// Terminals accepted as clock sources: only top-level terminals are supported for now.
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
    setError("NLClock sources must be a list of terminals");
    return false;
  }
  Py_ssize_t size = PyList_Size(list);
  for (Py_ssize_t i = 0; i < size; ++i) {
    NLObject* source = nullptr;
    if (not toSource(PyList_GetItem(list, i), source)) {
      setError("NLClock sources must be SNL terminals");
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
  // LCOV_EXCL_START NLClock::checkSource currently permits only SNLBitTerm sources.
  if (auto* instTerm = dynamic_cast<SNLInstTerm*>(source)) {
    return PySNLInstTerm_Link(instTerm);
  }
  setError("NLClock source is not a supported terminal");
  return nullptr;
  // LCOV_EXCL_STOP
}

}  // namespace

static PyObject* PyNLClock_createPrimary(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  double period = 0.0;
  const char* timeUnit = nullptr;
  PyObject* sourcesList = nullptr;
  if (not PyArg_ParseTuple(args, "OsdsO:NLClock.createPrimary", &arg0, &arg1, &period, &timeUnit, &sourcesList)) {
    setError("malformed NLClock primary creation method");
    return nullptr;
  }
  if (not IsPySNLDesign(arg0)) {
    setError("NLClock createPrimary accepts SNLDesign as first argument");
    return nullptr;
  }
  std::vector<NLObject*> sources;
  if (not toSources(sourcesList, sources)) {
    return nullptr;
  }

  NLClock* clock = nullptr;
  TRY
  clock = NLClock::createPrimary(PYSNLDesign_O(arg0), NLName(arg1), period, timeUnit, sources);
  NLCATCH
  return PyNLClock_Link(clock);
}

static PyObject* PyNLClock_createGenerated(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  PyObject* master = nullptr;
  PyObject* masterSourceObject = nullptr;
  unsigned int divideBy = 1;
  unsigned int multiplyBy = 1;
  int invert = 0;
  PyObject* sourcesList = nullptr;
  if (not PyArg_ParseTuple(args, "OsOOIIpO:NLClock.createGenerated", &arg0, &arg1, &master,
        &masterSourceObject, &divideBy, &multiplyBy, &invert, &sourcesList)) {
    setError("malformed NLClock generated creation method");
    return nullptr;
  }
  if (not IsPySNLDesign(arg0)) {
    setError("NLClock createGenerated accepts SNLDesign as first argument");
    return nullptr;
  }
  if (not IsPyNLClock(master)) {
    setError("NLClock createGenerated expects a master NLClock");
    return nullptr;
  }
  NLObject* masterSource = nullptr;
  if (not toSource(masterSourceObject, masterSource)) {
    setError("NLClock createGenerated expects an SNL terminal as master source");
    return nullptr;
  }
  std::vector<NLObject*> sources;
  if (not toSources(sourcesList, sources)) {
    return nullptr;
  }

  NLClock* clock = nullptr;
  TRY
  clock = NLClock::createGenerated(PYSNLDesign_O(arg0), NLName(arg1), PYNLClock_O(master),
      masterSource, divideBy, multiplyBy, invert != 0, sources);
  NLCATCH
  return PyNLClock_Link(clock);
}

static PyObject* PyNLClock_getKind(PyNLClock* self) {
  METHOD_HEAD("NLClock.getKind()")
  return PyUnicode_FromString(selfObject->getKind() == NLClock::Kind::Primary ? "primary" : "generated");
}

static PyObject* PyNLClock_getPeriod(PyNLClock* self) {
  METHOD_HEAD("NLClock.getPeriod()")
  TRY
  return PyFloat_FromDouble(selfObject->getPeriod());
  NLCATCH
}

static PyObject* PyNLClock_getFrequency(PyNLClock* self) {
  METHOD_HEAD("NLClock.getFrequency()")
  TRY
  return PyFloat_FromDouble(selfObject->getFrequency());
  NLCATCH
}

static PyObject* PyNLClock_getTimeUnit(PyNLClock* self) {
  METHOD_HEAD("NLClock.getTimeUnit()")
  return PyUnicode_FromString(selfObject->getTimeUnit().c_str());
}

static PyObject* PyNLClock_getRiseAt(PyNLClock* self) {
  METHOD_HEAD("NLClock.getRiseAt()")
  TRY
  return PyFloat_FromDouble(selfObject->getRiseAt());
  NLCATCH
}

static PyObject* PyNLClock_getFallAt(PyNLClock* self) {
  METHOD_HEAD("NLClock.getFallAt()")
  TRY
  return PyFloat_FromDouble(selfObject->getFallAt());
  NLCATCH
}

static PyObject* PyNLClock_setWaveform(PyNLClock* self, PyObject* args) {
  METHOD_HEAD("NLClock.setWaveform()")
  double riseAt = 0.0;
  double fallAt = 0.0;
  if (not PyArg_ParseTuple(args, "dd:NLClock.setWaveform", &riseAt, &fallAt)) {
    setError("NLClock.setWaveform() expects (rise_at, fall_at)");
    return nullptr;
  }
  TRY
  selfObject->setWaveform(riseAt, fallAt);
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PyNLClock_getSources(PyNLClock* self) {
  METHOD_HEAD("NLClock.getSources()")
  PyObject* list = PyList_New(0);
  for (NLObject* source: selfObject->getSources()) {
    PyObject* linked = linkSource(source);
    if (not linked) {
      // LCOV_EXCL_START Validated sources can fail to link only on CPython allocation failure.
      Py_DECREF(list);
      return nullptr;
      // LCOV_EXCL_STOP
    }
    PyList_Append(list, linked);
    Py_DECREF(linked);
  }
  return list;
}

static PyObject* PyNLClock_getMaster(PyNLClock* self) {
  METHOD_HEAD("NLClock.getMaster()")
  return PyNLClock_Link(selfObject->getMaster());
}

static PyObject* PyNLClock_getMasterSource(PyNLClock* self) {
  METHOD_HEAD("NLClock.getMasterSource()")
  NLObject* masterSource = selfObject->getMasterSource();
  if (not masterSource) {
    Py_RETURN_NONE;
  }
  return linkSource(masterSource);
}

static PyObject* PyNLClock_getDivideBy(PyNLClock* self) {
  METHOD_HEAD("NLClock.getDivideBy()")
  return PyLong_FromUnsignedLong(selfObject->getDivideBy());
}

static PyObject* PyNLClock_getMultiplyBy(PyNLClock* self) {
  METHOD_HEAD("NLClock.getMultiplyBy()")
  return PyLong_FromUnsignedLong(selfObject->getMultiplyBy());
}

static PyObject* PyNLClock_isInverted(PyNLClock* self) {
  METHOD_HEAD("NLClock.isInverted()")
  return PyBool_FromLong(selfObject->isInverted());
}

static PyObject* PyNLClock_getRootClock(PyNLClock* self) {
  METHOD_HEAD("NLClock.getRootClock()")
  return PyNLClock_Link(selfObject->getRootClock());
}

static PyObject* PyNLClock_isSynchronousWith(PyNLClock* self, PyObject* arg) {
  METHOD_HEAD("NLClock.isSynchronousWith()")
  if (not IsPyNLClock(arg)) {
    setError("NLClock.isSynchronousWith() expects an NLClock");
    return nullptr;
  }
  return PyBool_FromLong(selfObject->isSynchronousWith(PYNLClock_O(arg)));
}

GetNameMethod(NLClock)
GetObjectMethod(NLClock, SNLDesign, getDesign)

DBoDestroyAttribute(PyNLClock_destroy, PyNLClock)

PyMethodDef PyNLClock_Methods[] = {
  { "createPrimary", (PyCFunction)PyNLClock_createPrimary, METH_VARARGS|METH_STATIC,
    "create a primary clock (SDC create_clock) on top-level terminals of an SNLDesign."},
  { "createGenerated", (PyCFunction)PyNLClock_createGenerated, METH_VARARGS|METH_STATIC,
    "create a generated clock (SDC createGenerated_clock) derived from a master NLClock."},
  { "getName", (PyCFunction)PyNLClock_getName, METH_NOARGS,
    "get NLClock name"},
  { "getKind", (PyCFunction)PyNLClock_getKind, METH_NOARGS,
    "get 'primary' or 'generated'."},
  { "getDesign", (PyCFunction)PyNLClock_getDesign, METH_NOARGS,
    "get the SNLDesign owning this NLClock."},
  { "getPeriod", (PyCFunction)PyNLClock_getPeriod, METH_NOARGS,
    "get the clock period in getTimeUnit() units."},
  { "getFrequency", (PyCFunction)PyNLClock_getFrequency, METH_NOARGS,
    "get the clock frequency in 1/getTimeUnit() units, derived from the period."},
  { "getTimeUnit", (PyCFunction)PyNLClock_getTimeUnit, METH_NOARGS,
    "get the time unit of the period, for example 'ns'."},
  { "getRiseAt", (PyCFunction)PyNLClock_getRiseAt, METH_NOARGS,
    "get the first rising edge time (derived from the master for generated clocks)."},
  { "getFallAt", (PyCFunction)PyNLClock_getFallAt, METH_NOARGS,
    "get the next falling edge time; generated values may exceed the period."},
  { "setWaveform", (PyCFunction)PyNLClock_setWaveform, METH_VARARGS,
    "set a primary waveform: 0 <= rise_at < fall_at <= period; generated clocks reject overrides."},
  { "getSources", (PyCFunction)PyNLClock_getSources, METH_NOARGS,
    "get the list of terminals where this clock is defined."},
  { "getMaster", (PyCFunction)PyNLClock_getMaster, METH_NOARGS,
    "get the master NLClock, or None for a primary clock."},
  { "getMasterSource", (PyCFunction)PyNLClock_getMasterSource, METH_NOARGS,
    "get the master source terminal, or None for a primary clock."},
  { "getDivideBy", (PyCFunction)PyNLClock_getDivideBy, METH_NOARGS,
    "get the divide factor of a generated clock (1 for a primary clock)."},
  { "getMultiplyBy", (PyCFunction)PyNLClock_getMultiplyBy, METH_NOARGS,
    "get the multiply factor of a generated clock (1 for a primary clock)."},
  { "isInverted", (PyCFunction)PyNLClock_isInverted, METH_NOARGS,
    "return whether the generated clock is inverted with respect to its master."},
  { "getRootClock", (PyCFunction)PyNLClock_getRootClock, METH_NOARGS,
    "get the primary clock at the root of the master chain (this clock if primary)."},
  { "isSynchronousWith", (PyCFunction)PyNLClock_isSynchronousWith, METH_O,
    "return True if the other NLClock shares the same root clock."},
  { "destroy", (PyCFunction)PyNLClock_destroy, METH_NOARGS,
    "destroy this NLClock."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(NLClock)

DBoLinkCreateMethod(NLClock)
PyTypeNLObjectWithoutNLIDLinkPyType(NLClock)
PyTypeObjectDefinitions(NLClock)

}
